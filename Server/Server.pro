#-------------------------------------------------
#
# Project created by QtCreator 2012-03-29T15:10:22
#
#-------------------------------------------------

QT       += core gui network sql script

TARGET = SION!Server
TEMPLATE = app

DESTDIR = ../Build
unix:{
  QMAKE_LFLAGS += -Wl,--rpath="$$_PRO_FILE_PWD_/../Build"
  QMAKE_LFLAGS_RPATH="$$_PRO_FILE_PWD_/../Build"
}

LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lServerDatabase -lPluginInterface -lClientServerInterface -lQFileExtensions

INCLUDEPATH += ../ServerDatabase
INCLUDEPATH += ../PluginInterface
INCLUDEPATH += ../ClientServerInterface
INCLUDEPATH += ../QFileExtensions

SOURCES += main.cpp \
    server.cpp \
    watcher.cpp \
    classifier.cpp \
    filter.cpp \
    pathsegment.cpp \
    mainwindow.cpp

HEADERS += \
    server.h \
    watcher.h \
    classifier.h \
    filter.h \
    servercommands.h \
    pathsegment.h \
    mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources/Resources.qrc
