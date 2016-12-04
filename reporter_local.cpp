#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include "reporter_local.h"

ReporterLocal::ReporterLocal(const QUrl& story,
                             const QFont& font,
                             const QString& normal_stylesheet,
                             const QString& alert_stylesheet,
                             const QStringList& alert_keywords,
                             LocalTrigger trigger_type,
                             QObject *parent)
    : trigger_type(trigger_type),
      Reporter(story, font, normal_stylesheet, alert_stylesheet, alert_keywords, parent)
{
    target.setFile(story.toLocalFile());
}

void ReporterLocal::start_covering_story()
{
    target.refresh();
    stabilize_count = 0;

//    last_modified = target.lastModified();
    last_size = seek_offset = target.size();

    poll_timer = new QTimer(this);
    poll_timer->setInterval(1000);
    connect(poll_timer, &QTimer::timeout, this, &ReporterLocal::slot_poll);
    poll_timer->start();
}

void ReporterLocal::stop_covering_story()
{
    poll_timer->stop();
}

void ReporterLocal::save(QSettings& /*settings*/)
{
}

void ReporterLocal::load(QSettings& /*settings*/)
{
}

void ReporterLocal::slot_poll()
{
    target.refresh();

    if(stabilize_count > 0 && target.size() == last_size)
    {
        stabilize_count = 0;
//        last_modified = target.lastModified();

        if(trigger_type == LocalTrigger::FileChange)
        {
            // this is enough to trigger an headline
            report = QString("Story '%1' was updated on %2").arg(story.toString()).arg(target.lastModified().toString());
            HeadlinePointer headline(new Headline(story, report));

            headline->set_font(headline_font);
            headline->set_normal_stylesheet(headline_stylesheet_normal);
            headline->set_alert_stylesheet(headline_stylesheet_alert);
            headline->set_alert_keywords(headline_alert_keywords);

            emit signal_new_headline(headline);
        }
        else if(trigger_type == LocalTrigger::NewContent)
        {
            if(target.size() > seek_offset)
            {
                // gather the new content
                QFile target_file(target.absoluteFilePath());
                if(target_file.open(QIODevice::ReadOnly|QIODevice::Text))
                {
                    target_file.seek(seek_offset);
                    QByteArray data = target_file.readAll();

                    // file an headline with the new content
                    HeadlinePointer headline(new Headline(story, QString(data)));

                    headline->set_font(headline_font);
                    headline->set_normal_stylesheet(headline_stylesheet_normal);
                    headline->set_alert_stylesheet(headline_stylesheet_alert);
                    headline->set_alert_keywords(headline_alert_keywords);

                    emit signal_new_headline(headline);
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
