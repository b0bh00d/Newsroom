#pragma once

#include <QMainWindow>

#include <QtWidgets/QAction>
#include <QtWidgets/QSystemTrayIcon>

#include <QtGui/QCloseEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QDropEvent>

#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtCore/QMap>
#include <QtCore/QVector>
#include <QtCore/QByteArray>
#include <QtCore/QMimeDatabase>

#include "specialize.h"
#include "chyron.h"
#include "reporter_local.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

#ifndef NDEBUG
#define DEBUG
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void            set_visible(bool visible);

protected:  // methods
    void            closeEvent(QCloseEvent *event);
    void            dragEnterEvent(QDragEnterEvent *event);
    void            dragMoveEvent(QDragMoveEvent *event);
    void            dropEvent(QDropEvent *event);

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, QByteArray, Window)     // "WindowMap"
    SPECIALIZE_MAP(QUrl, ChyronPointer, Story)      // "StoryMap"
    SPECIALIZE_LIST(ReporterPointer, Reporter)      // "ReporterList"
    SPECIALIZE_QUEUE(ArticlePointer, Article)       // "ArticleQueue"
    SPECIALIZE_MAP(QUrl, ArticleQueue, Article)     // "ArticleMap"

private slots:
    void            slot_quit();
    void            slot_icon_activated(QSystemTrayIcon::ActivationReason reason);
    void            slot_message_clicked();
    void            slot_menu_action(QAction* action);
    void            slot_restore();

    private:    // methods
    void            load_application_settings();
    void            save_application_settings();
    void            save_window_data(QWidget* window);
    void            restore_window_data(QWidget* window);

    void            build_tray_menu();

private:    // data members
    Ui::MainWindow*     ui;

    QSystemTrayIcon*    trayIcon;
    QMenu*              trayIconMenu;

    QAction*            quit_action;
    QAction*            options_action;
    QAction*            about_action;

    WindowMap           window_data;

    bool                window_geometry_save_enabled;
    bool                start_automatically;
    bool                settings_modified;

    QMimeDatabase       mime_db;

    StoryMap            storys;
    ReporterList        reporters;
};
