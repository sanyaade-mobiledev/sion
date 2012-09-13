#-------------------------------------------------
#
# Project created by QtCreator 2012-06-20T10:45:40
#
#-------------------------------------------------

QT       += network
QT       -= gui

TARGET = QFileExtensions
TEMPLATE = lib

DESTDIR = ../Build

DEFINES += QFILEEXTENSIONS_LIBRARY

SOURCES += \
    qfileext.cpp \
    qdirext.cpp \
    qfileinfoext.cpp

HEADERS +=\
        QFileExtensions_global.h \
    qfileext.h \
    qdirext.h \
    qfileinfoext.h

