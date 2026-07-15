#ifndef _WIN32
#define _FILE_OFFSET_BITS 64
#endif

#include "imager/image_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#ifdef HAVE_LZMA
#include <lzma.h>
#endif
#ifdef HAVE_BZIP2
#include <bzlib.h>
#endif
#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#ifdef _WIN32
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#else
#define fseek64 fseeko
#define ftell64 ftello
#endif

#define IN_CHUNK 65536

struct image_reader {
    FILE *fp;
    image_format_t format;
    off_t input_size;
    off_t input_pos;
    int input_eof;
    int stream_end;
    unsigned char in_buf[IN_CHUNK];
    size_t in_len;
    size_t in_off;
#ifdef HAVE_ZLIB
    z_stream zs;
    int zs_init;
#endif
#ifdef HAVE_LZMA
    lzma_stream ls;
    int ls_init;
#endif
#ifdef HAVE_BZIP2
    bz_stream bs;
    int bs_init;
#endif
#ifdef HAVE_ZSTD
    ZSTD_DStream *zds;
    ZSTD_inBuffer zin;
#endif
};

int image_reader_format_supported(image_format_t fmt) {
    switch (fmt) {
    case IMG_FORMAT_COMPRESSED_GZ:
#ifdef HAVE_ZLIB
        return 1;
#else
        return 0;
#endif
    case IMG_FORMAT_COMPRESSED_XZ:
#ifdef HAVE_LZMA
        return 1;
#else
        return 0;
#endif
    case IMG_FORMAT_COMPRESSED_BZ2:
#ifdef HAVE_BZIP2
        return 1;
#else
        return 0;
#endif
    case IMG_FORMAT_COMPRESSED_ZSTD:
#ifdef HAVE_ZSTD
        return 1;
#else
        return 0;
#endif
    default:
        return 1;
    }
}

static int fill_input(image_reader_t *r) {
    if (r->in_off < r->in_len) {
        return 0;
    }
    r->in_off = 0;
    r->in_len = fread(r->in_buf, 1, IN_CHUNK, r->fp);
    r->input_pos += (off_t)r->in_len;
    if (r->in_len == 0) {
        if (ferror(r->fp)) {
            perror("read image");
            return -1;
        }
        r->input_eof = 1;
    }
    return 0;
}

static long read_raw(image_reader_t *r, unsigned char *out, size_t len) {
    size_t n = fread(out, 1, len, r->fp);
    if (n < len && ferror(r->fp)) {
        perror("read image");
        return -1;
    }
    r->input_pos += (off_t)n;
    return (long)n;
}

#ifdef HAVE_ZLIB
static long read_gzip(image_reader_t *r, unsigned char *out, size_t len) {
    r->zs.next_out = out;
    r->zs.avail_out = (uInt)len;
    while (r->zs.avail_out > 0 && !r->stream_end) {
        if (r->zs.avail_in == 0 && !r->input_eof) {
            if (fill_input(r) != 0) return -1;
            r->zs.next_in = r->in_buf;
            r->zs.avail_in = (uInt)r->in_len;
            r->in_off = r->in_len;
        }
        int rc = inflate(&r->zs, Z_NO_FLUSH);
        if (rc == Z_STREAM_END) {
            r->stream_end = 1;
            break;
        }
        if (rc == Z_BUF_ERROR) {
            if (r->input_eof) {
                fprintf(stderr, "gzip stream truncated\n");
                return -1;
            }
            continue;
        }
        if (rc != Z_OK) {
            fprintf(stderr, "gzip decompression error (%d)\n", rc);
            return -1;
        }
        if (r->input_eof && r->zs.avail_in == 0) break;
    }
    return (long)(len - r->zs.avail_out);
}
#endif

#ifdef HAVE_LZMA
static long read_xz(image_reader_t *r, unsigned char *out, size_t len) {
    r->ls.next_out = out;
    r->ls.avail_out = len;
    while (r->ls.avail_out > 0 && !r->stream_end) {
        if (r->ls.avail_in == 0 && !r->input_eof) {
            if (fill_input(r) != 0) return -1;
            r->ls.next_in = r->in_buf;
            r->ls.avail_in = r->in_len;
            r->in_off = r->in_len;
        }
        lzma_ret rc = lzma_code(&r->ls, r->input_eof ? LZMA_FINISH : LZMA_RUN);
        if (rc == LZMA_STREAM_END) {
            r->stream_end = 1;
            break;
        }
        if (rc != LZMA_OK) {
            fprintf(stderr, "xz decompression error (%d)\n", (int)rc);
            return -1;
        }
    }
    return (long)(len - r->ls.avail_out);
}
#endif

#ifdef HAVE_BZIP2
static long read_bz2(image_reader_t *r, unsigned char *out, size_t len) {
    r->bs.next_out = (char *)out;
    r->bs.avail_out = (unsigned int)len;
    while (r->bs.avail_out > 0 && !r->stream_end) {
        if (r->bs.avail_in == 0) {
            if (fill_input(r) != 0) return -1;
            if (r->in_len == 0) {
                fprintf(stderr, "bzip2 stream truncated\n");
                return -1;
            }
            r->bs.next_in = (char *)r->in_buf;
            r->bs.avail_in = (unsigned int)r->in_len;
            r->in_off = r->in_len;
        }
        int rc = BZ2_bzDecompress(&r->bs);
        if (rc == BZ_STREAM_END) {
            r->stream_end = 1;
            break;
        }
        if (rc != BZ_OK) {
            fprintf(stderr, "bzip2 decompression error (%d)\n", rc);
            return -1;
        }
    }
    return (long)(len - r->bs.avail_out);
}
#endif

#ifdef HAVE_ZSTD
static long read_zstd(image_reader_t *r, unsigned char *out, size_t len) {
    ZSTD_outBuffer zout;
    zout.dst = out;
    zout.size = len;
    zout.pos = 0;
    while (zout.pos < zout.size && !r->stream_end) {
        if (r->zin.pos >= r->zin.size) {
            if (fill_input(r) != 0) return -1;
            if (r->in_len == 0) break;
            r->zin.src = r->in_buf;
            r->zin.size = r->in_len;
            r->zin.pos = 0;
            r->in_off = r->in_len;
        }
        size_t rc = ZSTD_decompressStream(r->zds, &zout, &r->zin);
        if (ZSTD_isError(rc)) {
            fprintf(stderr, "zstd decompression error: %s\n", ZSTD_getErrorName(rc));
            return -1;
        }
        if (rc == 0 && r->zin.pos >= r->zin.size && r->input_eof) {
            r->stream_end = 1;
        }
    }
    return (long)zout.pos;
}
#endif

static int init_codec(image_reader_t *r, image_format_t format) {
    (void)r;
    switch (format) {
#ifdef HAVE_ZLIB
    case IMG_FORMAT_COMPRESSED_GZ:
        if (inflateInit2(&r->zs, 15 + 32) != Z_OK) {
            fprintf(stderr, "inflateInit2 failed\n");
            return -1;
        }
        r->zs_init = 1;
        break;
#endif
#ifdef HAVE_LZMA
    case IMG_FORMAT_COMPRESSED_XZ: {
        lzma_stream init = LZMA_STREAM_INIT;
        r->ls = init;
        if (lzma_stream_decoder(&r->ls, UINT64_MAX, LZMA_CONCATENATED) != LZMA_OK) {
            fprintf(stderr, "lzma_stream_decoder failed\n");
            return -1;
        }
        r->ls_init = 1;
        break;
    }
#endif
#ifdef HAVE_BZIP2
    case IMG_FORMAT_COMPRESSED_BZ2:
        if (BZ2_bzDecompressInit(&r->bs, 0, 0) != BZ_OK) {
            fprintf(stderr, "BZ2_bzDecompressInit failed\n");
            return -1;
        }
        r->bs_init = 1;
        break;
#endif
#ifdef HAVE_ZSTD
    case IMG_FORMAT_COMPRESSED_ZSTD:
        r->zds = ZSTD_createDStream();
        if (r->zds == NULL) {
            fprintf(stderr, "ZSTD_createDStream failed\n");
            return -1;
        }
        ZSTD_initDStream(r->zds);
        break;
#endif
    default:
        break;
    }
    return 0;
}

image_reader_t *image_reader_open(const char *path, image_format_t format) {
    image_reader_t *r = (image_reader_t *)calloc(1, sizeof(*r));
    if (r == NULL) {
        perror("calloc");
        return NULL;
    }
    r->format = format;

    if (!image_reader_format_supported(format)) {
        fprintf(stderr, "Error: this build has no support for: %s\n",
                image_format_name(format));
        free(r);
        return NULL;
    }

    r->fp = fopen(path, "rb");
    if (r->fp == NULL) {
        perror("open image");
        free(r);
        return NULL;
    }

    if (fseek64(r->fp, 0, SEEK_END) == 0) {
        r->input_size = (off_t)ftell64(r->fp);
    }
    fseek64(r->fp, 0, SEEK_SET);

    if (init_codec(r, format) != 0) {
        fclose(r->fp);
        free(r);
        return NULL;
    }
    return r;
}

long image_reader_read(image_reader_t *r, void *buf, size_t len) {
    if (len == 0 || r->stream_end) {
        return 0;
    }
    switch (r->format) {
#ifdef HAVE_ZLIB
    case IMG_FORMAT_COMPRESSED_GZ:
        return read_gzip(r, (unsigned char *)buf, len);
#endif
#ifdef HAVE_LZMA
    case IMG_FORMAT_COMPRESSED_XZ:
        return read_xz(r, (unsigned char *)buf, len);
#endif
#ifdef HAVE_BZIP2
    case IMG_FORMAT_COMPRESSED_BZ2:
        return read_bz2(r, (unsigned char *)buf, len);
#endif
#ifdef HAVE_ZSTD
    case IMG_FORMAT_COMPRESSED_ZSTD:
        return read_zstd(r, (unsigned char *)buf, len);
#endif
    default:
        return read_raw(r, (unsigned char *)buf, len);
    }
}

off_t image_reader_input_pos(const image_reader_t *r) {
    return r->input_pos;
}

off_t image_reader_input_size(const image_reader_t *r) {
    return r->input_size;
}

int image_reader_is_compressed(const image_reader_t *r) {
    return r->format >= IMG_FORMAT_COMPRESSED_GZ;
}

void image_reader_close(image_reader_t *r) {
    if (r == NULL) {
        return;
    }
#ifdef HAVE_ZLIB
    if (r->zs_init) inflateEnd(&r->zs);
#endif
#ifdef HAVE_LZMA
    if (r->ls_init) lzma_end(&r->ls);
#endif
#ifdef HAVE_BZIP2
    if (r->bs_init) BZ2_bzDecompressEnd(&r->bs);
#endif
#ifdef HAVE_ZSTD
    if (r->zds) ZSTD_freeDStream(r->zds);
#endif
    if (r->fp) fclose(r->fp);
    free(r);
}
