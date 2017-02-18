QT += network widgets
QT -= gui

# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(Transmission)
TEMPLATE = lib

CONFIG += C++11

DEFINES += TRANSMISSION_LIBRARY

INCLUDEPATH += ../../interfaces

CONFIG(debug, debug|release) {
    DLLDESTDIR = ../../../deploy/debug/reporters
    DEFINES += QT_DEBUG
} else {
    DLLDESTDIR = ../../../deploy/release/reporters
}

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

SOURCES += \
    transmission.cpp \
    transmissionfactory.cpp \
    transmissionpoller.cpp

HEADERS += \
    ../../interfaces/ireporter.h \
    transmission.h \
    transmissionglobal.h \
    transmissionfactory.h \
    transmissionpoller.h
