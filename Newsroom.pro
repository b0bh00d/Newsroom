#-------------------------------------------------
#
# Project created by QtCreator 2016-11-25T15:24:20
#
#-------------------------------------------------

QT += core gui network widgets xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Newsroom
TEMPLATE = app

CONFIG += C++11

CONFIG(debug, debug|release) {
    #CONFIG += highlight_lanes
    DESTDIR = deploy/debug
} else {
    DESTDIR = deploy/release
}

INCLUDEPATH += reporters/interfaces

RESOURCES += ./Newsroom.qrc

DEFINES += QT_DLL QT_NETWORK_LIB

mac {
    DEFINES += QT_OSX
}

unix:!mac {
    DEFINES += QT_LINUX
}

win32 {
    DEFINES += QT_WIN
}

win32:RC_FILE = Newsroom.rc

INTERMEDIATE_NAME = intermediate
MOC_DIR = $$INTERMEDIATE_NAME/moc
OBJECTS_DIR = $$INTERMEDIATE_NAME/obj
RCC_DIR = $$INTERMEDIATE_NAME/rcc
UI_DIR = $$INTERMEDIATE_NAME/ui

SOURCES += main.cpp \
           mainwindow.cpp \
           headline.cpp \
           chyron.cpp \
           settingsdialog.cpp \
           lanemanager.cpp \
           producer.cpp \
           addstorydialog.cpp \
           editheadlinedialog.cpp \
           settings.cpp \
           label.cpp \
           seriestree.cpp

HEADERS  += mainwindow.h \
            types.h \
            specialize.h \
            headline.h \
            chyron.h \
            settingsdialog.h \
            lanemanager.h \
            animentrytype.def \
            animexittype.def \
            producer.h \
            addstorydialog.h \
            storyinfo.h \
            reporters/interfaces/ireporter.h \
            editheadlinedialog.h \
            settings.h \
            seriesinfo.h \
            label.h \
            seriestree.h

# Plug-in interface
HEADERS += \

FORMS    += mainwindow.ui \
            settingsdialog.ui \
            addstorydialog.ui \
            editheadlinedialog.ui

highlight_lanes {
    SOURCES += highlightwidget.cpp
    HEADERS += highlightwidget.h
    DEFINES += HIGHLIGHT_LANES
}
