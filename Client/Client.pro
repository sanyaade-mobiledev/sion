#-------------------------------------------------
#
# Project created by QtCreator 2012-02-27T18:03:08
#
#-------------------------------------------------

QT       += core gui network

TARGET = SION!Browser
TEMPLATE = app

DESTDIR = ../Build
unix:{
  QMAKE_LFLAGS += -Wl,--rpath="$$_PRO_FILE_PWD_/../Build"
  QMAKE_LFLAGS_RPATH="$$_PRO_FILE_PWD_/../Build"
}

LIBS += -L"$$_PRO_FILE_PWD_/../Build/" -lClientServerInterface -lQFileExtensions

INCLUDEPATH += ../ClientServerInterface
INCLUDEPATH += ../QFileExtensions

SOURCES += main.cpp\
        mainwindow.cpp \
	serverproxy.cpp \
	treenode.cpp \
	newfilterdialog.cpp \
	scriptpage.cpp \
	editfilterdialog.cpp \
	pastefilterdialog.cpp \
	attributesmodel.cpp \
	iconsviewmodel.cpp \
	detailsviewmodel.cpp \
	jsedit.cpp \
	attributecondition.cpp \
	listbrowser.cpp \
	directorybrowser.cpp

HEADERS  += mainwindow.h \
	    serverproxy.h \
	    ../Server/servercommands.h \
	    treenode.h \
	    newfilterdialog.h \
	    common.h \
	    scriptpage.h \
	    editfilterdialog.h \
	    pastefilterdialog.h \
	    attributesmodel.h \
	    iconsviewmodel.h \
	    detailsviewmodel.h \
	    jsedit.h \
	    attributecondition.h \
	    listbrowser.h \
	    directorybrowser.h

FORMS    += mainwindow.ui \
    newfilterdialog.ui \
    scriptpage.ui \
    attributecondition.ui \
    listbrowser.ui

OTHER_FILES += \
    resources/ShowFolderFiles.png \
    resources/NewFolder.png \
    resources/DeleteFolder.png

RESOURCES += \
    resources/Resources.qrc
