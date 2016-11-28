#include "reporter.h"

Reporter::Reporter(const QUrl& story, QObject *parent)
    : story(story),
      QObject(parent)
{
}
