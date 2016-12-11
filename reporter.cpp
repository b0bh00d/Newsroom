#include "reporter.h"

ReporterWrapper::ReporterWrapper(
                   QObject* reporter_plugin,
                   const QUrl& story,
                   const QFont& font,
                   const QString& normal_stylesheet,
                   const QString& alert_stylesheet,
                   const QStringList& alert_keywords,
                   LocalTrigger trigger_type,
                   QObject *parent)
    : reporter_plugin(reporter_plugin),
      local_instance(nullptr),
      REST_instance(nullptr),
      story(story),
      headline_font(font),
      headline_stylesheet_normal(normal_stylesheet),
      headline_stylesheet_alert(alert_stylesheet),
      headline_alert_keywords(alert_keywords),
      trigger_type(trigger_type),
      QObject(parent)
{
    if(!reporter_plugin)
        target.setFile(story.toLocalFile());
}

void ReporterWrapper::start_covering_story()
{
    if(!reporter_plugin)
    {
        target.refresh();
        stabilize_count = 0;

        last_size = target.size();

        poll_timer = new QTimer(this);
        poll_timer->setInterval(1000);
        connect(poll_timer, &QTimer::timeout, this, &ReporterWrapper::slot_poll);
        poll_timer->start();
    }
    else
    {
        IPlugin* plugin = qobject_cast<IPlugin *>(reporter_plugin);
        if(plugin)
        {
            connect(plugin, &IPlugin::signal_new_data, this, &ReporterWrapper::slot_new_data);
            plugin->CoverStory();
        }
    }
}

void ReporterWrapper::stop_covering_story()
{
    if(!reporter_plugin)
        poll_timer->stop();
    else
    {
        IPlugin* plugin = qobject_cast<IPlugin *>(reporter_plugin);
        if(plugin)
        {
            disconnect(plugin, &IPlugin::signal_new_data, this, &ReporterWrapper::slot_new_data);
            plugin->FinishStory();
        }
    }
}

void ReporterWrapper::save(QSettings& /*settings*/)
{
}

void ReporterWrapper::load(QSettings& /*settings*/)
{
}

void ReporterWrapper::slot_new_data(const QByteArray& data)
{
    // file an headline with the new content
    HeadlinePointer headline(new Headline(story, QString(data)));

    headline->set_font(headline_font);
    headline->set_normal_stylesheet(headline_stylesheet_normal);
    headline->set_alert_stylesheet(headline_stylesheet_alert);
    headline->set_alert_keywords(headline_alert_keywords);

    emit signal_new_headline(headline);
}

void ReporterWrapper::slot_poll()
{
    if(!reporter_plugin)
    {
        target.refresh();

        if(stabilize_count > 0 && target.size() == last_size)
        {
            stabilize_count = 0;

            QString report = QString("Story '%1' was updated on %2").arg(story.toString()).arg(target.lastModified().toString());
            HeadlinePointer headline(new Headline(story, report));

            headline->set_font(headline_font);
            headline->set_normal_stylesheet(headline_stylesheet_normal);
            headline->set_alert_stylesheet(headline_stylesheet_alert);
            headline->set_alert_keywords(headline_alert_keywords);

            emit signal_new_headline(headline);
        }
        else
        {
            if(target.size() > last_size)
            {
                last_size = target.size();  // wait for the file to stop changing
                ++stabilize_count;
            }
            else if(target.size() < last_size)
            {
                // the file got smaller since we last checked, so reset
                last_size = target.size();
                stabilize_count = 0;
            }
        }
    }
    else
    {
        IPlugin* plugin = qobject_cast<IPlugin *>(reporter_plugin);
        if(plugin)
        {
            QString report = plugin->Headline();
            while(!report.isEmpty())
            {
                HeadlinePointer headline(new Headline(story, report));

                headline->set_font(headline_font);
                headline->set_normal_stylesheet(headline_stylesheet_normal);
                headline->set_alert_stylesheet(headline_stylesheet_alert);
                headline->set_alert_keywords(headline_alert_keywords);

                emit signal_new_headline(headline);

                report = plugin->Headline();
            }
        }
    }
}
