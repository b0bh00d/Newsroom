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

    load_application_settings();

    setAcceptDrops(true);

    lane_manager = LaneManagerPointer(new LaneManager(this));

    QDesktopWidget* desktop = QApplication::desktop();

    addlocal_dlg = new AddLocalDialog(this);
    addlocal_dlg->set_trigger(LocalTrigger::NewContent);
    addlocal_dlg->set_display(desktop->primaryScreen(), desktop->screenCount());
    addlocal_dlg->set_headlines_always_visible(true);
    addlocal_dlg->set_headlines_lock_size();
    addlocal_dlg->set_headlines_fixed_text();
    addlocal_dlg->set_animation_entry_and_exit(AnimEntryType::SlideDownLeftTop, AnimExitType::SlideLeft);

    // Load all the available plug-ins
    load_plugin_metadata();

//    if(plugins_map.isEmpty())
//        // we can't do much without reporters

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
    addlocal_dlg->deleteLater();
    delete ui;
}

void MainWindow::load_plugin_metadata()
{
    plugins_map.clear();

    QDir plugins("plugins");
    QStringList plugins_list = plugins.entryList(QStringList() << "*.dll" << "*.so", QDir::Files);
    foreach(const QString& filename, plugins_list)
    {
        QString plugin_path = QDir::toNativeSeparators(QString("%1/%2").arg(plugins.absolutePath()).arg(filename));
        QPluginLoader plugin(plugin_path);
        QObject* instance = plugin.instance();
        if(instance)
        {
            PluginInfo pi_info;
            pi_info.path = plugin_path;

            IPluginLocal* iLocal = qobject_cast<IPluginLocal *>(instance);
            if(iLocal)
            {
                if(!plugins_map.contains("Local"))
                    plugins_map["Local"] = PluginsInfoVector();

                pi_info.display = iLocal->DisplayName();
                pi_info.id = iLocal->PluginID();
                plugins_map["Local"].push_back(pi_info);
            }
            else
            {
                IPluginREST* iREST= qobject_cast<IPluginREST *>(instance);
                if(iREST)
                {
                    if(!plugins_map.contains("REST"))
                        plugins_map["REST"] = PluginsInfoVector();

                    pi_info.display = iREST->DisplayName();
                    pi_info.id = iREST->PluginID();
                    plugins_map["REST"].push_back(pi_info);
                }
            }
        }

        plugin.unload();
    }
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
        if(story.isLocalFile())
        {
            // if we have no Local plug-ins, we can't process this
            if(!plugins_map.contains("Local"))
                // post an error message
                ;
            else
            {
                text = story.toLocalFile();

                addlocal_dlg->set_target(text);
                addlocal_dlg->set_reporters(plugins_map["Local"]);
//                dlg.set_train_age_effects();

                restore_window_data(addlocal_dlg);

                if(addlocal_dlg->exec() == QDialog::Accepted)
                {
                    accept = true;

                    Chyron::Settings chyron_settings;

                    chyron_settings.entry_type = addlocal_dlg->get_animation_entry_type();

                    // see if this story is already being covered by a Reporter
                    if(stories.find(story) == stories.end())
                    {
                        int width, height;
                        bool use_fixed = addlocal_dlg->get_headlines_lock_size(width, height);

                        chyron_settings.ttl                   = addlocal_dlg->get_ttl();
                        chyron_settings.display               = addlocal_dlg->get_display();
                        chyron_settings.always_visible        = addlocal_dlg->get_headlines_always_visible();
                        chyron_settings.exit_type             = addlocal_dlg->get_animation_exit_type();
                        chyron_settings.stacking_type         = chyron_stacking;
                        chyron_settings.headline_fixed_width  = use_fixed ? width : 0;
                        chyron_settings.headline_fixed_height = use_fixed ? height : 0;
                        chyron_settings.headline_fixed_text   = addlocal_dlg->get_headlines_fixed_text();
                        chyron_settings.effect                = addlocal_dlg->get_train_age_effects(chyron_settings.train_reduce_opacity);

                        stories[story] = ChyronPointer(new Chyron(story, chyron_settings, lane_manager));
                    }

                    ChyronPointer chyron = stories[story];

                    // assign a new Reporter to cover the the story
                    ReporterPointer reporter(new ReporterWrapper(addlocal_dlg->get_reporter(),
                                                                 story,
                                                                 headline_font,
                                                                 headline_stylesheet_normal,
                                                                 headline_stylesheet_alert,
                                                                 headline_alert_keywords,
                                                                 addlocal_dlg->get_trigger(),
                                                                 this));
                    reporters.push_back(reporter);
                    connect(reporter.data(), &ReporterWrapper::signal_new_headline, chyron.data(), &Chyron::slot_file_headline);

                    reporter->start_covering_story();
                }

                save_window_data(addlocal_dlg);
            }
        }
        else
        {
            // if we have no REST plug-ins, we can't process this
            if(!plugins_map.contains("REST"))
                // post an error message
                ;
            else
            {
                text = story.toString();
            }
        }
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
    QString settings_file_name;
    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    QSettings settings(settings_file_name, QSettings::IniFormat);

    settings.clear();

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

    settings.setValue("auto_start", false);
    settings.setValue("chyron.font", headline_font.toString());
    settings.setValue("chyron.stacking", reportstacking_vec.indexOf(chyron_stacking));

    settings.setValue("settings.headline.stylesheet.normal", headline_stylesheet_normal);
    settings.setValue("settings.headline.stylesheet.alert", headline_stylesheet_alert);
    settings.setValue("settings.headline.alert.keywords", headline_alert_keywords);

    if(window_data.size())
    {
        settings.beginWriteArray("window_data");
          quint32 item_index = 0;
          QList<QString> keys = window_data.keys();
          foreach(QString key, keys)
          {
              settings.setArrayIndex(item_index++);

              settings.setValue("key", key);
              settings.setValue("geometry", window_data[key]);
          }
        settings.endArray();
    }

    settings_modified = false;
}

void MainWindow::load_application_settings()
{
    window_data.clear();

    QString settings_file_name;
    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    QSettings settings(settings_file_name, QSettings::IniFormat);

    (void)settings.value("auto_start", false).toBool();
    QFont f = ui->label->font();
    QString font_str = settings.value("chyron.font", QString()).toString();
    if(!font_str.isEmpty())
        headline_font.fromString(font_str);
    chyron_stacking = reportstacking_vec[settings.value("chyron.stacking", 0).toInt()];

    headline_stylesheet_normal = settings.value("settings.headline.stylesheet.normal", "color: rgb(255, 255, 255); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 50, 255), stop:1 rgba(0, 0, 255, 255)); border: 1px solid black; border-radius: 10px;").toString();
    headline_stylesheet_alert = settings.value("settings.headline.stylesheet.alert", "color: rgb(255, 255, 255); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(50, 0, 0, 255), stop:1 rgba(255, 0, 0, 255)); border: 1px solid black; border-radius: 10px;").toString();
    headline_alert_keywords = settings.value("settings.headline.alert.keywords", QStringList()).toStringList();

    int windata_size = settings.beginReadArray("window_data");
    if(windata_size)
    {
        for(int i = 0; i < windata_size; ++i)
        {
            settings.setArrayIndex(i);

            QString key = settings.value("key").toString();
            window_data[key] = settings.value("geometry").toByteArray();
        }
    }
    settings.endArray();

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
        QList<QUrl> remaining_stories = dlg.get_stories();

        if(remaining_stories.count() != stories.count())
        {
            // stories have been deleted

            QList<QUrl> deleted_stories;
            foreach(const QUrl& story, stories.keys())
            {
                if(!remaining_stories.contains(story))
                    deleted_stories.append(story);
            }

            foreach(const QUrl& story, deleted_stories)
                stories.remove(story);

            // TODO: adjust stacking orders
        }

        save_application_settings();
    }

    save_window_data(&dlg);
}
