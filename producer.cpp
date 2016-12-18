#include "producer.h"

Producer::Producer(IReporterPointer reporter,
                   const QUrl& story,
                   const QFont& font,
                   const QString& normal_stylesheet,
                   const QString& alert_stylesheet,
                   const QStringList& alert_keywords,
                   LocalTrigger trigger_type,
                   bool limit_content,
                   int limit_content_to,
                   QObject *parent)
    : reporter_plugin(reporter),
      story(story),
      headline_font(font),
      headline_stylesheet_normal(normal_stylesheet),
      headline_stylesheet_alert(alert_stylesheet),
      headline_alert_keywords(alert_keywords),
      trigger_type(trigger_type),
      limit_content(limit_content),
      limit_content_to(limit_content_to),
      QObject(parent)
{
}

bool Producer::start_covering_story()
{
    if(reporter_plugin.isNull())
        return false;

    connect(reporter_plugin.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
    reporter_plugin->SetStory(story);
    reporter_plugin->SetCoverage(trigger_type == LocalTrigger::FileChange);
    if(!reporter_plugin->CoverStory())
    {
        disconnect(reporter_plugin.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
        return false;
    }

    return true;
}

bool Producer::stop_covering_story()
{
    if(reporter_plugin.isNull())
        return false;

    disconnect(reporter_plugin.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
    return reporter_plugin->FinishStory();
}

void Producer::save(QSettings& /*settings*/)
{
}

void Producer::load(QSettings& /*settings*/)
{
}

void Producer::file_headline(const QString& data)
{
    // file a headline with the new content
    HeadlinePointer headline(new Headline(story, data));

    headline->set_font(headline_font);
    headline->set_normal_stylesheet(headline_stylesheet_normal);
    headline->set_alert_stylesheet(headline_stylesheet_alert);
    headline->set_alert_keywords(headline_alert_keywords);

    emit signal_new_headline(headline);
}

void Producer::slot_new_data(const QByteArray& data)
{
    if(!limit_content)
    {
        file_headline(QString(data));
        return;
    }

    bool has_breaks = data.contains("<br>");

    QStringList lines;
    if(has_breaks)
        lines = QString(data).split("<br>");
    else
        lines = QString(data).split('\n');

    QString new_line;
    int current_limit = 0;
    foreach(const QString& line, lines)
    {
        if(new_line.length())
        {
            if(has_breaks)
                new_line += "<br>";
            else
                new_line += "\n";
        }

        new_line += line;

        ++current_limit;
        if((current_limit % limit_content_to) == 0)
        {
            if(!new_line.isEmpty())
                file_headline(new_line);
            new_line.clear();
        }
    }

    if(!new_line.isEmpty())
        file_headline(new_line);
}
