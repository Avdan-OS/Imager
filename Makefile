CC=gcc
CFLAGS=-O2 -Wall -I./include
TARGET=imager
SRCS=src/main.c src/core/progress.c src/utils/utils.c src/core/iso_operations.c
OBJS=$(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) 