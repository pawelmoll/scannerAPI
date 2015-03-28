CFLAGS = -Wall -ggdb
LDFLAGS =

all: decode_iso scan_iso scan_png test

clean:
	rm -f scan_iso scan_iso.o
	rm -f scan_png scan_png.o
	rm -f test test.o
	rm -f decode_iso decode_iso.o
	rm -f iso_fmr.a iso_fmr_v20.o iso_fmr_v030.o
	rm -f scanner.a scanner_core.o scanner_init.o
	rm -f scanner_dummy.o example.o

decode_iso: decode_iso.o  iso_fmr.a
	$(CC) $^ -o $@

iso_fmr.a: iso_fmr_v20.o iso_fmr_v030.o
	$(AR) rcs $@ $^

decode_iso.o: decode_iso.c iso_fmr_v20.h

iso_fmr_v20.o: iso_fmr_v20.c iso_fmr_v20.h

iso_fmr_v030.o: iso_fmr_v030.c iso_fmr_v030.h

scan_iso: scan_iso.o scanner.a
	$(CC) $^ -o $@ $(LDFLAGS)

scan_iso.o: scan_iso.c scanner.h

scan_png: scan_png.o scanner.a
	$(CC) $^ -o $@ -lpng $(LDFLAGS)

scan_png.o: scan_png.c scanner.h

test: test.o scanner.a
	$(CC) $^ -o $@ $(LDFLAGS)

scanner.a: scanner_core.o scanner_init.o scanner_dummy.o example.o
	$(AR) rcs $@ $^

scanner_init.o: scanner_init.c

scanner_core.o: scanner_core.c scanner_driver.h scanner.h

scanner_dummy.o: scanner_dummy.c scanner_driver.h scanner.h example.h
example.o: example.c example.h
