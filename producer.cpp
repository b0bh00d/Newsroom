#include "producer.h"

Producer::Producer(ChyronPointer chyron,
                   IReporterPointer reporter,
                   StoryInfoPointer story_info,
                   StyleListPointer style_list,
                   QObject *parent)
    : chyron(chyron),
      reporter(reporter),
      covering_story(false),
      story_info(story_info),
      style_list(style_list),
      QObject(parent)
{
}

Producer::~Producer()
{
    if(covering_story)
        stop_covering_story();

    chyron.clear();
    reporter.clear();
}

bool Producer::start_covering_story()
{
    if(covering_story)
        return true;

    if(reporter.isNull() || chyron.isNull())
        return false;

    connect(this, &Producer::signal_new_headline, chyron.data(), &Chyron::slot_file_headline);
    chyron->display();

    connect(reporter.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
    reporter->SetStory(story_info->story);
    if(!reporter->CoverStory())
    {
        disconnect(reporter.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
        return false;
    }

    covering_story = true;
    return covering_story;
}

bool Producer::stop_covering_story()
{
    if(!covering_story)
        return true;

    if(reporter.isNull() || chyron.isNull())
        return false;

    disconnect(this, &Producer::signal_new_headline, chyron.data(), &Chyron::slot_file_headline);
    chyron->hide();

    disconnect(reporter.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
    covering_story = !reporter->FinishStory();
    return !covering_story;
}

void Producer::file_headline(const QString& data)
{
    // check for keyword triggers, and select the stylesheet appropriately

    QString lower_headline = data.toLower();

    QString stylesheet, default_stylesheet;
    foreach(const HeadlineStyle& style, (*style_list.data()))
    {
        if(!style.name.compare("Default"))
            default_stylesheet = style.stylesheet;
        else
        {
            foreach(const QString& trigger, style.triggers)
            {
                if(lower_headline.contains(trigger.toLower()))
                {
                    stylesheet = style.stylesheet;
                    break;
                }
            }
        }
    }

    if(stylesheet.isEmpty())
        stylesheet = default_stylesheet;    // set to Default

    // file a headline with the new content
    int w, h;
    story_info->get_dimensions(w, h);
    HeadlineGenerator generator(w, h, story_info->story, data);
    HeadlinePointer headline = generator.get_headline();

    headline->set_font(story_info->font);
    headline->set_stylesheet(stylesheet);
    if(story_info->include_progress_bar)
    {
        LandscapeHeadline* ls_headline = dynamic_cast<LandscapeHeadline*>(headline.data());
        if(ls_headline)
            ls_headline->enable_progress_detection(story_info->progress_text_re, story_info->progress_on_top);
    }

    emit signal_new_headline(headline);
}

void Producer::slot_start_covering_story()
{
    (void)start_covering_story();
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
