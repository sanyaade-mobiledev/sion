 #-------------------------------------------------
#
# Project created by QtCreator 2012-02-08T14:22:57
#
#-------------------------------------------------

QT       += script
QT       -= gui

TARGET = PluginInterface
TEMPLATE = lib
DESTDIR = ../Build

DEFINES += PLUGININTERFACE_LIBRARY

SOURCES += plugininterface.cpp \
    attribute.cpp \
    scriptrunner.cpp \
    script.cpp

HEADERS += plugininterface.h\
    PluginInterface_global.h \
    attribute.h \
    scriptrunner.h \
    script.h \
    plugininterfacewrapper.h
