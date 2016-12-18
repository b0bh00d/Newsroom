QT += network
QT -= gui

# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(TeamCity)
TEMPLATE = lib

CONFIG += C++11

DEFINES += TEAMCITY_LIBRARY

INCLUDEPATH += ../../interfaces

mac {
    DEFINES += QT_OSX
}

unix:!mac {
    DEFINES += QT_LINUX
}

win32 {
	DLLDESTDIR = ../../../plugins
    DEFINES += QT_WIN
}

INTERMEDIATE_NAME = intermediate
MOC_DIR = $$INTERMEDIATE_NAME/moc
OBJECTS_DIR = $$INTERMEDIATE_NAME/obj
RCC_DIR = $$INTERMEDIATE_NAME/rcc
UI_DIR = $$INTERMEDIATE_NAME/ui

SOURCES += teamcity.cpp \
    teamcityfactory.cpp

HEADERS += teamcity.h \
        teamcity_global.h \
        teamcityfactory.h \
        ../../interfaces/ireporter.h \
