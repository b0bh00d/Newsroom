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
#include "settings.h"

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
    default_stylesheet = "color: rgb(255, 255, 255); background-color: rgb(75, 75, 75); border: 1px solid black;";

//    QPixmap bkgnd(":/images/Add.png");
//    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
//    QPalette palette;
//    palette.setBrush(QPalette::Background, bkgnd);
//    this->setPalette(palette);

    setWindowTitle(tr("Newsroom by Bob Hood"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));

    headline_style_list = StyleListPointer(new HeadlineStyleList());

    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom.xml").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    settings = SettingsPointer(new Settings("Newsroom", settings_file_name));
    settings->cache();

    load_application_settings();

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
    settings->begin_section("/StoryDefaults");

    story_info->story                    = settings->get_item("story", QUrl()).toUrl();
    story_info->identity                 = settings->get_item("identity", QString()).toString();
    story_info->reporter_class           = settings->get_item("reporter_class", QString()).toString();
    story_info->reporter_id              = settings->get_item("reporter_id", QString()).toString();
    story_info->reporter_parameters      = settings->get_item("reporter_parameters", QStringList()).toStringList();
    story_info->ttl                      = settings->get_item("ttl", story_info->ttl).toInt();
    story_info->primary_screen           = settings->get_item("primary_screen", 0).toInt();
    story_info->headlines_always_visible = settings->get_item("headlines_always_visible", true).toBool();
    story_info->interpret_as_pixels      = settings->get_item("interpret_as_pixels", true).toBool();
    story_info->headlines_pixel_width    = settings->get_item("headlines_pixel_width", 0).toInt();
    story_info->headlines_pixel_height   = settings->get_item("headlines_pixel_height", 0).toInt();
    story_info->headlines_percent_width  = settings->get_item("headlines_percent_width", 0.0).toDouble();
    story_info->headlines_percent_height = settings->get_item("headlines_percent_height", 0.0).toDouble();
    story_info->limit_content_to         = settings->get_item("limit_content_to", 0).toInt();
    story_info->headlines_fixed_type     = static_cast<FixedText>(settings->get_item("headlines_fixed_type", 0).toInt());
    story_info->include_progress_bar     = settings->get_item("include_progress_bar", false).toBool();
    story_info->progress_text_re         = settings->get_item("progress_text_re", story_info->progress_text_re).toString();
    story_info->progress_on_top          = settings->get_item("progress_on_top", false).toBool();
    story_info->entry_type               = static_cast<AnimEntryType>(settings->get_item("entry_type", 0).toInt());
    story_info->exit_type                = static_cast<AnimExitType>(settings->get_item("exit_type", 0).toInt());
    story_info->anim_motion_duration     = settings->get_item("anim_motion_duration", story_info->anim_motion_duration).toInt();
    story_info->fade_target_duration     = settings->get_item("fade_target_duration", story_info->fade_target_duration).toInt();
    story_info->train_use_age_effect     = settings->get_item("train_use_age_effects", false).toBool();
    story_info->train_age_effect         = static_cast<AgeEffects>(settings->get_item("train_age_effect", 0).toInt());
    story_info->train_age_percent        = settings->get_item("train_age_percent", story_info->train_age_percent).toInt();
    story_info->dashboard_use_age_effect = settings->get_item("dashboard_use_age_effects", false).toBool();
    story_info->dashboard_age_percent    = settings->get_item("dashboard_age_percent", story_info->dashboard_age_percent).toInt();
    story_info->dashboard_group_id       = settings->get_item("dashboard_group_id", QString()).toString();

    // Chyron settings
    story_info->stacking_type            = static_cast<ReportStacking>(settings->get_item("stacking_type", static_cast<int>(chyron_stacking)).toInt());
    story_info->margin                   = settings->get_item("margins", story_info->margin).toInt();

    // Producer settings
    story_info->font.fromString(settings->get_item("font", headline_font.toString()).toString());

    settings->end_section();
}

void MainWindow::save_story_defaults(StoryInfoPointer story_info)
{
    settings->begin_section("/StoryDefaults");

    settings->set_item("story", story_info->story);
    settings->set_item("identity", story_info->identity);
    settings->set_item("reporter_class", story_info->reporter_class);
    settings->set_item("reporter_id", story_info->reporter_id);
    settings->set_item("reporter_parameters", story_info->reporter_parameters);
    settings->set_item("ttl", story_info->ttl);
    settings->set_item("primary_screen", story_info->primary_screen);
    settings->set_item("headlines_always_visible", story_info->headlines_always_visible);
    settings->set_item("interpret_as_pixels", story_info->interpret_as_pixels);
    settings->set_item("headlines_pixel_width", story_info->headlines_pixel_width);
    settings->set_item("headlines_pixel_height", story_info->headlines_pixel_height);
    settings->set_item("headlines_percent_width", story_info->headlines_percent_width);
    settings->set_item("headlines_percent_height", story_info->headlines_percent_height);
    settings->set_item("limit_content_to", story_info->limit_content_to);
    settings->set_item("headlines_fixed_type", static_cast<int>(story_info->headlines_fixed_type));
    settings->set_item("include_progress_bar", story_info->include_progress_bar);
    settings->set_item("progress_text_re", story_info->progress_text_re);
    settings->set_item("progress_on_top", story_info->progress_on_top);
    settings->set_item("entry_type", static_cast<int>(story_info->entry_type));
    settings->set_item("exit_type", static_cast<int>(story_info->exit_type));
    settings->set_item("anim_motion_duration", story_info->anim_motion_duration);
    settings->set_item("fade_target_duration", story_info->fade_target_duration);
    settings->set_item("train_use_age_effects", story_info->train_use_age_effect);
    settings->set_item("train_age_effect", static_cast<int>(story_info->train_age_effect));
    settings->set_item("train_age_percent", story_info->train_age_percent);
    settings->set_item("dashboard_use_age_effects", story_info->dashboard_use_age_effect);
    settings->set_item("dashboard_age_percent", story_info->dashboard_age_percent);
    settings->set_item("dashboard_group_id", story_info->dashboard_group_id);

    // Chyron settings
    settings->set_item("stacking_type", static_cast<int>(story_info->stacking_type));
    settings->set_item("margin", story_info->margin);

    // Producer settings
    settings->set_item("font", story_info->font.toString());

    settings->end_section();

    settings_modified = true;
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
            story_info->reporter_class = "REST";

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
    settings->begin_section("/Application");

    settings->set_item("auto_start", false);
    settings->set_item("chyron.font", headline_font.toString());
    settings->set_item("chyron.stacking", reportstacking_vec.indexOf(chyron_stacking));

    settings->begin_array("HeadlineStyles");
    quint32 item_index = 0;
    foreach(const HeadlineStyle& style, *(headline_style_list.data()))
    {
        settings->set_array_item(item_index, "name", style.name);
        settings->set_array_item(item_index, "triggers", style.triggers);
        settings->set_array_item(item_index++, "stylesheet", style.stylesheet);
    }

    settings->end_array();

    if(window_data.size())
    {
        settings->begin_array("WindowData");
          item_index = 0;
          QList<QString> keys = window_data.keys();
          foreach(QString key, keys)
          {
              settings->set_array_item(item_index, "key", key);
              settings->set_array_item(item_index++, "geometry", window_data[key]);
          }
        settings->end_array();
    }

    settings->end_section();

    settings->flush();

    settings_modified = false;
}

void MainWindow::load_application_settings()
{
    window_data.clear();

    headline_style_list->clear();
    // add a Default stylesheet entry on first runs
    HeadlineStyle hs;
    hs.name = "Default";
    hs.stylesheet = default_stylesheet;//"color: rgb(255, 255, 255); background-color: rgb(75, 75, 75); border: 1px solid black;";
    headline_style_list->append(hs);

    settings->begin_section("/Application");

    (void)settings->get_item("auto_start", false).toBool();
    QFont f = ui->label->font();
    QString font_str = settings->get_item("chyron.font", f.toString()).toString();
    if(!font_str.isEmpty())
        headline_font.fromString(font_str);
    chyron_stacking = reportstacking_vec[settings->get_item("chyron.stacking", 0).toInt()];

    int styles_size = settings->begin_array("HeadlineStyles");
    if(styles_size)
    {
        for(int i = 0; i < styles_size; ++i)
        {
            HeadlineStyle hs;
            hs.name = settings->get_array_item(i, "name").toString();
            hs.triggers = settings->get_array_item(i, "triggers").toStringList();
            hs.stylesheet = settings->get_array_item(i, "stylesheet").toString();

            bool updated = false;
            for(HeadlineStyleList::iterator iter = headline_style_list->begin();
                iter != headline_style_list->end();++iter)
            {
                if(!iter->name.compare(hs.name))
                {
                    iter->triggers = hs.triggers;
                    iter->stylesheet = hs.stylesheet;
                    updated = true;
                    break;
                }
            }

            if(!updated)
                headline_style_list->append(hs);
        }
    }
    settings->end_array();

    int windata_size = settings->begin_array("WindowData");
    if(windata_size)
    {
        for(int i = 0; i < windata_size; ++i)
        {
            QString key = settings->get_array_item(i, "key").toString();
            window_data[key] = settings->get_array_item(i, "geometry").toByteArray();
        }
    }
    settings->end_array();

    settings->end_section();

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
    dlg.set_styles(*(headline_style_list.data()));
    dlg.set_stacking(chyron_stacking);
    dlg.set_stories(story_identities);

    restore_window_data(&dlg);

    if(dlg.exec() == QDialog::Accepted)
    {
        (void)dlg.get_autostart();
        headline_font                    = dlg.get_font();
        chyron_stacking                  = dlg.get_stacking();
        QList<QString> remaining_stories = dlg.get_stories();

        dlg.get_styles(*(headline_style_list.data()));

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
