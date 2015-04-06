TEMPLATE = lib
CONFIG = staticlib

TARGET = scanner

SOURCES += core.c init.c dummy.c example.c
HEADERS += scanner.h driver.h example.h

VENDORS = $$fromfile(../vendors/vendors.mk, VENDORS)

for (VENDOR, $$list($$VENDORS)) {
    INCLUDEPATH += ../vendors/$$VENDOR/include
    DEPENDPATH += ../vendors/$$VENDOR/include
}
