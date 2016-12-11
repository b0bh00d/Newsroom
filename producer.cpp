#include "producer.h"

Producer::Producer(
                   QObject* reporter_plugin,
                   const QUrl& story,
                   const QFont& font,
                   const QString& normal_stylesheet,
                   const QString& alert_stylesheet,
                   const QStringList& alert_keywords,
                   LocalTrigger trigger_type,
                   QObject *parent)
    : reporter_plugin(reporter_plugin),
      story(story),
      headline_font(font),
      headline_stylesheet_normal(normal_stylesheet),
      headline_stylesheet_alert(alert_stylesheet),
      headline_alert_keywords(alert_keywords),
      trigger_type(trigger_type),
      QObject(parent)
{
}

bool Producer::start_covering_story()
{
    if(!reporter_plugin)
        return false;

    IPlugin* plugin = reinterpret_cast<IPlugin *>(reporter_plugin);
    if(!plugin)
        return false;

    connect(plugin, &IPlugin::signal_new_data, this, &Producer::slot_new_data);
    plugin->SetStory(story);
    plugin->SetCoverage(trigger_type == LocalTrigger::FileChange);
    if(!plugin->CoverStory())
    {
        disconnect(plugin, &IPlugin::signal_new_data, this, &Producer::slot_new_data);
        return false;
    }

    return true;
}

bool Producer::stop_covering_story()
{
    if(!reporter_plugin)
        return false;

    IPlugin* plugin = qobject_cast<IPlugin *>(reporter_plugin);
    if(!plugin)
        return false;

    disconnect(plugin, &IPlugin::signal_new_data, this, &Producer::slot_new_data);
    return plugin->FinishStory();
}

void Producer::save(QSettings& /*settings*/)
{
}

void Producer::load(QSettings& /*settings*/)
{
}

void Producer::slot_new_data(const QByteArray& data)
{
    // file a headline with the new content
    HeadlinePointer headline(new Headline(story, QString(data)));

    headline->set_font(headline_font);
    headline->set_normal_stylesheet(headline_stylesheet_normal);
    headline->set_alert_stylesheet(headline_stylesheet_alert);
    headline->set_alert_keywords(headline_alert_keywords);

    emit signal_new_headline(headline);
}

//void Producer::slot_poll()
//{
//    if(!reporter_plugin)
//    {
//        target.refresh();

//        if(stabilize_count > 0 && target.size() == last_size)
//        {
//            stabilize_count = 0;

//            QString report = QString("Story '%1' was updated on %2").arg(story.toString()).arg(target.lastModified().toString());
//            HeadlinePointer headline(new Headline(story, report));

//            headline->set_font(headline_font);
//            headline->set_normal_stylesheet(headline_stylesheet_normal);
//            headline->set_alert_stylesheet(headline_stylesheet_alert);
//            headline->set_alert_keywords(headline_alert_keywords);

//            emit signal_new_headline(headline);
//        }
//        else
//        {
//            if(target.size() > last_size)
//            {
//                last_size = target.size();  // wait for the file to stop changing
//                ++stabilize_count;
//            }
//            else if(target.size() < last_size)
//            {
//                // the file got smaller since we last checked, so reset
//                last_size = target.size();
//                stabilize_count = 0;
//            }
//        }
//    }
////    else
////    {
////        IPlugin* plugin = qobject_cast<IPlugin *>(reporter_plugin);
////        if(plugin)
////        {
////            QString report = plugin->Headline();
////            while(!report.isEmpty())
////            {
////                HeadlinePointer headline(new Headline(story, report));

////                headline->set_font(headline_font);
////                headline->set_normal_stylesheet(headline_stylesheet_normal);
////                headline->set_alert_stylesheet(headline_stylesheet_alert);
////                headline->set_alert_keywords(headline_alert_keywords);

////                emit signal_new_headline(headline);

////                report = plugin->Headline();
////            }
////        }
////    }
//}
