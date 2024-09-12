CC = gcc
CFLAGS = -Wall -O3 
INCLUDES = 
LFLAGS = 
LIBS = -ljpeg -lm

OBJS = fisheye_stitcher.o bitmaplib.o

all: fisheye_stitcher

fisheye_stitcher: $(OBJS)
	$(CC) $(INCLUDES) $(CFLAGS) -o fisheye_stitcher $(OBJS) $(LFLAGS) $(LIBS) 

fisheye_stitcher.o: fisheye_stitcher.c fisheye_stitcher.h
	$(CC) $(INCLUDES) $(CFLAGS) -c fisheye_stitcher.c
 
bitmaplib.o: bitmaplib.c bitmaplib.h
	$(CC) $(INCLUDES) $(CFLAGS) -c bitmaplib.c

clean:
	rm -rf core fisheye_stitcher $(OBJS)

