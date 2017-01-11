#include <random>

#include <QtWidgets/QMessageBox>

#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>

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

#include "chyron.h"

MainWindow* mainwindow;

MainWindow::MainWindow(QWidget *parent)
    : auto_start(false),
      continue_coverage(false),
      compact_mode(false),
      compact_compression(25),
      edit_story_first_time(true),
      trayIconMenu(0),
      window_geometry_save_enabled(true),
      start_automatically(false),
      settings_modified(false),
      last_start_offset(0),
      settings_dlg(nullptr),
      QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    mainwindow = this;

    ui->setupUi(this);

    headline_font = ui->label->font();
    default_stylesheet = "color: rgb(255, 255, 255); background-color: rgb(75, 75, 75); border: 1px solid black;";

    setWindowTitle(tr("Newsroom by Bob Hood"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));
    setFixedSize(256, 256);

    background_image = PixmapPointer(new QPixmap(":/images/Newsroom256.png"));

    headline_style_list = StyleListPointer(new HeadlineStyleList());

    settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom").arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    settings = SettingsPointer(new SettingsXML("Newsroom", settings_file_name));
    settings->cache();

    // Load all the available Reporter plug-ins
    if(!load_reporters())
    {
        QMessageBox::critical(0,
                              tr("Newsroom: Error"),
                              tr("No Reporter plug-ins were found!\n"
                                 "Newsroom cannot function without Reporters."));
        QTimer::singleShot(100, qApp, &QApplication::quit);
        return;
    }

    load_application_settings();

    setAcceptDrops(true);

    // Tray

    trayIcon = new QSystemTrayIcon(this);
    bool connected = connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::slot_message_clicked);
    ASSERT_UNUSED(connected);
    connected = connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slot_icon_activated);
    ASSERT_UNUSED(connected);

    connected = connect(ui->action_Settings, &QAction::triggered, this, &MainWindow::slot_edit_settings);
    ASSERT_UNUSED(connected);

    trayIcon->setIcon(QIcon(":/images/Newsroom16.png"));
    trayIcon->setToolTip(tr("Newsroom"));

    build_tray_menu();

    trayIcon->show();

    qApp->setQuitOnLastWindowClosed(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::load_reporters()
{
    QMap<QString, bool> id_filter;

    beat_reporters.clear();

    QDir plugins("reporters");
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

                if(!id_filter.contains(pi_info.id))
                {
                    QString pi_class = ireporter->PluginClass();
                    if(!beat_reporters.contains(pi_class))
                        beat_reporters[pi_class] = PluginsInfoVector();
                    beat_reporters[pi_class].push_back(pi_info);
                }
            }
        }
    }

    return beat_reporters.count() > 0;
}

void MainWindow::set_visible(bool visible)
{
    if(visible)
        restore_window_data(this);

    QWidget::setVisible(visible);
}

void MainWindow::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    auto winSize = size();
    auto pixmapRatio = (float)background_image->width() / background_image->height();
    auto windowRatio = (float)winSize.width() / winSize.height();

    if(pixmapRatio > windowRatio)
    {
        auto newWidth = (int)(winSize.height() * pixmapRatio);
        auto offset = (newWidth - winSize.width()) / -2;
        painter.drawPixmap(offset, 0, newWidth, winSize.height(), *(background_image.data()));
    }
    else
    {
        auto newHeight = (int)(winSize.width() / pixmapRatio);
        auto offset = (newHeight - winSize.height()) / -2;
        painter.drawPixmap(0, offset, winSize.width(), newHeight, *(background_image.data()));
    }
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

//void MainWindow::fix_identity_duplication(StoryInfoPointer story_info)
//{
//    int next_dupe = 0;
//    QRegExp dupe_value("\\:\\:(\\d+)$");

//    foreach(StoryInfoPointer key, staff.keys())
//    {
//        if(key->identity.startsWith(story_info->identity))
//        {
//            QString identity = key->identity;
//            identity.remove(0, story_info->identity.length());
//            if(dupe_value.indexIn(identity) != -1)
//            {
//                int this_dupe = dupe_value.cap(1).toInt();
//                if(this_dupe > next_dupe)
//                    next_dupe = this_dupe;
//            }
//        }
//    }

//    if(next_dupe)
//        story_info->identity = QString("%1_%2").arg(story_info->identity).arg(next_dupe + 1);
//}

void MainWindow::fix_identity_duplication(StoryInfoPointer story_info)
{
    QString prefix = QString("%1::").arg(story_info->identity);
    foreach(const SeriesInfo& series_info, series_ordered)
    {
        foreach(ProducerPointer producer, series_info.producers)
        {
            StoryInfoPointer si = producer->get_story();
            if(si->identity.startsWith(prefix))
            {
                // generate a random number to append to the identity
                std::random_device rd;
                std::mt19937 mt(rd());
                std::uniform_int_distribution<> dist(INT_MAX / 2, INT_MAX);

                prefix = QString("%1::%2").arg(story_info->identity).arg(QString::number(dist(mt),16));
                story_info->identity = prefix;
                break;
            }
        }
    }
}

bool MainWindow::cover_story(ProducerPointer& producer, StoryInfoPointer story_info, CoverageStart coverage_start, const PluginsInfoVector *reporters_info)
{
    bool result = false;

    if(producer.isNull())
    {
        if(!reporters_info)
        {
            if(beat_reporters.contains(story_info->reporter_beat))
                reporters_info = &beat_reporters[story_info->reporter_beat];

            if(!reporters_info)
            {
                QMessageBox::critical(0,
                                      tr("Newsroom: Error"),
                                      tr("No Reporters are available to cover the \"%1\" beat!").arg(story_info->reporter_beat));
                return result;
            }
        }

        // create a Chyron to display the Headlines for this story

        ChyronPointer chyron = ChyronPointer(new Chyron(story_info, lane_manager));

        // assign a staff Reporter from the selected department to cover the story

        IReporterPointer reporter;
        foreach(const PluginInfo& pi_info, (*reporters_info))
        {
            QObject* instance = pi_info.factory->instance();
            IReporterFactory* ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
            Q_ASSERT(ireporterfactory);
            IReporterPointer plugin_reporter = ireporterfactory->newInstance();
            if(!story_info->reporter_id.compare(plugin_reporter->PluginID()))
            {
                reporter = plugin_reporter;
                // give the Reporter their assignment
                reporter->SetRequirements(story_info->reporter_parameters);
                break;
            }
        }

        // assign a staff Producer to receive Reporter filings and create Headlines

        producer = ProducerPointer(new Producer(chyron, reporter, story_info, headline_style_list, this));
    }

    if(coverage_start == CoverageStart::Delayed)
    {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> dist(2, std::nextafter(6, INT_MAX));

        int offset = last_start_offset + dist(mt);
        last_start_offset = offset;

        QTimer::singleShot(offset * 1000, producer.data(), &Producer::slot_start_covering_story);

        result = true;
    }
    else if(coverage_start == CoverageStart::Immediate)
    {
        if(producer->start_covering_story())
            result = true;
        else
        {
            QMessageBox::critical(0,
                                  tr("Newsroom: Error"),
                                  tr("The Reporter \"%1\" could not cover the Story!")
                                        .arg(producer->get_reporter()->DisplayName()[0]));
            producer.clear();
        }
    }
    else if(coverage_start == CoverageStart::None)
        result = true;

    return result;
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
        story_info->dashboard_compact_mode = compact_mode;
        story_info->dashboard_compression = compact_compression;

        if(story.isLocalFile())
            story_info->reporter_beat = "Local";
        else
            story_info->reporter_beat = "REST";

        if(beat_reporters.contains(story_info->reporter_beat))
            reporters_info = &beat_reporters[story_info->reporter_beat];

        if(!reporters_info)
        {
            QMessageBox::critical(0,
                                  tr("Newsroom: Error"),
                                  tr("No Reporters are available to cover the \"%1\" beat!")
                                        .arg(story_info->reporter_beat));
            return;
        }

        AddStoryDialog addstory_dlg(reporters_info, story_info, settings, this);

        restore_window_data(&addstory_dlg);

        if(addstory_dlg.exec() == QDialog::Accepted)
        {
            // AddStoryDialog has automatically updated 'story_info' with all settings
            save_story_defaults(story_info);

            // fix any duplication issues with the story identity
            fix_identity_duplication(story_info);

            // find "Default"
            for(SeriesInfoList::iterator iter = series_ordered.begin();iter != series_ordered.end();++iter)
            {
                if(!iter->name.compare("Default"))
                {
                    ProducerPointer producer;
                    if(cover_story(producer, story_info, CoverageStart::Immediate, reporters_info))
                        iter->producers.append(producer);
                    break;
                }
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
    settings_action = new QAction(QIcon(":/images/Options.png"), tr("&Settings..."), this);
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
    trayIconMenu->addAction(settings_action);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(about_action);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quit_action);

    trayIcon->setContextMenu(trayIconMenu);
    connect(trayIconMenu, &QMenu::triggered, this, &MainWindow::slot_menu_action);
}

void MainWindow::save_application_settings()
{
    settings->clear_section("/Application");

    settings->begin_section("/Application");

    settings->set_item("auto_start", auto_start);
    settings->set_item("continue_coverage", continue_coverage);
    settings->set_item("dashboard.compact_mode", compact_mode);
    settings->set_item("dashboard.compact_compression", compact_compression);
    settings->set_item("chyron.font", headline_font.toString());

    settings->begin_array("HeadlineStyles");
    quint32 index = 0;
    foreach(const HeadlineStyle& style, *(headline_style_list.data()))
    {
        settings->set_array_index(index++);

        settings->set_item("name", style.name);
        settings->set_item("triggers", style.triggers);
        settings->set_item("stylesheet", style.stylesheet);
    }

    settings->end_array();

    if(window_data.size())
    {
        settings->begin_array("WindowData");

        index = 0;
        QList<QString> keys = window_data.keys();
        foreach(QString key, keys)
        {
            settings->set_array_index(index++);

            settings->set_item("key", key);
            settings->set_item("geometry", window_data[key]);
        }

        settings->end_array();
    }

    QStringList series_names;
    foreach(const SeriesInfo& series_info, series_ordered)
    {
        series_names << series_info.name;
        save_series(series_info);
    }

    settings->set_item("series", series_names);

    settings->end_section();

    settings->flush();

    settings_modified = false;
}

void MainWindow::load_application_settings()
{
    window_data.clear();
    if(lane_manager)
        lane_manager.clear();

    // add a Default Series entry on first runs
    series_ordered.clear();

    // add a Default stylesheet entry on first runs
    headline_style_list->clear();
    HeadlineStyle hs;
    hs.name = "Default";
    hs.stylesheet = default_stylesheet;
    headline_style_list->append(hs);

    settings->begin_section("/Application");

    auto_start = settings->get_item("auto_start", false).toBool();
    continue_coverage = settings->get_item("continue_coverage", false).toBool();
    compact_mode = settings->get_item("dashboard.compact_mode", false).toBool();
    compact_compression = settings->get_item("dashboard.compact_compression", 25).toInt();
    QFont f = ui->label->font();
    QString font_str = settings->get_item("chyron.font", f.toString()).toString();
    if(!font_str.isEmpty())
        headline_font.fromString(font_str);

    int styles_size = settings->begin_array("HeadlineStyles");
    if(styles_size)
    {
        for(int i = 0; i < styles_size; ++i)
        {
            settings->set_array_index(i);

            HeadlineStyle hs;
            hs.name = settings->get_item("name").toString();
            hs.triggers = settings->get_item("triggers").toStringList();
            hs.stylesheet = settings->get_item("stylesheet").toString();

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

    lane_manager = LaneManagerPointer(new LaneManager(headline_font, (*headline_style_list.data())[0].stylesheet, this));

    int windata_size = settings->begin_array("WindowData");
    if(windata_size)
    {
        for(int i = 0; i < windata_size; ++i)
        {
            settings->set_array_index(i);

            QString key = settings->get_item("key").toString();
            window_data[key] = settings->get_item("geometry").toByteArray();
        }
    }
    settings->end_array();

    QStringList series_names = settings->get_item("series", QStringList() << "Default").toStringList();
    foreach(const QString& series_name, series_names)
    {
        SeriesInfo si;
        si.name = series_name;
        series_ordered.append(si);

        load_series(series_ordered.back());
    }

    settings->end_section();

    settings_modified = !QFile::exists(settings_file_name);
}

void MainWindow::load_series(SeriesInfo &series_info)
{
    QString series_dir = QDir::toNativeSeparators(QString("%1/Series")
                                                .arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    if(!QFile::exists(series_dir))
        return;

    QString series_file_name = QDir::toNativeSeparators(QString("%1/%2").arg(series_dir).arg(QString(QUrl::toPercentEncoding(series_info.name, "", "?*$"))));
    SettingsPointer series_settings = SettingsPointer(new SettingsXML("NewsroomSeries", series_file_name));
    series_settings->cache();

    series_settings->begin_section("/Series");

    QBitArray is_active;
    QStringList active_list = series_settings->get_item("active", QString()).toString().split(",");

    int story_count = series_settings->begin_array("Stories");
    if(story_count)
    {
        is_active.resize(story_count);
        foreach(const QString& active_index, active_list)
            is_active.setBit(active_index.toInt());

        QMap<ProducerPointer, CoverageStart> start_info;

        for(int i = 0; i < story_count; ++i)
        {
            series_settings->set_array_index(i);

            StoryInfoPointer story_info = StoryInfoPointer(new StoryInfo());
            restore_story(series_settings, story_info);

            story_info->dashboard_compact_mode = compact_mode;
            story_info->dashboard_compression = compact_compression;

            // inject the story into the system, but don't start coverage

            ProducerPointer producer;
            if(cover_story(producer, story_info, CoverageStart::None))
            {
                series_info.producers.append(producer);

                CoverageStart coverage_start = CoverageStart::None;
                if(continue_coverage && is_active.testBit(i))
                {
                    if(story_info->reporter_beat.compare("Local"))
                        coverage_start = CoverageStart::Immediate;
                    else
                        // prevent non-local Reporters from potentially hammering servers all at once
                        coverage_start = CoverageStart::Delayed;
                }

                start_info[producer] = coverage_start;
            }
        }

        // launch the Stories in user-defined order
        foreach(ProducerPointer key, start_info.keys())
        {
            if(start_info[key] != CoverageStart::None)
                cover_story(key, key->get_story(), start_info[key]);
        }
    }

    series_settings->end_array();

    series_settings->end_section();
}

void MainWindow::save_series(const SeriesInfo &series_info)
{
    QString series_dir = QDir::toNativeSeparators(QString("%1/Series")
                                                .arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));
    if(!QFile::exists(series_dir))
    {
        QDir d(series_dir);
        if(!d.mkdir("."))
            return;
    }

    QString series_file_name = QDir::toNativeSeparators(QString("%1/%2").arg(series_dir).arg(QString(QUrl::toPercentEncoding(series_info.name, "", "?*$"))));
    SettingsPointer series_settings = SettingsPointer(new SettingsXML("NewsroomSeries", series_file_name));

    QStringList active;

    series_settings->cache();

    series_settings->begin_section("/Series");

    if(!series_info.producers.isEmpty())
    {
        series_settings->begin_array("Stories");

        int index = 0;
        foreach(ProducerPointer producer, series_info.producers)
        {
            if(producer->is_covering_story())
                active << QString::number(index);

            series_settings->set_array_index(index++);
            save_story(series_settings, producer->get_story());
        }

        series_settings->end_array();

        series_settings->set_item("active", active.join(","));
    }
    else
        series_settings->clear_section("Stories");

    series_settings->end_section();

    series_settings->flush();
}

void MainWindow::restore_story_defaults(StoryInfoPointer story_info)
{
    settings->begin_section("/StoryDefaults");

    story_info->story                    = settings->get_item("story", QUrl()).toUrl();
    story_info->identity                 = settings->get_item("identity", QString()).toString();
    story_info->reporter_beat            = settings->get_item("reporter_class", QString()).toString();
    story_info->reporter_id              = settings->get_item("reporter_id", QString()).toString();
    // Note: Reporter parameter defaults are managed by the AddStoryDialog class
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
    settings->set_item("reporter_class", story_info->reporter_beat);
    settings->set_item("reporter_id", story_info->reporter_id);
    // Note: Reporter parameter defaults are managed by the AddStoryDialog class
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
    settings->set_item("margin", story_info->margin);

    // Producer settings
    settings->set_item("font", story_info->font.toString());

    settings->end_section();

    settings_modified = true;
}

void MainWindow::save_story(SettingsPointer settings, StoryInfoPointer story_info)
{
    settings->set_item("story", story_info->story);
    settings->set_item("identity", story_info->identity);
    settings->set_item("reporter_class", story_info->reporter_beat);
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
    settings->set_item("margin", story_info->margin);

    // Producer settings
    settings->set_item("font", story_info->font.toString());
}

void MainWindow::restore_story(SettingsPointer settings, StoryInfoPointer story_info)
{
    story_info->story                    = settings->get_item("story", QUrl()).toUrl();
    story_info->identity                 = settings->get_item("identity", QString()).toString();
    story_info->reporter_beat            = settings->get_item("reporter_class", QString()).toString();
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
    story_info->margin                   = settings->get_item("margins", story_info->margin).toInt();

    // Producer settings
    story_info->font.fromString(settings->get_item("font", headline_font.toString()).toString());
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
                             tr("Newsroom"),
                             tr("Sorry, I already gave what help I could.\n"
                                "Maybe you could try asking a human?"));
}

void MainWindow::slot_menu_action(QAction* action)
{
    if(action == settings_action)
        slot_edit_settings(false);
    else if(action == about_action)
//        slot_about()
    {}
    else if(action == quit_action)
        slot_quit();
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
    if(settings_dlg)
        return;

    settings_action->setEnabled(false);
    ui->action_Settings->setEnabled(false);

    settings_dlg = new SettingsDialog(this);

    settings_dlg->set_autostart(auto_start);
    settings_dlg->set_continue_coverage(continue_coverage);
    settings_dlg->set_compact_mode(compact_mode, compact_compression);
    settings_dlg->set_font(headline_font);
    settings_dlg->set_styles(*(headline_style_list.data()));
    settings_dlg->set_series(series_ordered);

    restore_window_data(settings_dlg);

    connect(settings_dlg, &SettingsDialog::signal_edit_story, this, &MainWindow::slot_edit_story);

    QStringList original_series;
    foreach(const SeriesInfo& series_info, series_ordered)
        original_series << series_info.name;

    if(settings_dlg->exec() == QDialog::Accepted)
    {
        QString series_dir = QDir::toNativeSeparators(QString("%1/Series")
                                                    .arg(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0]));

        auto_start                       = settings_dlg->get_autostart();
        continue_coverage                = settings_dlg->get_continue_coverage();
        compact_mode                     = settings_dlg->get_compact_mode(compact_compression);
        headline_font                    = settings_dlg->get_font();
        settings_dlg->get_styles(*(headline_style_list.data()));

        // process any moves before any deletions!

        // the SeriesInfoList returned by SettingsDialog is the way
        // Series/Stories look now, and the assignment to 'series_ordered'
        // has IMMEDIATE affect.
        //
        // the remaining code simply cleans up the lingering remnants of
        // any deleted Series.

        series_ordered = settings_dlg->get_series();

        // first, handle Stories that might have moved to a different Series

        QStringList remaining_series;
        foreach(const SeriesInfo& series_info, series_ordered)
            remaining_series << series_info.name;

        // now, clear any deleted Series

        QStringList series_names = remaining_series;
        QStringList deleted_series;
        foreach(const QString& series_name, original_series)
        {
            if(!remaining_series.contains(series_name))
            {
                deleted_series << series_name;
                series_names.removeAll(series_name);
            }
        }

        foreach(const QString& series_name, deleted_series)
        {
            QString series_file_name = QDir::toNativeSeparators(QString("%1/%2").arg(series_dir).arg(QString(QUrl::toPercentEncoding(series_name, "", "?*$"))));
            SettingsPointer series_settings = SettingsPointer(new SettingsXML("NewsroomSeries", series_file_name));
            series_settings->remove();
        }

        save_application_settings();
    }

    save_window_data(settings_dlg);

    settings_dlg->deleteLater();
    settings_dlg = nullptr;

    settings_action->setEnabled(true);
    ui->action_Settings->setEnabled(true);
}

void MainWindow::slot_edit_story(const QString& story_id)
{
    ProducerPointer producer;
    foreach(const SeriesInfo& si, series_ordered)
    {
        foreach(ProducerPointer pp, si.producers)
        {
            StoryInfoPointer story_info = pp->get_story();
            if(!story_info->identity.compare(story_id))
            {
                producer = pp;
                break;
            }
        }
    }

    Q_ASSERT(!producer.isNull());       // this really shouldn't happen

    StoryInfoPointer story_info = producer->get_story();
    PluginsInfoVector* reporters_info = nullptr;

    if(beat_reporters.contains(story_info->reporter_beat))
        reporters_info = &beat_reporters[story_info->reporter_beat];

    if(!reporters_info)
    {
        QMessageBox::critical(0,
                              tr("Newsroom: Error"),
                              tr("No Reporters are available to cover the \"%1\" beat!")
                                    .arg(story_info->reporter_beat));
        return;
    }

    CoverageStart coverage_start = producer->is_covering_story() ? CoverageStart::Immediate : CoverageStart::None;

    AddStoryDialog addstory_dlg(reporters_info, story_info, settings, this);

    addstory_dlg.lock_angle();

    restore_window_data(&addstory_dlg);

    if(edit_story_first_time && coverage_start == CoverageStart::Immediate)
    {
        QMessageBox::warning(&addstory_dlg,
                             tr("Newsroom: Edit Story"),
                             tr("Be aware that editing a Story takes place outside of the this\n"
                                "dialog, and any changes made to an active Story will have\n"
                                "immediate effect if you press \"Ok\" to accept changes."));
        edit_story_first_time = false;
    }

    if(addstory_dlg.exec() == QDialog::Accepted)
    {
        producer->stop_covering_story();

        // AddStoryDialog has automatically updated 'story_info' with all settings
        (void)cover_story(producer, story_info, coverage_start, reporters_info);
    }

    save_window_data(&addstory_dlg);
}
