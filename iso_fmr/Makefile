CFLAGS = -Wall -ggdb
LDFLAGS =

all: fmr_decode

clean:
	rm -f fmr_decode fmr_decode.o
	rm -f v20.o v030.o

fmr_decode: fmr_decode.o v20.o v030.o
	$(CC) $^ -o $@ $(LDFLAGS)

fmr_decode.o: fmr_decode.c v20.h v030.h

v20.o: v20.c v20.h

v030.o: v030.c v030.h
