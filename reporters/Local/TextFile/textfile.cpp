#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>

#include "textfile.h"

TextFile::TextFile(QObject *parent)
    : poll_timer(nullptr),
      trigger(LocalTrigger::NewContent),
      left_strip(0),
      right_strip(0),
      IReporter(parent)
{}

// IPlugin
QStringList TextFile::DisplayName() const
{
    return QStringList() << QObject::tr("Text File (Log)") <<
           QObject::tr("Reads a slow-to-moderately updated text file from the local\n"
                       "disc.  Assumes text is appended to the end of the file.");
}

QByteArray TextFile::PluginID() const
{
    return "{F1949758-2A08-4E8A-8290-90DCD270A8B9}";
}

bool TextFile::Supports(const QUrl& entity) const
{
    if(!entity.isLocalFile() || !QFile::exists(entity.toLocalFile()))
        return false;

    // Here we should peek at the file contents to make sure
    // it is a format we can grok, but for now, we'll just
    // assume and hope for the best...

    return true;
}

QStringList TextFile::Requires() const
{
    return QStringList() << "New headlines are triggered by" << "combo:new content,file changes" <<
                            "Strip characters from left" << "integer:0" <<
                            "Strip characters from right" << "integer:0";
}

bool TextFile::SetRequirements(const QStringList& parameters)
{
    error_message.clear();

    if(parameters.count() < Param::Count)
    {
        error_message = QStringLiteral("TextFile: Not enough parameters provided.");
        return false;
    }

    trigger = static_cast<LocalTrigger>(parameters[Param::Trigger].toInt());
    left_strip = parameters[Param::LeftStrip].toInt();
    right_strip = parameters[Param::RightStrip].toInt();

    return true;
}

void TextFile::SetStory(const QUrl& story)
{
    this->story = story;
    if(story.isLocalFile())
        target.setFile(story.toLocalFile());
}

bool TextFile::CoverStory()
{
    if(poll_timer)
        return false;           // calling us a second time

//    if(!story.isValid() || !story.isLocalFile())
//        return false;

    if(!target.exists())
        return false;
    target.refresh();
    stabilize_count = 0;

//    last_modified = target.lastModified();
    last_size = seek_offset = target.size();

    poll_timer = new QTimer(this);
    poll_timer->setInterval(1000);
    connect(poll_timer, &QTimer::timeout, this, &TextFile::slot_poll);
    poll_timer->start();

    return true;
}

bool TextFile::FinishStory()
{
    if(poll_timer)
    {
        poll_timer->stop();
        poll_timer->deleteLater();
        poll_timer = nullptr;
    }

    return true;
}

void TextFile::preprocess(QByteArray& ba)
{
    if(!left_strip && !right_strip)
        return;     // no modifications

    QString str(ba);
    QStringList lines = str.split('\n');

    if(left_strip)
    {
        for(int i = 0;i < lines.length();++i)
            lines[i].remove(0, left_strip);
    }

    if(right_strip)
    {
        for(int i = 0;i < lines.length();++i)
        {
            if(lines[i].length() > right_strip)
                lines[i] = lines[i].left(lines[i].length() - right_strip);
            else
                lines[i].clear();
        }
    }

    ba = lines.join('\n').toUtf8();
}

void TextFile::slot_poll()
{
    target.refresh();

    if(stabilize_count > 0 && target.size() == last_size)
    {
        stabilize_count = 0;

        if(trigger == LocalTrigger::FileChange)
        {
            // this is enough to trigger a headline
            QString data = QString("Story '%1' was updated on %2").arg(story.toString()).arg(target.lastModified().toString());
            emit signal_new_data(data.toUtf8());
        }
        else
        {
            if(target.size() > seek_offset)
            {
                // gather the new content
                QFile target_file(target.absoluteFilePath());
                if(target_file.open(QIODevice::ReadOnly|QIODevice::Text))
                {
                    target_file.seek(seek_offset);
                    QByteArray data = target_file.readAll();
                    preprocess(data);
                    emit signal_new_data(data);
                }
            }

            last_size = seek_offset = target.size();
        }
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
            last_size = seek_offset = target.size();
            stabilize_count = 0;
        }
    }
}
