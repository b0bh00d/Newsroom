#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMessageBox>

#include <QtGui/QFontMetrics>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

#include "types.h"
#include "storyinfo.h"
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

MainWindow* mainwindow;

MainWindow::MainWindow(QWidget *parent)
    : trayIconMenu(0),
      window_geometry_save_enabled(true),
      start_automatically(false),
      settings_modified(false),
      chyron_stacking(ReportStacking::Stacked),
      QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    mainwindow = this;

    ui->setupUi(this);

    headline_font = ui->label->font();

//    QPixmap bkgnd(":/images/Add.png");
//    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
//    QPalette palette;
//    palette.setBrush(QPalette::Background, bkgnd);
//    this->setPalette(palette);

    setWindowTitle(tr("Newsroom by Bob Hood"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));

    headline_style_list = StyleListPointer(new HeadlineStyleList());

    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    settings = SettingsPointer(new QSettings(settings_file_name, QSettings::IniFormat));

    settings_root = new QTreeWidgetItem(0);

    load_application_settings();

    if(headline_style_list->isEmpty())
    {
        // add a Default stylesheet entry on first runs
        HeadlineStyle hs;
        hs.name = "Default";
        hs.stylesheet = "color: rgb(255, 255, 255); background-color: rgb(75, 75, 75); border: 1px solid black;";
        headline_style_list->append(hs);
    }

    setAcceptDrops(true);

    lane_manager = LaneManagerPointer(new LaneManager(headline_font, (*headline_style_list.data())[0].stylesheet, this));

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

            IReporterFactory* ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
            if(ireporterfactory)
            {
                IReporterPointer ireporter = ireporterfactory->newInstance();

                QStringList display = ireporter->DisplayName();
                pi_info.name = display[0];
                pi_info.tooltip = display[1];
                pi_info.id = ireporter->PluginID();

                QString pi_class = ireporter->PluginClass();

                if(!plugins_map.contains(pi_class))
                    plugins_map[pi_class] = PluginsInfoVector();
                plugins_map[pi_class].push_back(pi_info);
            }
        }
    }

    return plugins_map.count() > 0;
}

void MainWindow::restore_story_defaults(StoryInfoPointer story_info)
{
    settings->beginGroup("StoryDefaults");

    story_info->story                    = settings->value("story", QUrl()).toUrl();
    story_info->identity                 = settings->value("identity", QString()).toString();
    story_info->reporter_class           = settings->value("reporter_class", QString()).toString();
    story_info->reporter_id              = settings->value("reporter_id", QString()).toString();
    story_info->reporter_parameters      = settings->value("reporter_parameters", QStringList()).toStringList();
    story_info->trigger_type             = static_cast<LocalTrigger>(settings->value("local_trigger", 0).toInt());
    story_info->ttl                      = settings->value("ttl", story_info->ttl).toInt();
    story_info->primary_screen           = settings->value("primary_screen", 0).toInt();
    story_info->headlines_always_visible = settings->value("headlines_always_visible", true).toBool();
    story_info->interpret_as_pixels      = settings->value("interpret_as_pixels", true).toBool();
    story_info->headlines_pixel_width    = settings->value("headlines_pixel_width", 0).toInt();
    story_info->headlines_pixel_height   = settings->value("headlines_pixel_height", 0).toInt();
    story_info->headlines_percent_width  = settings->value("headlines_percent_width", 0.0).toDouble();
    story_info->headlines_percent_height = settings->value("headlines_percent_height", 0.0).toDouble();
    story_info->limit_content_to         = settings->value("limit_content_to", 0).toInt();
    story_info->headlines_fixed_type     = static_cast<FixedText>(settings->value("headlines_fixed_type", 0).toInt());
    story_info->entry_type               = static_cast<AnimEntryType>(settings->value("entry_type", 0).toInt());
    story_info->exit_type                = static_cast<AnimExitType>(settings->value("exit_type", 0).toInt());
    story_info->anim_motion_duration     = settings->value("anim_motion_duration", story_info->anim_motion_duration).toInt();
    story_info->fade_target_duration     = settings->value("fade_target_duration", story_info->fade_target_duration).toInt();
    story_info->train_use_age_effect     = settings->value("train_use_age_effects", false).toBool();
    story_info->train_age_effect         = static_cast<AgeEffects>(settings->value("train_age_effect", 0).toInt());
    story_info->train_age_percent        = settings->value("train_age_percent", story_info->train_age_percent).toInt();
    story_info->dashboard_use_age_effect = settings->value("dashboard_use_age_effects", false).toBool();
    story_info->dashboard_age_percent    = settings->value("dashboard_age_percent", story_info->dashboard_age_percent).toInt();
    story_info->dashboard_group_id       = settings->value("dashboard_group_id", QString()).toString();

    // Chyron settings
    story_info->stacking_type            = static_cast<ReportStacking>(settings->value("stacking_type", static_cast<int>(chyron_stacking)).toInt());
    story_info->margin                   = settings->value("margins", story_info->margin).toInt();

    // Producer settings
    story_info->font.fromString(settings->value("font", headline_font.toString()).toString());
    story_info->normal_stylesheet        = settings->value("normal_stylesheet", headline_stylesheet_normal).toString();
    story_info->alert_stylesheet         = settings->value("alert_stylesheet", headline_stylesheet_alert).toString();
    story_info->alert_keywords           = settings->value("alert_keywords", headline_alert_keywords).toStringList();

    settings->endGroup();
}

void MainWindow::save_story_defaults(StoryInfoPointer story_info)
{
    settings->beginGroup("StoryDefaults");

    settings->setValue("story", story_info->story);
    settings->setValue("identity", story_info->identity);
    settings->setValue("reporter_class", story_info->reporter_class);
    settings->setValue("reporter_id", story_info->reporter_id);
    settings->setValue("reporter_parameters", story_info->reporter_parameters);
    settings->setValue("local_trigger", static_cast<int>(story_info->trigger_type));
    settings->setValue("ttl", story_info->ttl);
    settings->setValue("primary_screen", story_info->primary_screen);
    settings->setValue("headlines_always_visible", story_info->headlines_always_visible);
    settings->setValue("interpret_as_pixels", story_info->interpret_as_pixels);
    settings->setValue("headlines_pixel_width", story_info->headlines_pixel_width);
    settings->setValue("headlines_pixel_height", story_info->headlines_pixel_height);
    settings->setValue("headlines_percent_width", story_info->headlines_percent_width);
    settings->setValue("headlines_percent_height", story_info->headlines_percent_height);
    settings->setValue("limit_content_to", story_info->limit_content_to);
    settings->setValue("headlines_fixed_type", static_cast<int>(story_info->headlines_fixed_type));
    settings->setValue("entry_type", static_cast<int>(story_info->entry_type));
    settings->setValue("exit_type", static_cast<int>(story_info->exit_type));
    settings->setValue("anim_motion_duration", story_info->anim_motion_duration);
    settings->setValue("fade_target_duration", story_info->fade_target_duration);
    settings->setValue("train_use_age_effects", story_info->train_use_age_effect);
    settings->setValue("train_age_effect", static_cast<int>(story_info->train_age_effect));
    settings->setValue("train_age_percent", story_info->train_age_percent);
    settings->setValue("dashboard_use_age_effects", story_info->dashboard_use_age_effect);
    settings->setValue("dashboard_age_percent", story_info->dashboard_age_percent);
    settings->setValue("dashboard_group_id", story_info->dashboard_group_id);

    // Chyron settings
    settings->setValue("stacking_type", static_cast<int>(story_info->stacking_type));
    settings->setValue("margin", story_info->margin);

    // Producer settings
    settings->setValue("font", story_info->font.toString());
    settings->setValue("normal_stylesheet", story_info->normal_stylesheet);
    settings->setValue("alert_stylesheet", story_info->alert_stylesheet);
    settings->setValue("alert_keywords", story_info->alert_keywords);

    settings->endGroup();
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

    // each element in the list has already been vetted
    // in dragEnterEvent() above

    QList<QUrl> urls = event->mimeData()->urls();
    foreach(const QUrl& story, urls)
    {
        PluginsInfoVector* reporters_info = nullptr;

        StoryInfoPointer story_info = StoryInfoPointer(new StoryInfo());
        restore_story_defaults(story_info);
        story_info->story = story;
        story_info->identity.clear();

        if(story.isLocalFile())
            story_info->reporter_class = "Local";
        else
            story_info->reporter_class = "URL";

        if(plugins_map.contains(story_info->reporter_class))
            reporters_info = &plugins_map[story_info->reporter_class];

        if(!reporters_info)
        {
            QMessageBox::critical(0,
                                  tr("Newsroom: Error"),
                                  tr("No Reporter plug-ins are available to cover that Story!"));
            return;
        }

        AddStoryDialog addstory_dlg(reporters_info, story_info, settings, this);

        restore_window_data(&addstory_dlg);

        if(addstory_dlg.exec() == QDialog::Accepted)
        {
            // AddStoryDialog has automatically updated 'story_info' with all settings
            save_story_defaults(story_info);

            StaffInfo staff_info;

            accept = true;

            // create a Chyron to display the Headlines for this story

            staff_info.chyron = ChyronPointer(new Chyron(story_info, lane_manager));

            // assign a staff Reporter from the selected department to cover the story

            foreach(const PluginInfo& pi_info, (*reporters_info))
            {
                QObject* instance = pi_info.factory->instance();
                IReporterFactory* ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
                Q_ASSERT(ireporterfactory);
                IReporterPointer plugin_reporter = ireporterfactory->newInstance();
                if(!story_info->reporter_id.compare(plugin_reporter->PluginID()))
                {
                    staff_info.reporter = plugin_reporter;
                    staff_info.reporter->SetRequirements(story_info->reporter_parameters);
                    break;
                }
            }

            // assign a staff Producer to receive Reporter filings and create headlines

            staff_info.producer = ProducerPointer(new Producer(staff_info.reporter, story_info, headline_style_list, this));
            if(staff_info.producer->start_covering_story())
            {
                connect(staff_info.producer.data(), &Producer::signal_new_headline,
                        staff_info.chyron.data(), &Chyron::slot_file_headline);

                staff[story_info] = staff_info;
            }
            else
            {
                QMessageBox::critical(0,
                                      tr("Newsroom: Error"),
                                      tr("The Reporter could not cover the Story!"));
            }
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
return;
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

    settings->beginGroup("General");

    settings->setValue("auto_start", false);
    settings->setValue("chyron.font", headline_font.toString());
    settings->setValue("chyron.stacking", reportstacking_vec.indexOf(chyron_stacking));

//    settings->setValue("settings.headline.stylesheet.normal", headline_stylesheet_normal);
//    settings->setValue("settings.headline.stylesheet.alert", headline_stylesheet_alert);
//    settings->setValue("settings.headline.alert.keywords", headline_alert_keywords);

    settings->beginWriteArray("HeadlineStyles");
    quint32 item_index = 0;
    foreach(const HeadlineStyle& style, headline_styles)
    {
        settings->setArrayIndex(item_index++);
        settings->setValue("name", style.name);
        settings->setValue("triggers", style.triggers);
        settings->setValue("stylesheet", style.stylesheet);
    }

    settings->endArray();

//    settings->endGroup();

//    settings->beginGroup("Application");

    if(window_data.size())
    {
        settings->beginWriteArray("WindowData");
          item_index = 0;
          QList<QString> keys = window_data.keys();
          foreach(QString key, keys)
          {
              settings->setArrayIndex(item_index++);

              settings->setValue("key", key);
              settings->setValue("geometry", window_data[key]);
          }
        settings->endArray();
    }

    settings->endGroup();

//    settings->sync();

    settings_modified = false;
}

void MainWindow::load_application_settings()
{
    window_data.clear();
    headline_styles.clear();

return;
    settings->beginGroup("General");

//    QString settings_file_name;
//    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.ini").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
//    QSettings settings(settings_file_name, QSettings::IniFormat);

    (void)settings->value("auto_start", false).toBool();
    QFont f = ui->label->font();
    QString font_str = settings->value("chyron.font", f.toString()).toString();
    if(!font_str.isEmpty())
        headline_font.fromString(font_str);
    chyron_stacking = reportstacking_vec[settings->value("chyron.stacking", 0).toInt()];

    int styles_size = settings->beginReadArray("HeadlineStyles");
    if(styles_size)
    {
        for(int i = 0; i < styles_size; ++i)
        {
            settings->setArrayIndex(i);

            HeadlineStyle hs;
            hs.name = settings->value("name").toString();
            hs.triggers = settings->value("triggers").toStringList();
            hs.stylesheet = settings->value("triggers").toString();

            headline_styles.append(hs);
        }
    }

//    settings->endGroup();

//    settings->beginGroup("Application");

    int windata_size = settings->beginReadArray("WindowData");
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

    settings->endGroup();

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

    QList<QString> story_identities;
    foreach(StoryInfoPointer story_info, staff.keys())
        story_identities.append(story_info->identity);

    dlg.set_autostart(false);
    dlg.set_font(headline_font);
    dlg.set_styles(headline_styles);
    dlg.set_stacking(chyron_stacking);
    dlg.set_stories(story_identities);

    restore_window_data(&dlg);

    if(dlg.exec() == QDialog::Accepted)
    {
        (void)dlg.get_autostart();
        headline_font                    = dlg.get_font();
        chyron_stacking                  = dlg.get_stacking();
        QList<QString> remaining_stories = dlg.get_stories();

        dlg.get_styles(headline_styles);

        if(remaining_stories.count() != staff.count())
        {
            // stories have been deleted

            QVector<StoryInfoPointer> deleted_stories;
            foreach(const QString& story_id, story_identities)
            {
                if(!remaining_stories.contains(story_id))
                {
                    foreach(StoryInfoPointer story_info, staff.keys())
                    {
                        if(!story_info->identity.compare(story_id))
                           deleted_stories.append(story_info);
                    }
                }
            }

            foreach(StoryInfoPointer story_info, deleted_stories)
                staff.remove(story_info);
        }

        save_application_settings();
    }

    save_window_data(&dlg);
}
