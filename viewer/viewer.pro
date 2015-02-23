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
        viewer.cpp

HEADERS  += viewer.h

FORMS    += viewer.ui



SOURCES += ../scanner_dummy.c ../example.c
HEADERS += ../scanner.h ../example.h
