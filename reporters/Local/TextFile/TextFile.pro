QT -= gui

# qtLibraryTarget() adds the 'd' or '_debug' extension, if doing debug builds
TARGET = $$qtLibraryTarget(TextFile)
TEMPLATE = lib

CONFIG += C++11

DEFINES += TEXTFILE_LIBRARY

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

SOURCES += textfile.cpp

HEADERS += textfile.h\
           textfile_global.h \
           ../../interfaces/iplugin.h \
           ../../interfaces/ipluginlocal.h \

target.path = ../../../plugins
CONFIG(debug, debug|release) {
    win32 {
        target.files = debug/*.dll
    }
    unix {
        target.files = debug/*.so
    }
} else {
    win32 {
        target.files = release/*.dll
    }
    unix {
        target.files = release/*.so
    }
}
INSTALLS += target
