#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMessageBox>

#include <QtGui/QFontMetrics>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

#include "types.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "settingsdialog.h"
#include "chyron.h"

static const QVector<ReportStacking> reportstacking_vec
{
#   define X(a) ReportStacking::a,
#   include "reportstacking.def"
#   undef X
};

MainWindow::MainWindow(QWidget *parent)
    : trayIconMenu(0),
      window_geometry_save_enabled(true),
      start_automatically(false),
      settings_modified(false),
      chyron_stacking(ReportStacking::Stacked),
      QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    headline_font = ui->label->font();

//    QPixmap bkgnd(":/images/Add.png");
//    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
//    QPalette palette;
//    palette.setBrush(QPalette::Background, bkgnd);
//    this->setPalette(palette);

    setWindowTitle(tr("Newsroom by Bob Hood"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));

    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    settings = new QSettings(settings_file_name, QSettings::IniFormat);

    load_application_settings();

    setAcceptDrops(true);

    lane_manager = LaneManagerPointer(new LaneManager(headline_font, headline_stylesheet_normal, this));

    // Load all the available Reporter plug-ins
    if(!load_plugin_factories())
    {
        QMessageBox::critical(0,
                              tr("Newsroom: Error"),
                              tr("No Reporter plug-ins were found!\n"
                                 "The Newsroom cannot function without Reporters."));
        qApp->quit();
        return;
    }

    // Tray

    trayIcon = new QSystemTrayIcon(this);
    bool connected = connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::slot_message_clicked);
    ASSERT_UNUSED(connected);
    connected = connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slot_icon_activated);
    ASSERT_UNUSED(connected);

    connected = connect(ui->action_Settings, &QAction::triggered, this, &MainWindow::slot_edit_settings);
    ASSERT_UNUSED(connected);

    trayIcon->setIcon(QIcon(":/images/Newsroom16x16.png"));
    trayIcon->setToolTip(tr("Newsroom"));

    build_tray_menu();

    trayIcon->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::load_plugin_factories()
{
    plugins_map.clear();

    QDir plugins("plugins");
    QStringList plugins_list = plugins.entryList(QStringList() << "*.dll" << "*.so", QDir::Files);
    foreach(const QString& filename, plugins_list)
    {
        QString plugin_path = QDir::toNativeSeparators(QString("%1/%2").arg(plugins.absolutePath()).arg(filename));
        FactoryPointer plugin(new QPluginLoader(plugin_path));
        QObject* instance = plugin->instance();
        if(instance)
        {
            PluginInfo pi_info;
            pi_info.factory = plugin;
            pi_info.path = plugin_path;

            IPluginFactory* ipluginfactory = reinterpret_cast<IPluginFactory*>(instance);
            if(ipluginfactory)
            {
                IPluginPointer iplugin = ipluginfactory->newInstance();

                QStringList display = iplugin->DisplayName();
                pi_info.name = display[0];
                pi_info.tooltip = display[1];
                pi_info.id = iplugin->PluginID();

                QString pi_class = iplugin->PluginClass();

                if(!plugins_map.contains(pi_class))
                    plugins_map[pi_class] = PluginsInfoVector();
                plugins_map[pi_class].push_back(pi_info);
            }
        }
    }

    return plugins_map.count() > 0;
}

void MainWindow::set_visible(bool visible)
{
    if(visible)
        restore_window_data(this);

    QWidget::setVisible(visible);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if(trayIcon->isVisible())
    {
        save_window_data(this);
        hide();
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    bool accept = true;

    //    QStringList formats = event->mimeData()->formats();
    //    if(event->mimeData()->hasFormat("text/uri-list"))

    if(event->mimeData()->hasUrls())
    {
        QList<QUrl> urls = event->mimeData()->urls();
        foreach(const QUrl& url, urls)
        {
            if(url.isLocalFile())
            {
                QString text = url.toLocalFile();
                QString name = mime_db.mimeTypeForFile(text).name();
                if(!name.startsWith("text/"))
                    accept = false;
            }
        }
    }
    else
        accept = false;

    if(accept)
        event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    bool accept = false;
    QString text;

    // each element in the list has already been vetted
    // in dragEnterEvent() above
    QList<QUrl> urls = event->mimeData()->urls();
    foreach(const QUrl& story, urls)
    {
        PluginsInfoVector* reporters_info = nullptr;

        if(story.isLocalFile())
        {
            if(plugins_map.contains("Local"))
                reporters_info = &plugins_map["Local"];
        }
        else
        {
            if(plugins_map.contains("URL"))
                reporters_info = &plugins_map["URL"];
        }

        if(!reporters_info)
        {
            QMessageBox::critical(0,
                                  tr("Newsroom: Error"),
                                  tr("No Reporter plug-ins are available to cover that Story!"));
            return;
        }

        AddStoryDialog addstory_dlg;
        addstory_dlg.load_defaults(settings);

        restore_window_data(&addstory_dlg);

        addstory_dlg.set_story(story);
        addstory_dlg.set_reporters(reporters_info);

        if(addstory_dlg.exec() == QDialog::Accepted)
        {
            accept = true;

            QString story_id = addstory_dlg.get_story_identity();

            Chyron::Settings chyron_settings;
            chyron_settings.entry_type = addstory_dlg.get_animation_entry_type();

            // see if this story is already being covered by a Reporter
            if(stories.find(story_id) == stories.end())
            {
                int pixel_width = 0, pixel_height = 0;
                double percent_width = 0.0, percent_height = 0.0;
                bool interpret_as_pixels = false, interpret_as_percent = false;

                interpret_as_pixels = addstory_dlg.get_headlines_size(pixel_width, pixel_height);
                if(!interpret_as_pixels)
                    interpret_as_percent = addstory_dlg.get_headlines_size(percent_width, percent_height);

                chyron_settings.ttl                     = addstory_dlg.get_ttl();
                chyron_settings.display                 = addstory_dlg.get_display();
                chyron_settings.always_visible          = addstory_dlg.get_headlines_always_visible();
                chyron_settings.exit_type               = addstory_dlg.get_animation_exit_type();
                chyron_settings.anim_motion_duration    = addstory_dlg.get_anim_motion_duration();
                chyron_settings.fade_target_duration    = addstory_dlg.get_fade_target_duration();
                chyron_settings.stacking_type           = chyron_stacking;
                chyron_settings.headline_pixel_width    = interpret_as_pixels ? pixel_width : 0;
                chyron_settings.headline_pixel_height   = interpret_as_pixels ? pixel_height : 0;
                chyron_settings.headline_percent_width  = interpret_as_percent ? percent_width : 0.0;
                chyron_settings.headline_percent_height = interpret_as_percent ? percent_height : 0.0;
                chyron_settings.headline_fixed_text     = addstory_dlg.get_headlines_fixed_text();
                chyron_settings.train_effect            = addstory_dlg.get_train_age_effects(chyron_settings.train_reduce_opacity);
                chyron_settings.dashboard_group         = addstory_dlg.get_dashboard_group_id();
                chyron_settings.dashboard_effect        = addstory_dlg.get_dashboard_age_effects(chyron_settings.dashboard_reduce_opacity);

                stories[story_id] = ChyronPointer(new Chyron(addstory_dlg.get_target(), chyron_settings, lane_manager));
            }

            ChyronPointer chyron = stories[story_id];

            // assign a new Reporter to cover the local story

            int content_lines;
            bool limit_content = addstory_dlg.get_limit_content(content_lines);

            ProducerPointer producer(new Producer(addstory_dlg.get_reporter(),
                                                  addstory_dlg.get_target(),
                                                  headline_font,
                                                  headline_stylesheet_normal,
                                                  headline_stylesheet_alert,
                                                  headline_alert_keywords,
                                                  addstory_dlg.get_trigger(),
                                                  limit_content,
                                                  content_lines,
                                                  this));
            if(producer->start_covering_story())
            {
                producers.push_back(producer);
                connect(producer.data(), &Producer::signal_new_headline, chyron.data(), &Chyron::slot_file_headline);
            }
            else
            {
                stories.remove(story_id);
                QMessageBox::critical(0,
                                      tr("Newsroom: Error"),
                                      tr("The Reporter could not cover the Story!"));
            }

            addstory_dlg.save_defaults(settings);
        }

        save_window_data(&addstory_dlg);
    }

    if(accept)
        event->acceptProposedAction();
}

void MainWindow::save_window_data(QWidget* window)
{
    if(!window_geometry_save_enabled)
        return;

    QString key = window->windowTitle();
    window_data[key] = window->saveGeometry();

    settings_modified = true;
}

void MainWindow::restore_window_data(QWidget* window)
{
    QString key = window->windowTitle();
    if(window_data.find(key) == window_data.end())
        return;

    window->restoreGeometry(window_data[key]);
}

void MainWindow::slot_quit()
{
    if(settings_modified)
        save_application_settings();

    qApp->quit();
}

void MainWindow::build_tray_menu()
{
    if(trayIconMenu)
    {
        disconnect(trayIconMenu, &QMenu::triggered, this, &MainWindow::slot_menu_action);
        delete trayIconMenu;
    }

//    options_action  = new QAction(QIcon(":/images/Options.png"), tr("Edit &Settings..."), this);
//    if(note_clipboard)
//        paste_action = new QAction(QIcon(":/images/Restore.png"), tr("&Paste Note"), this);
    about_action    = new QAction(QIcon(":/images/About.png"), tr("&About"), this);
    quit_action     = new QAction(QIcon(":/images/Quit.png"), tr("&Quit"), this);

    trayIconMenu = new QMenu(this);

//    trayIconMenu->addAction(options_action);
//    trayIconMenu->addSeparator();
//    if(note_clipboard)
//    {
//        trayIconMenu->addAction(paste_action);
//        trayIconMenu->addSeparator();
//    }
    trayIconMenu->addAction(about_action);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quit_action);

    trayIcon->setContextMenu(trayIconMenu);
    connect(trayIconMenu, &QMenu::triggered, this, &MainWindow::slot_menu_action);
}

void MainWindow::save_application_settings()
{
//    QString settings_file_name;
//    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
//    QSettings settings(settings_file_name, QSettings::IniFormat);

//    settings.clear();

//    settings.setValue("next_tab_icon", next_tab_icon);
//    settings.setValue("backup_database", backup_database);
//    settings.setValue("max_backups", max_backups);
//    settings.setValue("add_hook_key", QChar(add_hook_key));
//    settings.setValue("start_automatically", start_automatically);
//    settings.setValue("selected_opacity", selected_opacity);
//    settings.setValue("unselected_opacity", unselected_opacity);
//    settings.setValue("favored_side", favored_side);

//    settings.setValue("clipboard_ttl", clipboard_ttl);
//    settings.setValue("clipboard_ttl_timeout", clipboard_ttl_timeout);

//    settings.setValue("enable_sound_effects", enable_sound_effects);
//    settings.beginWriteArray("sound_files");
//      quint32 item_index = 0;
//      foreach(QString key, sound_files)
//      {
//          settings.setArrayIndex(item_index++);
//          settings.setValue(QString("sound_file_%1").arg(item_index), sound_files[item_index-1]);
//      }
//    settings.endArray();

    settings->setValue("auto_start", false);
    settings->setValue("chyron.font", headline_font.toString());
    settings->setValue("chyron.stacking", reportstacking_vec.indexOf(chyron_stacking));

    settings->setValue("settings.headline.stylesheet.normal", headline_stylesheet_normal);
    settings->setValue("settings.headline.stylesheet.alert", headline_stylesheet_alert);
    settings->setValue("settings.headline.alert.keywords", headline_alert_keywords);

    if(window_data.size())
    {
        settings->beginWriteArray("window_data");
          quint32 item_index = 0;
          QList<QString> keys = window_data.keys();
          foreach(QString key, keys)
          {
              settings->setArrayIndex(item_index++);

              settings->setValue("key", key);
              settings->setValue("geometry", window_data[key]);
          }
        settings->endArray();
    }

    settings->sync();

    settings_modified = false;
}

void MainWindow::load_application_settings()
{
    window_data.clear();

//    QString settings_file_name;
//    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
//    QSettings settings(settings_file_name, QSettings::IniFormat);

    (void)settings->value("auto_start", false).toBool();
    QFont f = ui->label->font();
    QString font_str = settings->value("chyron.font", QString()).toString();
    if(!font_str.isEmpty())
        headline_font.fromString(font_str);
    chyron_stacking = reportstacking_vec[settings->value("chyron.stacking", 0).toInt()];

    headline_stylesheet_normal = settings->value("settings.headline.stylesheet.normal", "color: rgb(255, 255, 255); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 50, 255), stop:1 rgba(0, 0, 255, 255)); border: 1px solid black; border-radius: 10px;").toString();
    headline_stylesheet_alert = settings->value("settings.headline.stylesheet.alert", "color: rgb(255, 255, 255); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(50, 0, 0, 255), stop:1 rgba(255, 0, 0, 255)); border: 1px solid black; border-radius: 10px;").toString();
    headline_alert_keywords = settings->value("settings.headline.alert.keywords", QStringList()).toStringList();

    int windata_size = settings->beginReadArray("window_data");
    if(windata_size)
    {
        for(int i = 0; i < windata_size; ++i)
        {
            settings->setArrayIndex(i);

            QString key = settings->value("key").toString();
            window_data[key] = settings->value("geometry").toByteArray();
        }
    }
    settings->endArray();
    settings_modified = !QFile::exists(settings_file_name);
}

void MainWindow::slot_icon_activated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
        case QSystemTrayIcon::DoubleClick:
//            ui->tab_Main->setCurrentIndex(MAINTAB_SETTINGS);
            slot_restore();
            break;

        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::MiddleClick:
        default:
            break;
    }
}

void MainWindow::slot_message_clicked()
{
    QMessageBox::information(0,
                             tr("Dashboard"),
                             tr("Sorry, I already gave what help I could.\n"
                                "Maybe you could try asking a human?"));
}

void MainWindow::slot_menu_action(QAction* action)
{
//    if(action == options_action)
//    {
//        slot_edit_options();
//    }
//    else
    if(action == about_action)
    {
//        slot_about();
    }
    else if(action == quit_action)
    {
        slot_quit();
    }
}

void MainWindow::slot_restore()
{
    restore_window_data(this);
    showNormal();
    activateWindow();
    raise();
}

void MainWindow::slot_edit_settings(bool /*checked*/)
{
    SettingsDialog dlg;

    dlg.set_autostart(false);
    dlg.set_font(headline_font);
    dlg.set_normal_stylesheet(headline_stylesheet_normal);
    dlg.set_alert_stylesheet(headline_stylesheet_alert);
    dlg.set_alert_keywords(headline_alert_keywords);
    dlg.set_stacking(chyron_stacking);
    dlg.set_stories(stories.keys());

    restore_window_data(&dlg);

    if(dlg.exec() == QDialog::Accepted)
    {
        (void)dlg.get_autostart();
        headline_font               = dlg.get_font();
        chyron_stacking             = dlg.get_stacking();
        headline_stylesheet_normal  = dlg.get_normal_stylesheet();
        headline_stylesheet_alert   = dlg.get_alert_stylesheet();
        headline_alert_keywords     = dlg.get_alert_keywords();
        QList<QString> remaining_stories = dlg.get_stories();

        if(remaining_stories.count() != stories.count())
        {
            // stories have been deleted

            QList<QString> deleted_stories;
            foreach(const QString& story, stories.keys())
            {
                if(!remaining_stories.contains(story))
                    deleted_stories.append(story);
            }

            foreach(const QString& story, deleted_stories)
                stories.remove(story);
        }

        save_application_settings();
    }

    save_window_data(&dlg);
}
