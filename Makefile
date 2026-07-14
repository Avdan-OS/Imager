CC=gcc

# Streaming decompression codecs. Remove entries here (and the matching
# libraries below) if a library is unavailable on your system.
# The CMake build auto-detects these instead.
CODEC_DEFS=-DHAVE_ZLIB -DHAVE_LZMA -DHAVE_BZIP2 -DHAVE_ZSTD
CODEC_LIBS=-lz -llzma -lbz2 -lzstd

CFLAGS=-O2 -Wall -I./include $(CODEC_DEFS)
TARGET=imager
SRCS=src/main.c src/core/progress.c src/utils/utils.c src/core/iso_operations.c src/core/image_format.c src/core/image_reader.c src/core/image_write.c src/core/windows_iso.c
OBJS=$(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CODEC_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
