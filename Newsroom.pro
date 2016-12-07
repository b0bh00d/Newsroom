#-------------------------------------------------
#
# Project created by QtCreator 2016-11-25T15:24:20
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Newsroom
TEMPLATE = app

CONFIG += C++11


CONFIG(debug, debug|release) {
    CONFIG += highlight_lanes
}

RESOURCES += ./Newsroom.qrc

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
        reporter.cpp \
        reporter_local.cpp \
        addlocaldialog.cpp \
        headline.cpp \
        chyron.cpp \
        settingsdialog.cpp \
    lanemanager.cpp

HEADERS  += mainwindow.h \
            reporter.h \
            reporter_local.h \
            addlocaldialog.h \
            types.h \
            specialize.h \
            headline.h \
            chyron.h \
            settingsdialog.h \
            animentrytype.def \
            animexittype.def \
    lanemanager.h

FORMS    += mainwindow.ui \
            addlocaldialog.ui \
            settingsdialog.ui

highlight_lanes {
    SOURCES += highlightwidget.cpp
    HEADERS += highlightwidget.h
    DEFINES += HIGHLIGHT_LANES
}
