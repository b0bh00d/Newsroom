#pragma once

#include <QMainWindow>

#include <QtWidgets/QAction>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QTreeWidgetItem>

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
#include <QtCore/QPluginLoader>
#include <QtCore/QSettings>

#include "specialize.h"
#include "chyron.h"
#include "lanemanager.h"
#include "producer.h"
#include "settings.h"

#include "addstorydialog.h"

#include "reporters/interfaces/ireporter.h"

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

    void                set_visible(bool visible);

    void                save_window_data(QWidget* window);
    void                restore_window_data(QWidget* window);

public:     // data members;
    QString             default_stylesheet;

protected:  // methods
    void                closeEvent(QCloseEvent *event);
    void                dragEnterEvent(QDragEnterEvent *event);
    void                dragMoveEvent(QDragMoveEvent *event);
    void                dropEvent(QDropEvent *event);

private:    // typedefs and enums
    struct StaffInfo
    {
        // this is the staff and equipment assigned to
        // cover a story
        IReporterPointer    reporter;
        ProducerPointer     producer;
        ChyronPointer       chyron;
    };

    SPECIALIZE_MAP(QString, QByteArray, Window)             // "WindowMap"
//    SPECIALIZE_MAP(QString, ChyronPointer, Story)           // "StoryMap"
//    SPECIALIZE_LIST(ProducerPointer, Producer)              // "ProducerList"
    SPECIALIZE_QUEUE(HeadlinePointer, Headline)             // "HeadlineQueue"
    SPECIALIZE_MAP(QUrl, HeadlineQueue, Headline)           // "HeadlineMap"
    SPECIALIZE_VECTOR(ChyronPointer, Story)                 // "StoryVector"
    SPECIALIZE_MAP(QString, PluginsInfoVector, Plugins)     // "PluginsMap"
    SPECIALIZE_MAP(StoryInfoPointer, StaffInfo, Staff)      // "StaffMap"

private slots:
    void                slot_quit();
    void                slot_icon_activated(QSystemTrayIcon::ActivationReason reason);
    void                slot_message_clicked();
    void                slot_menu_action(QAction* action);
    void                slot_restore();
    void                slot_edit_settings(bool checked);

private:    // methods
    bool                load_plugin_factories();
    void                load_application_settings();
    void                save_application_settings();
    void                restore_story_defaults(StoryInfoPointer story_info);
    void                save_story_defaults(StoryInfoPointer story_info);

    void                build_tray_menu();

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

    QFont               headline_font;
    ReportStacking      chyron_stacking;

    QMimeDatabase       mime_db;

    StaffMap            staff;
    LaneManagerPointer  lane_manager;

    PluginsMap          plugins_map;

    SettingsPointer     settings;
    QString             settings_file_name;

    StyleListPointer    headline_style_list;
};
