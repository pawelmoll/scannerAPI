SCANNER_OBJS = scanner_dummy.o example.o

CFLAGS = -Wall -g

all: decode_iso scan_iso scan_png test

clean:
	rm -f decode_iso decode_iso.o iso_fmr_v20.o iso_fmr_v030.o
	rm -f scan_iso scan_iso.o
	rm -f scan_png scan_png.o
	rm -f test test.o
	rm -f $(SCANNER_OBJS)

decode_iso: decode_iso.o iso_fmr_v20.o iso_fmr_v030.o
	$(CC) $^ -o $@

decode_iso.o: decode_iso.c iso_fmr_v20.h

iso_fmr_v20.o: iso_fmr_v20.c iso_fmr_v20.h

iso_fmr_v030.o: iso_fmr_v030.c iso_fmr_v030.h

scan_iso: scan_iso.o $(SCANNER_OBJS)
	$(CC) $^ -o $@

scan_iso.o: scan_iso.c scanner.h

scan_png: scan_png.o $(SCANNER_OBJS)
	$(CC) $^ -o $@ -lpng

scan_png.o: scan_png.c scanner.h

test: test.o $(SCANNER_OBJS);
	$(CC) $^ -o $@


scanner_dummy.o: scanner_dummy.c example.h scanner.h

example.o: example.c example.h
