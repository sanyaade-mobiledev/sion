#-------------------------------------------------
#
# Project created by QtCreator 2012-03-23T14:25:47
#
#-------------------------------------------------

QT       += script
QT       -= gui

TARGET = Mp3Plugin
TEMPLATE = lib

DESTDIR = ../Build
unix:{
  QMAKE_LFLAGS += -Wl,--rpath="$$_PRO_FILE_PWD_/../Build"
  QMAKE_LFLAGS_RPATH="$$_PRO_FILE_PWD_/../Build"
}


INCLUDEPATH += ../PluginInterface
INCLUDEPATH += ../FilePlugin

LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lPluginInterface
LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lFilePlugin
LIBS += -lid3tag

DEFINES += MP3PLUGIN_LIBRARY

SOURCES += mp3plugin.cpp

HEADERS += mp3plugin.h\
        Mp3Plugin_global.h
