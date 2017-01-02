QT += network
QT -= gui

# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(TeamCity9)
TEMPLATE = lib

CONFIG += C++11

DEFINES += TEAMCITY9_LIBRARY

INCLUDEPATH += ../../interfaces

mac {
    DEFINES += QT_OSX
}

unix:!mac {
    DEFINES += QT_LINUX
}

win32 {
    DLLDESTDIR = ../../../deploy/bin/reporters
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
