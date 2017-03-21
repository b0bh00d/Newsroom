QT += network xml widgets
#QT -= gui

# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(YahooChartAPI)
TEMPLATE = lib

CONFIG += C++11

DEFINES += YAHOOCHARTAPI_LIBRARY

INCLUDEPATH += ../../interfaces

CONFIG(debug, debug|release) {
    DLLDESTDIR = ../../../deploy/debug/reporters
    DEFINES += QT_DEBUG
} else {
    DLLDESTDIR = ../../../deploy/release/reporters
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
    chartapi.cpp \
    chartapifactory.cpp \
    chartapipoller.cpp

HEADERS += \
        ../../interfaces/ireporter.h \
    chartapi.h \
    chartapi_global.h \
    chartapifactory.h \
    chartapipoller.h
