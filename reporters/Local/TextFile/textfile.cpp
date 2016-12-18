#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>

#include "textfile.h"

TextFile::TextFile(QObject *parent)
    : poll_timer(nullptr),
      notices_only(false),
      IReporter(parent)
{}

// IPlugin
QStringList TextFile::DisplayName() const
{
    return QStringList() << QObject::tr("Text File") << QObject::tr("Reads a text file containing ASCII characters");
}

QByteArray TextFile::PluginID() const
{
    return "{F1949758-2A08-4E8A-8290-90DCD270A8B9}";
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
    poll_timer->stop();
    poll_timer->deleteLater();
    poll_timer = nullptr;

    return true;
}

// IPluginLocal
bool TextFile::Supports(const QString& /*file*/) const
{
    return true;
}

void TextFile::SetCoverage(bool notices_only)
{
    this->notices_only = notices_only;
}

void TextFile::slot_poll()
{
    target.refresh();

    if(stabilize_count > 0 && target.size() == last_size)
    {
        stabilize_count = 0;
//        last_modified = target.lastModified();

        if(notices_only)
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
