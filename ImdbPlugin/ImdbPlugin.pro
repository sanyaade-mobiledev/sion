#-------------------------------------------------
#
# Project created by QtCreator 2012-03-23T14:25:47
#
#-------------------------------------------------

QT       += script xml network
QT       -= gui

TARGET = ImdbPlugin
TEMPLATE = lib

DESTDIR = ../Build
unix:{
  QMAKE_LFLAGS += -Wl,--rpath="$$_PRO_FILE_PWD_/../Build"
  QMAKE_LFLAGS_RPATH="$$_PRO_FILE_PWD_/../Build"
}

LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lPluginInterface
LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lFilePlugin
LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lQFileExtensions

INCLUDEPATH += ../PluginInterface
INCLUDEPATH += ../FilePlugin
INCLUDEPATH += ../QFileExtensions

DEFINES += IMDBPLUGIN_LIBRARY

SOURCES += imdbplugin.cpp

HEADERS += imdbplugin.h\
        ImdbPlugin_global.h
