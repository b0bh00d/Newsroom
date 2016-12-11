#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include "textfile.h"

QStringList TextFile::DisplayName() const
{
    return QStringList() << QStringLiteral("Text File") << QStringLiteral("Reads a text file containing ASCII characters");
}

QByteArray TextFile::PluginID() const
{
    return "{F1949758-2A08-4E8A-8290-90DCD270A8B9}";
}

bool TextFile::Supports(const QString& /*file*/) const
{
    return true;
}

void TextFile::SetStory(const QString& file)
{
    target.setFile(file);
}

bool TextFile::CoverStory()
{
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

QString TextFile::Headline()
{
    return QString();
}

void TextFile::slot_poll()
{
    target.refresh();

    if(stabilize_count > 0 && target.size() == last_size)
    {
        stabilize_count = 0;
//        last_modified = target.lastModified();

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
