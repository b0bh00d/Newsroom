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

#include "seriesinfo.h"
#include "lanemanager.h"
#include "settings.h"

#include "addstorydialog.h"
#include "settingsdialog.h"

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

public:     // methods
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void                set_visible(bool visible);

    void                save_window_data(QWidget* window);
    void                restore_window_data(QWidget* window);

    inline QString      encode_for_filesystem(const QString& str) const;

public:     // data members;
    QString             default_stylesheet;

protected:  // methods
    void                paintEvent(QPaintEvent*);
    void                closeEvent(QCloseEvent *event);
    void                dragEnterEvent(QDragEnterEvent *event);
    void                dragMoveEvent(QDragMoveEvent *event);
    void                dropEvent(QDropEvent *event);

private:    // typedefs and enums
    enum class CoverageStart
    {
        None,
        Immediate,
        Delayed
    };

    SPECIALIZE_MAP(QString, QByteArray, Window)             // "WindowMap"
    SPECIALIZE_QUEUE(HeadlinePointer, Headline)             // "HeadlineQueue"
    SPECIALIZE_MAP(QUrl, HeadlineQueue, Headline)           // "HeadlineMap"
    SPECIALIZE_SHAREDPTR(QPixmap, Pixmap)                   // "PixmapPointer"
    SPECIALIZE_LIST(StoryInfoPointer, Story)                // "StoryList"
    SPECIALIZE_MAP(QString, QString, String)                // "StringMap"
    SPECIALIZE_MAP(QString, QStringList, StringList)        // "StringListMap"

private slots:
    void                slot_quit();
    void                slot_icon_activated(QSystemTrayIcon::ActivationReason reason);
    void                slot_message_clicked();
    void                slot_menu_action(QAction* action);
    void                slot_restore();
    void                slot_edit_settings(bool checked);
    void                slot_edit_story(const QString& story_id);

private:    // methods
    bool                configure_reporters();
    void                load_application_settings();
    void                save_application_settings();
    void                load_series(SeriesInfoPointer series_info);
    void                save_series(SeriesInfoPointer series_info);
    void                save_story(SettingsPointer application_settings, StoryInfoPointer story_info);
    void                restore_story(SettingsPointer application_settings, StoryInfoPointer story_info);
    void                restore_story_defaults(StoryInfoPointer story_info);
    void                save_story_defaults(StoryInfoPointer story_info);
    bool                cover_story(ProducerPointer& producer, StoryInfoPointer story_info, CoverageStart coverage_start, const ReportersInfoVector *reporters_info = nullptr);
    void                fix_angle_duplication(StoryInfoPointer story_info);

    const ReporterInfo* get_reporter_info(const QString& id) const;

    void                build_tray_menu();

private:    // data members
    Ui::MainWindow*     ui;

    bool                auto_start;
    bool                continue_coverage;
    bool                edit_story_first_time;

    QSystemTrayIcon*    trayIcon;
    QMenu*              trayIconMenu;

    QAction*            settings_action;
    QAction*            quit_action;
    QAction*            options_action;
    QAction*            about_action;

    WindowMap           window_data;

    bool                window_geometry_save_enabled;
    bool                start_automatically;
    bool                settings_modified;

    QFont               headline_font;

    QMimeDatabase       mime_db;

    SeriesInfoList      series_ordered;

    LaneManagerPointer  lane_manager;

    BeatsMap            beats;

    SettingsPointer     application_settings;
    QString             application_settings_file_name;

    StyleListPointer    headline_style_list;

    int                 last_start_offset;

    PixmapPointer       background_image;

    SettingsDialog*     settings_dlg;

    QString             series_folder;
    QString             parameters_base_folder;
    QString             parameters_defaults_folder;
    QString             parameters_stories_folder;
};
