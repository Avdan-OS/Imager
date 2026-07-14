#ifndef IMAGE_FORMAT_H
#define IMAGE_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IMG_FORMAT_UNKNOWN = 0,
    IMG_FORMAT_ISO9660,
    IMG_FORMAT_ISO_HYBRID,
    IMG_FORMAT_UDF,
    IMG_FORMAT_RAW_MBR,
    IMG_FORMAT_RAW_GPT,
    IMG_FORMAT_COMPRESSED_GZ,
    IMG_FORMAT_COMPRESSED_XZ,
    IMG_FORMAT_COMPRESSED_BZ2,
    IMG_FORMAT_COMPRESSED_ZSTD
} image_format_t;

typedef struct {
    image_format_t format;
    int has_mbr;
    int has_gpt;
    int has_iso9660;
    int has_udf;
    int has_el_torito;
    char volume_label[33];
    char description[160];
} image_info_t;


int detect_image_format(const char *path, image_info_t *info);
const char *image_format_name(image_format_t fmt);
int image_format_is_raw_writable(const image_info_t *info);

#ifdef __cplusplus
}
#endif

#endif
