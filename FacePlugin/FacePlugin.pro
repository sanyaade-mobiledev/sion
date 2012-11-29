#-------------------------------------------------
#
# Project created by QtCreator 2012-03-23T14:25:47
#
#-------------------------------------------------

QT       += script gui

TARGET = FacePlugin
TEMPLATE = lib

DESTDIR = ../Build
unix:{
  QMAKE_LFLAGS += -Wl,--rpath="$$_PRO_FILE_PWD_/../Build"
  QMAKE_LFLAGS_RPATH="$$_PRO_FILE_PWD_/../Build"
}


INCLUDEPATH += ../PluginInterface
INCLUDEPATH += ../FilePlugin
INCLUDEPATH += -I/usr/local/include/opencv

LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lPluginInterface
LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lFilePlugin
LIBS += -L//usr/local/lib/ -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

DEFINES += FACEPLUGIN_LIBRARY

SOURCES += faceplugin.cpp \
    qopencvimage.cpp

HEADERS += faceplugin.h\
        FacePlugin_global.h \
    qopencvimage.h
