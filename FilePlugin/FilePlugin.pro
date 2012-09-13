#-------------------------------------------------
#
# Project created by QtCreator 2012-02-08T14:01:41
#
#-------------------------------------------------

QT       += script
QT       -= gui

TARGET = FilePlugin
TEMPLATE = lib

DESTDIR = ../Build
unix:{
  QMAKE_LFLAGS += -Wl,--rpath="$$_PRO_FILE_PWD_/../Build"
  QMAKE_LFLAGS_RPATH="$$_PRO_FILE_PWD_/../Build"
}

LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lPluginInterface

INCLUDEPATH += ../PluginInterface

DEFINES += FILEPLUGIN_LIBRARY

SOURCES += fileplugin.cpp

HEADERS += fileplugin.h\
        FilePlugin_global.h
