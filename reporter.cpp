#include "reporter.h"

Reporter::Reporter(const QUrl& story,
                   const QFont& font,
                   const QString& normal_stylesheet,
                   const QString& alert_stylesheet,
                   const QStringList& alert_keywords,
                   QObject *parent)
    : story(story),
      headline_font(font),
      headline_stylesheet_normal(normal_stylesheet),
      headline_stylesheet_alert(alert_stylesheet),
      headline_alert_keywords(alert_keywords),
      QObject(parent)
{
}
