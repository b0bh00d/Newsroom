#include <random>

#include <QtWidgets/QMessageBox>

#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>

#include "types.h"
#include "storyinfo.h"
#include "settings.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "chyron.h"

const int application_settings_version = 1;
const int series_settings_version = 1;

MainWindow* mainwindow;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
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

    application_settings_folder_name = QDir::toNativeSeparators(QString("%1/Newsroom%2")
                                    .arg(QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation)[0])
#ifdef QT_DEBUG
                                    .arg("_db")
#else
                                    .arg("")
#endif
                                    );

    QDir settings_dir(application_settings_folder_name);
    if(!settings_dir.exists())
    {
        if(!settings_dir.mkpath("."))
        {
            QMessageBox::critical(nullptr,
                                  tr("Newsroom: Error"),
                                  tr("Could not create application settings folder:\n%1").arg(application_settings_folder_name));
            QTimer::singleShot(100, qApp, &QApplication::quit);
            return;
        }
    }

    application_settings_file_name = QDir::toNativeSeparators(QString("%1/Newsroom").arg(application_settings_folder_name));
    application_settings = SettingsPointer(new SettingsXML("Newsroom", application_settings_file_name));
    application_settings->init();

    series_folder = QDir::toNativeSeparators(QString("%1/Series").arg(application_settings_folder_name));
    parameters_base_folder = QDir::toNativeSeparators(QString("%1/Parameters").arg(application_settings_folder_name));
    parameters_defaults_folder = QDir::toNativeSeparators(QString("%1/Defaults").arg(parameters_base_folder));
    parameters_stories_folder = QDir::toNativeSeparators(QString("%1/Stories").arg(parameters_base_folder));

    // Load all the available Reporter plug-ins
    if(!configure_reporters())
    {
        QMessageBox::critical(nullptr,
                              tr("Newsroom: Error"),
                              tr("A critical error occurred while configuring Reporter\n"
                                 "plug-ins!  Newsroom cannot function without Reporters."));
        QTimer::singleShot(100, qApp, &QApplication::quit);
        return;
    }

    load_application_settings();

    setAcceptDrops(true);

    // Tray

    trayIcon = new QSystemTrayIcon(this);
    auto connected = connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::slot_message_clicked);
    ASSERT_UNUSED(connected)
    connected = connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slot_icon_activated);
    ASSERT_UNUSED(connected)

    connected = connect(ui->action_Settings, &QAction::triggered, this, &MainWindow::slot_edit_settings);
    ASSERT_UNUSED(connected)

    trayIcon->setIcon(QIcon(":/images/Newsroom16.png"));
#ifdef QT_DEBUG
    trayIcon->setToolTip(tr("Newsroom_db"));
#else
    trayIcon->setToolTip(tr("Newsroom"));
#endif

    build_tray_menu();

    trayIcon->show();

    qApp->setQuitOnLastWindowClosed(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

const ReporterInfo* MainWindow::get_reporter_info(const QString& id) const
{
    const ReporterInfo* reporter_info{nullptr};
    foreach(const auto& beat_key, beats.keys())
    {
        foreach(const auto& reporter_ref, beats[beat_key])
        {
            if(!reporter_ref.id.compare(id))
            {
                reporter_info = &reporter_ref;
                break;
            }
        }

        if(reporter_info)
            break;
    }

    return reporter_info;
}

bool MainWindow::configure_reporters()
{
    QMap<QString, bool> id_filter;

    beats.clear();

    // the Parameters folder in the config holds cached settings
    // for a given reporter ID in different contexts

    StringListMap parameter_files;

    QDir parameters_dir(parameters_base_folder);
    if(!parameters_dir.exists())
    {
        if(!parameters_dir.mkpath("."))
            return false;
        if(!parameters_dir.mkpath("Defaults"))
            return false;
        if(!parameters_dir.mkpath("Stories"))
            return false;
    }
    else
    {
        QDirIterator it(parameters_base_folder, QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            auto full_path = it.next();
            if(!full_path.isNull())
            {
                QFileInfo info(full_path);
                auto key = QString(QUrl::fromPercentEncoding(info.baseName().toUtf8()));
                if(!parameter_files.contains(key))
                    parameter_files[key] = QStringList();
                parameter_files[key] << full_path;
            }
        }
    }

    QDir plugins("reporters");
    auto plugins_list = plugins.entryList(QStringList() << "*.dll" << "*.so", QDir::Files);
    foreach(const auto& filename, plugins_list)
    {
        auto plugin_path = QDir::toNativeSeparators(QString("%1/%2").arg(plugins.absolutePath()).arg(filename));
        FactoryPointer plugin(new QPluginLoader(plugin_path));
        auto instance = plugin->instance();
        if(instance)
        {
            auto ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
            if(ireporterfactory)
            {
                auto ireporter = ireporterfactory->newInstance();
                auto display = ireporter->DisplayName();

                ReporterInfo pi_info{plugin, plugin_path,
                                    display[0], display[1],
                                    ireporter->PluginID(),
                                    ireporter->RequiresVersion(),
                                    ireporter->Requires()};

                if(!id_filter.contains(pi_info.id))
                {
                    // ensure a sane operating state by upgrading or destroying our
                    // cached Reporter data, as indicated

                    if(parameter_files.contains(pi_info.id))
                    {
                        foreach(const auto& param_filename, parameter_files[pi_info.id])
                        {
                            auto reporter_settings = SettingsPointer(new SettingsXML("ReporterData", param_filename));
                            reporter_settings->init();

                            auto cached_version = reporter_settings->get_version();
                            auto current_version = ireporter->RequiresVersion();

                            if(cached_version > current_version)
                                // the Reporter instance is somehow older than the cached
                                // data, so destroy the cached file
                                QFile::remove(reporter_settings->get_filename());

                            else if(cached_version < current_version)
                            {
                                // read back in the data in the previous version format
                                auto requires_params = ireporter->Requires(cached_version);

                                QStringList old_data;

                                reporter_settings->begin_section("/ReporterData");
                                  for(auto i = 0, j = 0;i < requires_params.length();i += 2, ++j)
                                  {
                                      old_data.append(QString());
                                      old_data[j] = reporter_settings->get_item(requires_params[i], QString()).toString();
                                  }
                                reporter_settings->end_section();

                                // data is upgraded in-place
                                if(ireporter->RequiresUpgrade(reporter_settings->get_version(), old_data))
                                {
                                    // save it back out in the upgraded form

                                    reporter_settings->clear_section("/ReporterData");
                                    reporter_settings->set_version(current_version);

                                    requires_params = ireporter->Requires();

                                    reporter_settings->begin_section("/ReporterData");
                                      for(auto i = 0, j = 0;i < requires_params.length();i += 2, ++j)
                                          reporter_settings->set_item(requires_params[i], old_data[j]);
                                    reporter_settings->end_section();

                                    reporter_settings->flush();
                                }
                                else if(!ireporter->ErrorString().isEmpty())
                                {
                                    // something went wrong in the upgrade.  bail.
                                    QMessageBox::critical(nullptr,
                                                          tr("Newsroom: Error"),
                                                          tr("The Reporter \"%1\" reported an error\n"
                                                             "\"%1\"\n"
                                                             "while attempting to upgrade cached data!")
                                                                .arg(ireporter->DisplayName()[0])
                                                                .arg(ireporter->ErrorString()));
                                    return false;
                                }
                            }
                        }
                    }

                    auto pi_class = ireporter->PluginClass();
                    if(!beats.contains(pi_class))
                        beats[pi_class] = ReportersInfoVector();
                    beats[pi_class].push_back(pi_info);
                }
            }
        }
    }

    return beats.count() > 0;
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
    auto pixmapRatio = static_cast<float>(background_image->width() / background_image->height());
    auto windowRatio = static_cast<float>(winSize.width() / winSize.height());

    if(pixmapRatio > windowRatio)
    {
        auto newWidth = static_cast<int>(winSize.height() * pixmapRatio);
        auto offset = (newWidth - winSize.width()) / -2;
        painter.drawPixmap(offset, 0, newWidth, winSize.height(), *(background_image.data()));
    }
    else
    {
        auto newHeight = static_cast<int>(winSize.width() / pixmapRatio);
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

void MainWindow::fix_angle_duplication(StoryInfoPointer story_info)
{
    auto prefix = QString("%1::").arg(story_info->angle);
    foreach(auto& series_info, series_ordered)
    {
        foreach(auto producer, series_info->producers)
        {
            auto si = producer->get_story();
            if(si->angle.startsWith(prefix))
            {
                // generate a random number to append to the identity
                std::random_device rd;
                std::mt19937 mt(rd());
                std::uniform_int_distribution<> dist(INT_MAX / 2, INT_MAX);

                prefix = QString("%1::%2").arg(story_info->angle).arg(QString::number(dist(mt),16));
                story_info->angle = prefix;
                break;
            }
        }
    }
}

bool MainWindow::cover_story(ProducerPointer& producer,
                             StoryInfoPointer story_info,
                             CoverageStart coverage_start,
                             const ReportersInfoVector *reporters_info)
{
    auto result{false};

    if(producer.isNull())
    {
        if(!reporters_info)
        {
            if(beats.contains(story_info->reporter_beat))
                reporters_info = &beats[story_info->reporter_beat];

            if(!reporters_info)
            {
                QMessageBox::critical(nullptr,
                                      tr("Newsroom: Error"),
                                      tr("No Reporters are available to cover the \"%1\" beat!").arg(story_info->reporter_beat));
                return result;
            }
        }

        // create a Chyron to display the Headlines for this story

        auto chyron = ChyronPointer(new Chyron(story_info, lane_manager));

        // assign a staff Reporter from the selected department to cover the story

        IReporterPointer reporter;
        foreach(const auto& pi_info, (*reporters_info))
        {
            auto instance = pi_info.factory->instance();
            auto ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
            Q_ASSERT(ireporterfactory);
            auto plugin_reporter = ireporterfactory->newInstance();
            if(!story_info->reporter_id.compare(plugin_reporter->PluginID()))
            {
                reporter = plugin_reporter;
                // give the Reporter their assignment
                if(!reporter->SetRequirements(story_info->reporter_parameters))
                {
                    QMessageBox::critical(nullptr,
                                          tr("Newsroom: Error"),
                                          tr("Reporters \"%1\" encountered an error.\n"
                                             "\"%1\"").arg(reporter->ErrorString()));
                    return result;
                }
                break;
            }
        }

        // assign a staff Producer to receive Reporter filings and create Headlines

        producer = ProducerPointer(new Producer(chyron, reporter, story_info, headline_style_list, this));

        connect(producer.data(), &Producer::signal_shelve_story, this, &MainWindow::slot_shelve_story);
        connect(producer.data(), &Producer::signal_unshelve_story, this, &MainWindow::slot_unshelve_story);
    }

    if(coverage_start == CoverageStart::Delayed)
    {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<> dist(2, static_cast<int>(std::nextafter(6, INT_MAX)));

        auto offset = last_start_offset + dist(mt);
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
            QMessageBox::critical(nullptr,
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
    auto accept{false};

    //    QStringList formats = event->mimeData()->formats();
    //    if(event->mimeData()->hasFormat("text/uri-list"))

    if(event->mimeData()->hasUrls())
    {
        auto urls = event->mimeData()->urls();
        foreach(const auto& url, urls)
        {
            if(url.isLocalFile())
            {
                auto text = url.toLocalFile();
                auto name = mime_db.mimeTypeForFile(text).name();
                if(name.startsWith("text/"))
                    accept = true;
                else if(name.contains("mswinurl"))
                    accept = true;
            }
        }
    }

    if(accept)
        event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    auto accept{false};

    // each element in the list has already been vetted
    // in dragEnterEvent() above

    auto urls = event->mimeData()->urls();
    foreach(const auto& story, urls)
    {
        auto story_info = StoryInfoPointer(new StoryInfo());
        restore_story_defaults(story_info);

        if(story.isLocalFile())
        {
            // see if this is a .url file, which is an INI file
            // that holds an actual URL address

            QString url;

            auto local_file = story.toLocalFile().toLower();
            if(local_file.endsWith(".url"))
            {
                QSettings url_file(local_file, QSettings::IniFormat);
                url_file.beginGroup("InternetShortcut");
                    if(url_file.contains("URL"))
                        url = url_file.value("URL").toString();
                url_file.endGroup();
            }

            if(url.isEmpty())
                story_info->story = story;
            else
                story_info->story = QUrl(url);
        }
        else
            story_info->story = story;

        story_info->angle.clear();

        // Provide a 'hint' as to which beat is appropriate for this Story

        QMap<float, QStringList> guesses;

        story_info->reporter_beat.clear();
        foreach(const auto& key, beats.keys())
        {
            foreach(const auto & pi_info, beats[key])
            {
                auto instance = pi_info.factory->instance();
                auto ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
                Q_ASSERT(ireporterfactory);
                auto plugin_reporter = ireporterfactory->newInstance();

                auto best_guess = plugin_reporter->Supports(story_info->story);
                if(!guesses.contains(best_guess))
                    guesses[best_guess] = QStringList();
                guesses[best_guess] << key;
            }
        }

        // identify and suggest the best guess
        auto keys = guesses.keys();
        if(keys.length() > 1)
            std::sort(keys.begin(), keys.end(), /* descending */ std::greater<float>());
        story_info->reporter_beat = guesses[keys[0]][0];

        if(story_info->reporter_beat.isEmpty())
        {
            QMessageBox::critical(nullptr,
                                  tr("Newsroom: Error"),
                                  tr("No Reporters are available to cover this Story's beat!"));
            return;
        }

        AddStoryDialog addstory_dlg(beats,
                                    story_info,
                                    application_settings,
                                    parameters_defaults_folder,
                                    AddStoryDialog::Mode::Add,
                                    this);

        restore_window_data(&addstory_dlg);

        auto story_accepted{false};
        if(addstory_dlg.exec() == QDialog::Accepted)
        {
            // AddStoryDialog has automatically updated 'story_info' with all settings
            save_story_defaults(story_info);

            // fix any duplication issues with the story identity
            fix_angle_duplication(story_info);

            story_info->identity = QUuid::createUuid().toString();

            // find "Default"
            for(auto iter = series_ordered.begin();iter != series_ordered.end();++iter)
            {
                if(!(*iter)->name.compare("Default"))
                {
                    auto reporters_info = &beats[story_info->reporter_beat];

                    story_info->dashboard_compact_mode = (*iter)->compact_mode;
                    story_info->dashboard_compression = (*iter)->compact_compression;

                    ProducerPointer producer;
                    if(cover_story(producer, story_info, autostart_coverage ? CoverageStart::Immediate : CoverageStart::None, reporters_info))
                    {
                        (*iter)->producers.append(producer);
                        story_accepted = true;
                    }
                    break;
                }
            }
        }

        save_window_data(&addstory_dlg);
        if(story_accepted)
            save_application_settings();
    }

    if(accept)
        event->acceptProposedAction();
}

void MainWindow::save_window_data(QWidget* window)
{
    if(!window_geometry_save_enabled)
        return;

    auto key = window->windowTitle();
    window_data[key] = window->saveGeometry();

    settings_modified = true;
}

void MainWindow::restore_window_data(QWidget* window)
{
    auto key = window->windowTitle();
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

    settings_action = new QAction(QIcon(":/images/Options.png"), tr("&Edit Settings..."), this);
    about_action    = new QAction(QIcon(":/images/About.png"), tr("&About"), this);
    quit_action     = new QAction(QIcon(":/images/Quit.png"), tr("&Quit"), this);

    trayIconMenu = new QMenu(this);

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
    application_settings->set_version(application_settings_version);

    application_settings->clear_section("/Application");

    application_settings->begin_section("/Application");

    application_settings->set_item("auto_start", auto_start);
    application_settings->set_item("continue_coverage", continue_coverage);
    application_settings->set_item("autostart_coverage", autostart_coverage);
    application_settings->set_item("chyron.font", headline_font.toString());

    application_settings->clear_section("HeadlineStyles");

    application_settings->begin_array("HeadlineStyles");
    auto index{0};
    foreach(const auto& style, *(headline_style_list.data()))
    {
        application_settings->set_array_index(index++);

        application_settings->set_item("name", style.name);
        application_settings->set_item("triggers", style.triggers);
        application_settings->set_item("stylesheet", style.stylesheet);
    }

    application_settings->end_array();

    application_settings->clear_section("WindowData");
    if(window_data.size())
    {
        application_settings->begin_array("WindowData");

        index = 0;
        auto keys = window_data.keys();
        foreach(auto& key, keys)
        {
            application_settings->set_array_index(index++);

            application_settings->set_item("key", key);
            application_settings->set_item("geometry", window_data[key]);
        }

        application_settings->end_array();
    }

    QStringList series_names;
    foreach(auto series_info, series_ordered)
    {
        series_names << series_info->name;
        save_series(series_info);
    }

    application_settings->set_item("series", series_names);

    application_settings->end_section();

    application_settings->flush();

    settings_modified = false;
}

void MainWindow::load_application_settings()
{
    window_data.clear();
    if(lane_manager)
        lane_manager.clear();

    series_ordered.clear();

    // add a Default stylesheet entry on first runs
    headline_style_list->clear();
    HeadlineStyle hs{"Default", default_stylesheet, QStringList()};
    headline_style_list->append(hs);

    application_settings->begin_section("/Application");

    auto_start = application_settings->get_item("auto_start", false).toBool();
    continue_coverage = application_settings->get_item("continue_coverage", false).toBool();
    autostart_coverage = application_settings->get_item("autostart_coverage", true).toBool();
    auto f = ui->label->font();
    auto font_str = application_settings->get_item("chyron.font", f.toString()).toString();
    if(!font_str.isEmpty())
        headline_font.fromString(font_str);

    auto styles_size = application_settings->begin_array("HeadlineStyles");
    if(styles_size)
    {
        for(auto i = 0; i < styles_size; ++i)
        {
            application_settings->set_array_index(i);

            HeadlineStyle hs{application_settings->get_item("name").toString(),
                             application_settings->get_item("stylesheet").toString(),
                             application_settings->get_item("triggers").toStringList()};

            auto updated{false};
            for(auto iter = headline_style_list->begin(); iter != headline_style_list->end();++iter)
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
    application_settings->end_array();

    lane_manager = LaneManagerPointer(new LaneManager(headline_font, (*headline_style_list.data())[0].stylesheet, this));

    auto windata_size = application_settings->begin_array("WindowData");
    if(windata_size)
    {
        for(auto i = 0; i < windata_size; ++i)
        {
            application_settings->set_array_index(i);

            auto key = application_settings->get_item("key").toString();
            window_data[key] = application_settings->get_item("geometry").toByteArray();
        }
    }
    application_settings->end_array();

    auto series_names = application_settings->get_item("series", QStringList() << "Default").toStringList();
    foreach(const auto& series_name, series_names)
    {
        auto si = SeriesInfoPointer(new SeriesInfo());
        si->name = series_name;
        series_ordered.append(si);

        load_series(series_ordered.back());
    }

    application_settings->end_section();

    settings_modified = !QFile::exists(application_settings_file_name);
}

void MainWindow::load_series(SeriesInfoPointer series_info)
{
    if(!QFile::exists(series_folder))
        return;

    auto series_file_name = QDir::toNativeSeparators(QString("%1/%2").arg(series_folder).arg(encode_for_filesystem(series_info->name)));
    auto series_settings = SettingsPointer(new SettingsXML("NewsroomSeries", series_file_name));
    series_settings->init();

    series_settings->begin_section("/Series");

    series_info->compact_mode = series_settings->get_item("compact_mode", series_info->compact_mode).toBool();
    series_info->compact_compression = series_settings->get_item("compact_compression", series_info->compact_compression).toInt();

    QBitArray is_active;
    auto active_list = series_settings->get_item("active", QString()).toString().split(",");

    auto story_count = series_settings->begin_array("Stories");
    if(story_count)
    {
        is_active.resize(story_count);
        foreach(const auto& active_index, active_list)
            is_active.setBit(active_index.toInt());

        QMap<ProducerPointer, CoverageStart> start_info;

        for(auto i = 0; i < story_count; ++i)
        {
            series_settings->set_array_index(i);

            auto story_info = StoryInfoPointer(new StoryInfo());
            restore_story(series_settings, story_info);

            story_info->dashboard_compact_mode = series_info->compact_mode;
            story_info->dashboard_compression = series_info->compact_compression;

            // inject the story into the system, but don't start coverage

            ProducerPointer producer;
            if(cover_story(producer, story_info, CoverageStart::None))
            {
                series_info->producers.append(producer);

                auto coverage_start = CoverageStart::None;
                if(continue_coverage && is_active.testBit(i))
                {
                    if(story_info->story.isLocalFile() && QFile::exists(story_info->story.toLocalFile()))
                        coverage_start = CoverageStart::Immediate;
                    else
                        // prevent non-local Reporters from potentially hammering servers all at once
                        coverage_start = CoverageStart::Delayed;
                }

                start_info[producer] = coverage_start;
            }
        }

        // launch the Stories in user-defined order
        foreach(auto key, start_info.keys())
        {
            if(start_info[key] != CoverageStart::None)
                cover_story(key, key->get_story(), start_info[key]);
        }
    }

    series_settings->end_array();

    series_settings->end_section();
}

void MainWindow::save_series(SeriesInfoPointer series_info)
{
    if(!QFile::exists(series_folder))
    {
        QDir d(series_folder);
        if(!d.mkpath("."))
            return;
    }

    auto series_file_name = QDir::toNativeSeparators(QString("%1/%2").arg(series_folder).arg(encode_for_filesystem(series_info->name)));
    auto series_settings = SettingsPointer(new SettingsXML("NewsroomSeries", series_file_name));

    QStringList active;

    series_settings->init(true);

    series_settings->set_version(series_settings_version);

    series_settings->begin_section("/Series");

    series_settings->set_item("compact_mode", series_info->compact_mode);
    series_settings->set_item("compact_compression", series_info->compact_compression);

    series_settings->clear_section("Stories");

    if(!series_info->producers.isEmpty())
    {
        series_settings->begin_array("Stories");

        auto index{0};
        foreach(auto producer, series_info->producers)
        {
            if(producer->is_covering_story())
                active << QString::number(index);

            series_settings->set_array_index(index++);
            save_story(series_settings, producer->get_story());
        }

        series_settings->end_array();

        series_settings->set_item("active", active.join(","));
    }

    series_settings->end_section();

    series_settings->flush();
}

void MainWindow::restore_story_defaults(StoryInfoPointer story_info)
{
    application_settings->begin_section("/StoryDefaults");

    story_info->story                    = application_settings->get_item("story", QUrl()).toUrl();
    story_info->angle                    = application_settings->get_item("identity", QString()).toString();
    story_info->reporter_beat            = application_settings->get_item("reporter_class", QString()).toString();
    story_info->reporter_id              = application_settings->get_item("reporter_id", QString()).toString();
    story_info->reporter_parameters_version = application_settings->get_item("reporter_parameters_version", 1).toInt();
    // Note: Reporter parameter defaults are managed by the AddStoryDialog class
    story_info->ttl                      = static_cast<unsigned int>(application_settings->get_item("ttl", story_info->ttl).toInt());
    story_info->primary_screen           = application_settings->get_item("primary_screen", 0).toInt();
    story_info->headlines_always_visible = application_settings->get_item("headlines_always_visible", true).toBool();
    story_info->interpret_as_pixels      = application_settings->get_item("interpret_as_pixels", true).toBool();
    story_info->headlines_pixel_width    = application_settings->get_item("headlines_pixel_width", 0).toInt();
    story_info->headlines_pixel_height   = application_settings->get_item("headlines_pixel_height", 0).toInt();
    story_info->headlines_percent_width  = application_settings->get_item("headlines_percent_width", 0.0).toDouble();
    story_info->headlines_percent_height = application_settings->get_item("headlines_percent_height", 0.0).toDouble();
    story_info->limit_content_to         = application_settings->get_item("limit_content_to", 0).toInt();
    story_info->headlines_fixed_type     = static_cast<FixedText>(application_settings->get_item("headlines_fixed_type", 0).toInt());
    story_info->include_progress_bar     = application_settings->get_item("include_progress_bar", false).toBool();
    story_info->progress_text_re         = application_settings->get_item("progress_text_re", story_info->progress_text_re).toString();
    story_info->progress_on_top          = application_settings->get_item("progress_on_top", false).toBool();
    story_info->entry_type               = static_cast<AnimEntryType>(application_settings->get_item("entry_type", 0).toInt());
    story_info->exit_type                = static_cast<AnimExitType>(application_settings->get_item("exit_type", 0).toInt());
    story_info->anim_motion_duration     = application_settings->get_item("anim_motion_duration", story_info->anim_motion_duration).toInt();
    story_info->fade_target_duration     = application_settings->get_item("fade_target_duration", story_info->fade_target_duration).toInt();
    story_info->train_use_age_effect     = application_settings->get_item("train_use_age_effects", false).toBool();
    story_info->train_age_effect         = static_cast<AgeEffects>(application_settings->get_item("train_age_effect", 0).toInt());
    story_info->train_age_percent        = application_settings->get_item("train_age_percent", story_info->train_age_percent).toInt();
    story_info->dashboard_use_age_effect = application_settings->get_item("dashboard_use_age_effects", false).toBool();
    story_info->dashboard_age_percent    = application_settings->get_item("dashboard_age_percent", story_info->dashboard_age_percent).toInt();
    story_info->dashboard_group_id       = application_settings->get_item("dashboard_group_id", QString()).toString();

    // Chyron settings
    story_info->margin                   = application_settings->get_item("margins", story_info->margin).toInt();

    // Producer settings
    story_info->font.fromString(application_settings->get_item("font", headline_font.toString()).toString());

    application_settings->end_section();
}

void MainWindow::save_story_defaults(StoryInfoPointer story_info)
{
    application_settings->begin_section("/StoryDefaults");

    application_settings->set_item("story", story_info->story);
    application_settings->set_item("identity", story_info->angle);
    application_settings->set_item("reporter_class", story_info->reporter_beat);
    application_settings->set_item("reporter_id", story_info->reporter_id);
    application_settings->set_item("reporter_parameters_version", story_info->reporter_parameters_version);
    // Note: Reporter parameter defaults are managed by the AddStoryDialog class
    application_settings->set_item("ttl", story_info->ttl);
    application_settings->set_item("primary_screen", story_info->primary_screen);
    application_settings->set_item("headlines_always_visible", story_info->headlines_always_visible);
    application_settings->set_item("interpret_as_pixels", story_info->interpret_as_pixels);
    application_settings->set_item("headlines_pixel_width", story_info->headlines_pixel_width);
    application_settings->set_item("headlines_pixel_height", story_info->headlines_pixel_height);
    application_settings->set_item("headlines_percent_width", story_info->headlines_percent_width);
    application_settings->set_item("headlines_percent_height", story_info->headlines_percent_height);
    application_settings->set_item("limit_content_to", story_info->limit_content_to);
    application_settings->set_item("headlines_fixed_type", static_cast<int>(story_info->headlines_fixed_type));
    application_settings->set_item("include_progress_bar", story_info->include_progress_bar);
    application_settings->set_item("progress_text_re", story_info->progress_text_re);
    application_settings->set_item("progress_on_top", story_info->progress_on_top);
    application_settings->set_item("entry_type", static_cast<int>(story_info->entry_type));
    application_settings->set_item("exit_type", static_cast<int>(story_info->exit_type));
    application_settings->set_item("anim_motion_duration", story_info->anim_motion_duration);
    application_settings->set_item("fade_target_duration", story_info->fade_target_duration);
    application_settings->set_item("train_use_age_effects", story_info->train_use_age_effect);
    application_settings->set_item("train_age_effect", static_cast<int>(story_info->train_age_effect));
    application_settings->set_item("train_age_percent", story_info->train_age_percent);
    application_settings->set_item("dashboard_use_age_effects", story_info->dashboard_use_age_effect);
    application_settings->set_item("dashboard_age_percent", story_info->dashboard_age_percent);
    application_settings->set_item("dashboard_group_id", story_info->dashboard_group_id);

    // Chyron settings
    application_settings->set_item("margin", story_info->margin);

    // Producer settings
    application_settings->set_item("font", story_info->font.toString());

    application_settings->end_section();

    settings_modified = true;
}

void MainWindow::save_story(SettingsPointer settings, StoryInfoPointer story_info)
{
    settings->set_item("story", story_info->story);
    settings->set_item("angle", story_info->angle);
    settings->set_item("identity", story_info->identity);
    settings->set_item("reporter_class", story_info->reporter_beat);
    settings->set_item("reporter_id", story_info->reporter_id);
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

    // save the Reporter's "live" parameter data to a separate,
    // upgradable file independent of the other Story data

    auto story_filename = QDir::toNativeSeparators(QString("%1/%2")
                                                .arg(parameters_stories_folder)
                                                .arg(encode_for_filesystem(story_info->identity))
                                                );

    QDir story_dir(story_filename);
    if(!story_dir.exists())
    {
        if(!story_dir.mkpath("."))
            return;     // TODO: need to handle this error
    }

    auto parameters_filename = QDir::toNativeSeparators(QString("%1/%2")
                                                .arg(story_filename)
                                                .arg(encode_for_filesystem(story_info->reporter_id))
                                                );

    // get the Reporter metadata from the beats[]

    auto reporter_info = get_reporter_info(story_info->reporter_id);
    Q_ASSERT(reporter_info);

    auto params_count = reporter_info->params_requires.count() / 2;
    if(params_count && (params_count == story_info->reporter_parameters.count()))
    {
        auto reporter_settings = SettingsPointer(new SettingsXML("ReporterData", parameters_filename));
        reporter_settings->init(true);
        reporter_settings->set_version(reporter_info->params_version);

        reporter_settings->begin_section("/ReporterData");
          for(auto i = 0, j = 0;i < reporter_info->params_requires.length();i += 2, ++j)
            reporter_settings->set_item(reporter_info->params_requires[i], story_info->reporter_parameters[j]);
        reporter_settings->end_section();

        reporter_settings->flush();
    }
}

void MainWindow::restore_story(SettingsPointer settings, StoryInfoPointer story_info)
{
    story_info->story                    = settings->get_item("story", QUrl()).toUrl();
    story_info->angle                    = settings->get_item("angle", QString()).toString();
    story_info->identity                 = settings->get_item("identity", QString()).toString();
    story_info->reporter_beat            = settings->get_item("reporter_class", QString()).toString();
    story_info->reporter_id              = settings->get_item("reporter_id", QString()).toString();
    story_info->ttl                      = static_cast<unsigned int>(settings->get_item("ttl", story_info->ttl).toInt());
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

    // restore the Reporter's "live" parameter data from an independent
    // data file

    story_info->reporter_parameters_version = 1;
    story_info->reporter_parameters.clear();

    auto parameters_filename = QDir::toNativeSeparators(QString("%1/%2/%3")
                                                .arg(parameters_stories_folder)
                                                .arg(encode_for_filesystem(story_info->identity))
                                                .arg(encode_for_filesystem(story_info->reporter_id))
                                                );
    auto reporter_settings = SettingsPointer(new SettingsXML("ReporterData", parameters_filename));
    if(QFile::exists(reporter_settings->get_filename()))
    {
        reporter_settings->init();

        // get the Reporter metadata from the beats[]

        auto reporter_info = get_reporter_info(story_info->reporter_id);
        Q_ASSERT(reporter_info);

        // this shouldn't happen
        Q_ASSERT(reporter_info->params_version == reporter_settings->get_version());

        story_info->reporter_parameters_version = reporter_settings->get_version();

        reporter_settings->begin_section("/ReporterData");
          for(auto i = 0, j = 0;i < reporter_info->params_requires.length();i += 2, ++j)
          {
              story_info->reporter_parameters.append(QString());
              story_info->reporter_parameters[j] = reporter_settings->get_item(reporter_info->params_requires[i], QString()).toString();
          }
        reporter_settings->end_section();
    }
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
    QMessageBox::information(nullptr,
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
    settings_dlg->set_autostart_coverage(autostart_coverage);
    settings_dlg->set_font(headline_font);
    settings_dlg->set_styles(*(headline_style_list.data()));
    settings_dlg->set_series(series_ordered);

    restore_window_data(settings_dlg);

    connect(settings_dlg, &SettingsDialog::signal_edit_story, this, &MainWindow::slot_edit_story);

    QStringList original_series;
    foreach(auto series_info, series_ordered)
        original_series << series_info->name;

    if(settings_dlg->exec() == QDialog::Accepted)
    {
        auto_start         = settings_dlg->get_autostart();
        continue_coverage  = settings_dlg->get_continue_coverage();
        autostart_coverage = settings_dlg->get_autostart_coverage();
        headline_font      = settings_dlg->get_font();
        settings_dlg->get_styles(*(headline_style_list.data()));

        // get a list of the Story ids that will deleted by the call
        // to get_series() below.  this gives us a means to clear any
        // cached data.

        auto removed_stories = settings_dlg->get_removed_stories();

        // the SeriesInfoList returned by SettingsDialog is the way
        // Series/Stories look now, and the assignment to 'series_ordered'
        // has IMMEDIATE affect.
        //
        // the remaining code simply cleans up the lingering remnants of
        // any deleted Series.

        series_ordered = settings_dlg->get_series();

        // clear any deleted Series

        QStringList remaining_series;
        foreach(auto series_info, series_ordered)
            remaining_series << series_info->name;

        auto series_names = remaining_series;
        QStringList deleted_series;
        foreach(const auto& series_name, original_series)
        {
            if(!remaining_series.contains(series_name))
            {
                deleted_series << series_name;
                series_names.removeAll(series_name);
            }
        }

        foreach(const auto& series_name, deleted_series)
        {
            auto series_file_name = QDir::toNativeSeparators(QString("%1/%2").arg(series_folder).arg(encode_for_filesystem(series_name)));
            auto series_settings = SettingsPointer(new SettingsXML("NewsroomSeries", series_file_name));
            series_settings->remove();
        }

        // clean up any abandoned Story data
        foreach(const auto& story_id, removed_stories)
        {
            auto parameters_folder = QDir::toNativeSeparators(QString("%1/%2")
                                                        .arg(parameters_stories_folder)
                                                        .arg(encode_for_filesystem(story_id))
                                                        );
            if(QFile::exists(parameters_folder))
            {
                QDir d(parameters_folder);
                d.removeRecursively();
            }
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
    foreach(auto si, series_ordered)
    {
        foreach(auto pp, si->producers)
        {
            auto story_info = pp->get_story();
            if(!story_info->identity.compare(story_id))
            {
                producer = pp;
                break;
            }
        }
    }

    Q_ASSERT(!producer.isNull());       // this really shouldn't happen

    auto story_info = producer->get_story();

    auto coverage_start = producer->is_covering_story() ? CoverageStart::Immediate : CoverageStart::None;

    AddStoryDialog addstory_dlg(beats,
                                story_info,
                                application_settings,
                                parameters_defaults_folder,
                                AddStoryDialog::Mode::Edit,
                                this);

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
        auto reporters_info = &beats[story_info->reporter_beat];
        (void)cover_story(producer, story_info, coverage_start, reporters_info);
    }

    save_window_data(&addstory_dlg);
}

void MainWindow::slot_shelve_story()
{
    auto producer_raw = qobject_cast<Producer*>(sender());

    // find this Producer shared pointer so we don't cling to the raw pointer

    ProducerPointer producer;
    foreach(auto si, series_ordered)
    {
        foreach(auto pp, si->producers)
        {
            if(pp.data() == producer_raw)
            {
                producer = pp;
                break;
            }
        }

        if(!producer.isNull())
            break;
    }

    if(!shelve_queue.contains(producer))
        shelve_queue.enqueue(producer);
    if(shelve_queue.length() == 1)
        QTimer::singleShot(100, this, &MainWindow::slot_process_shelve_queue);
}

void MainWindow::slot_unshelve_story()
{
    auto producer_raw = qobject_cast<Producer*>(sender());

    // find this Producer shared pointer so we don't cling to the raw pointer

    ProducerPointer producer;
    foreach(auto si, series_ordered)
    {
        foreach(auto pp, si->producers)
        {
            if(pp.data() == producer_raw)
            {
                producer = pp;
                break;
            }
        }

        if(!producer.isNull())
            break;
    }

    if(!unshelve_queue.contains(producer))
        unshelve_queue.enqueue(producer);
    if(unshelve_queue.length() == 1)
        QTimer::singleShot(100, this, &MainWindow::slot_process_unshelve_queue);
}

void MainWindow::slot_process_shelve_queue()
{
    if(!shelve_queue.length())
        return;     // shouldn't happen, but just in case...

    if(!lane_manager->anim_in_progress())
    {
        auto producer = shelve_queue.head();
        if(!producer.isNull())
        {
            if(producer->is_covering_story() && !producer->is_story_shelved())
                producer->shelve_story();
        }
        (void)shelve_queue.dequeue();
    }

    if(shelve_queue.length())
        QTimer::singleShot(100, this, &MainWindow::slot_process_shelve_queue);
}

void MainWindow::slot_process_unshelve_queue()
{
    if(!unshelve_queue.length())
        return;     // shouldn't happen, but just in case...

    if(!lane_manager->anim_in_progress())
    {
        auto producer = unshelve_queue.head();
        if(!producer.isNull())
        {
            if(producer->is_story_shelved())
                producer->start_covering_story();
        }
        (void)unshelve_queue.dequeue();
    }

    if(unshelve_queue.length())
        QTimer::singleShot(100, this, &MainWindow::slot_process_unshelve_queue);
}
