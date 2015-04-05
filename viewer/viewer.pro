QT += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = viewer
TEMPLATE = app

SOURCES += main.cpp viewer.cpp fingerprint.cpp scanner.cpp

HEADERS += viewer.h fingerprint.h scanner.h

FORMS += viewer.ui

INCLUDEPATH += $$PWD/..

unix:!macx: LIBS += -L$$OUT_PWD/../iso_fmr/ -liso_fmr -L$$OUT_PWD/../scanner/ -lscanner

unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/../iso_fmr/libiso_fmr.a $$OUT_PWD/../scanner/libscanner.a

ARCH = $$system(gcc -print-multiarch)
VENDORS = $$fromfile(../vendors/vendors.mk, VENDORS)

for (VENDOR, $$list($$VENDORS)) {
    VENDOR_LIBS = $$fromfile(../vendors/$$VENDOR/libs.mk, LDFLAGS)
    unix:!macx: LIBS += -L$$PWD/../vendors/$$VENDOR/lib -L$$PWD/../vendors/$$VENDOR/lib/$$ARCH $$VENDOR_LIBS
}
