#include "producer.h"

Producer::Producer(IReporterPointer reporter,
                   StoryInfoPointer story_info,
                   QObject *parent)
    : reporter_plugin(reporter),
      story_info(story_info),
      QObject(parent)
{
}

bool Producer::start_covering_story()
{
    if(reporter_plugin.isNull())
        return false;

    connect(reporter_plugin.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
    reporter_plugin->SetStory(story_info->story);
    reporter_plugin->SetCoverage(story_info->trigger_type == LocalTrigger::FileChange);
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

void Producer::file_headline(const QString& data)
{
    // file a headline with the new content
    HeadlinePointer headline(new Headline(story_info->story, data));

    headline->set_font(story_info->font);
    headline->set_normal_stylesheet(story_info->normal_stylesheet);
    headline->set_alert_stylesheet(story_info->alert_stylesheet);
    headline->set_alert_keywords(story_info->alert_keywords);

    emit signal_new_headline(headline);
}

void Producer::slot_new_data(const QByteArray& data)
{
    if(!story_info->limit_content)
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
        if((current_limit % story_info->limit_content_to) == 0)
        {
            if(!new_line.isEmpty())
                file_headline(new_line);
            new_line.clear();
        }
    }

    if(!new_line.isEmpty())
        file_headline(new_line);
}
