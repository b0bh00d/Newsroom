#-------------------------------------------------
#
# Project created by QtCreator 2016-12-08T14:05:59
#
#-------------------------------------------------

QT -= gui

DESTDIR = ../../plugins
# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(Local)
TEMPLATE = lib

CONFIG += C++11

DEFINES += LOCAL_LIBRARY

INCLUDEPATH += ../interfaces

mac {
    DEFINES += QT_OSX
}

unix:!mac {
    DEFINES += QT_LINUX
}

win32 {
    DEFINES += QT_WIN
}

INTERMEDIATE_NAME = intermediate
MOC_DIR = $$INTERMEDIATE_NAME/moc
OBJECTS_DIR = $$INTERMEDIATE_NAME/obj
RCC_DIR = $$INTERMEDIATE_NAME/rcc
UI_DIR = $$INTERMEDIATE_NAME/ui

SOURCES += local.cpp

HEADERS += local.h\
           local_global.h \
           ../interfaces/iplugin.h \
           ../interfaces/ipluginlocal.h \

unix {
    target.path = /usr/lib
    INSTALLS += target
}
