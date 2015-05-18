include ../vendors/vendors.mk

CFLAGS = -Wall -ggdb -fPIC
CXXFLAGS = -Wall -ggdb -fPIC

ARCH := $(shell gcc -print-multiarch)
OBJS := core.o dummy.o init.o example.o $(addsuffix .o,$(VENDORS))

CPPFLAGS := $(patsubst %,-I../vendors/%/include,$(VENDORS))
LDFLAGS := $(patsubst %,-L../vendors/%/lib,$(VENDORS))
LDFLAGS += $(patsubst %,-L../vendors/%/lib/$(ARCH),$(VENDORS))

EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
LD_LIBRARY_PATH := $(subst $(SPACE),:,$(patsubst %,$(abspath $(shell pwd)/../vendors/%/lib),$(VENDORS)) $(patsubst %,$(abspath $(shell pwd)/../vendors/%/lib/$(ARCH)),$(VENDORS)))
include $(patsubst %,../vendors/%/libs.mk,$(VENDORS))

all: scan_iso scan_png test setup.sh

clean:
	rm -f scan_iso scan_iso.o
	rm -f scan_png scan_png.o
	rm -f test test.o
	rm -f $(OBJS)
	rm -f setup.sh
	rm -f pyscanner.so pyscanner.o scanner.pyc

decode_iso.o: decode_iso.c

scan_iso: scan_iso.o $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

scan_iso.o: scan_iso.c

scan_png: scan_png.o $(OBJS)
	$(CXX) $^ -o $@ -lpng $(LDFLAGS)

scan_png.o: scan_png.c

test: test.o $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

.PHONY: setup.sh
setup.sh:
	@echo export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):\$$LD_LIBRARY_PATH > $@

# Python wrapper

PYTHONINC ?= /usr/include/python2.7/

.PHONY: python
python: pyscanner.so

pyscanner.so: pyscanner.o $(OBJS)
	$(LD) -shared $^ -o $@ $(LDFLAGS)

pyscanner.o: pyscanner.c
	$(CC) -c $^ -o $@ -I$(PYTHONINC) $(CFLAGS)
