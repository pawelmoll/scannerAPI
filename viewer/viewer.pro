#-------------------------------------------------
#
# Project created by QtCreator 2015-02-23T12:06:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = viewer
TEMPLATE = app


SOURCES += main.cpp\
        viewer.cpp \
    ../iso_fmr_v20.c \
    ../iso_fmr_v030.c \
    fingerprint.cpp \
    scanner.cpp

HEADERS  += viewer.h \
    ../iso_fmr_v20.h \
    ../iso_fmr_v030.h \
    fingerprint.h \
    scanner.h

FORMS    += viewer.ui



SOURCES += ../scanner_dummy.c ../example.c
HEADERS += ../scanner.h ../example.h
