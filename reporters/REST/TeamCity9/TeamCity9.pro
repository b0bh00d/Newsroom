QT += network
QT -= gui

# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(TeamCity9)
TEMPLATE = lib

CONFIG += C++11

DEFINES += TEAMCITY9_LIBRARY

INCLUDEPATH += ../../interfaces

CONFIG(debug, debug|release) {
    DLLDESTDIR = ../../../deploy/debug/reporters
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
    teamcity9.cpp \
    teamcity9factory.cpp \
    teamcity9poller.cpp

HEADERS += \
        ../../interfaces/ireporter.h \
    teamcity9.h \
    teamcity9_global.h \
    teamcity9factory.h \
    teamcity9poller.h
