QT += network widgets
QT -= gui

# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(Transmission)
TEMPLATE = lib

CONFIG += C++11

DEFINES += TRANSMISSION_LIBRARY

INCLUDEPATH += ../../interfaces

CONFIG(debug, debug|release) {
    win32 {
        DLLDESTDIR = ../../../deploy/debug/reporters
    }
    unix {
        DESTDIR = ../../../deploy/debug/reporters
    }
    DEFINES += QT_DEBUG
} else {
    win32 {
        DLLDESTDIR = ../../../deploy/release/reporters
    }
    unix {
        DESTDIR = ../../../deploy/release/reporters
    }
    #CONFIG += drmemory
}

drmemory {
    CONFIG(debug, debug|release) {
        DESTDIR = ../../../deploy/debug/reporters
    } else {
        DESTDIR = ../../../deploy/release/reporters
    }
    QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE += $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}

mac {
    DEFINES += QT_OSX
}

unix:!mac {
    DEFINES += QT_LINUX
    QMAKE_CXXFLAGS += -Wno-reorder -Wno-switch
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
