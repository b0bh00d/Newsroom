#include <QtCore/QDebug>
#include <QtCore/QDateTime>

#include "producer.h"

Producer::Producer(ChyronPointer chyron,
                   IReporterPointer reporter,
                   StoryInfoPointer story_info,
                   StyleListPointer style_list,
                   QObject *parent)
    : QObject(parent),
      reporter(reporter),
      chyron(chyron),
      story_info(story_info),
      style_list(style_list)
{
    auto reporter_draw = dynamic_cast<IReporter2*>(reporter.data());
    if(reporter_draw && reporter_draw->UseReporterDraw())
        connect(chyron.data(), &Chyron::signal_headline_going_out_of_scope, this, &Producer::slot_headline_going_out_of_scope);
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
    if(story_shelved)
    {
        chyron->display();
        story_shelved = false;
        return true;
    }

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
    if(!story_shelved)
        chyron->hide();
    story_shelved = false;

    disconnect(reporter.data(), &IReporter::signal_new_data, this, &Producer::slot_new_data);
    covering_story = !reporter->FinishStory();
    return !covering_story;
}

bool Producer::shelve_story()
{
    // we have to already be covering a story to go into
    // 'shelved' mode

    if(!covering_story)
        return false;

    if(reporter.isNull() || chyron.isNull())
        return false;

    chyron->shelve();
    story_shelved = true;

    return story_shelved;
}

void Producer::file_headline(const QString& data)
{
    // check for keyword triggers, and select the stylesheet appropriately

    auto lower_headline = data.toLower();

    QString stylesheet, default_stylesheet;
    foreach(const auto& style, (*style_list.data()))
    {
        if(!style.name.compare("Default"))
            default_stylesheet = style.stylesheet;
        else if(stylesheet.isEmpty())
        {
            foreach(const auto& trigger, style.triggers)
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
    auto w{0};
    auto h{0};
    story_info->get_dimensions(w, h);
    HeadlineGenerator generator(w, h, story_info, data);
    auto headline = generator.get_headline();

    headline->set_stylesheet(stylesheet);

    auto reporter_draw = dynamic_cast<IReporter2*>(reporter.data());
    if(reporter_draw && reporter_draw->UseReporterDraw())
    {
        headline->set_reporter_draw(true);
        connect(headline.data(), &Headline::signal_reporter_draw, reporter_draw, &IReporter2::ReporterDraw);
        connect(reporter_draw, &IReporter2::signal_highlight, this, &Producer::slot_headline_highlight);
        headlines.append(headline);

        connect(reporter_draw, &IReporter2::signal_shelve_story, this, &Producer::signal_shelve_story);
        connect(reporter_draw, &IReporter2::signal_unshelve_story, this, &Producer::signal_unshelve_story);
    }

    emit signal_new_headline(headline);
}

void Producer::slot_headline_going_out_of_scope(HeadlinePointer headline)
{
    foreach(auto hp, headlines)
    {
        if(hp.data() == headline.data())
        {
            headlines.removeAll(hp);
            break;
        }
    }
}

void Producer::slot_headline_highlight(qreal opacity, int timeout)
{
    chyron->highlight_headline(headlines.front(), opacity, timeout);
}

void Producer::slot_start_covering_story()
{
    (void)start_covering_story();
}

void Producer::slot_new_data(const QByteArray& data)
{
    if(story_shelved)
        return;

    if(!story_info->limit_content)
    {
        file_headline(QString(data));
        return;
    }

    auto has_breaks = data.contains("<br>");

    QStringList lines;
    if(has_breaks)
        lines = QString(data).split("<br>");
    else
        lines = QString(data).split('\n');

    QString new_line;
    auto current_limit{0};
    foreach(const auto& line, lines)
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
