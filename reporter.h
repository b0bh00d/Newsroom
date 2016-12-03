#pragma once

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include "types.h"
#include "specialize.h"
#include "headline.h"

class Reporter : public QObject
{
    Q_OBJECT
public:
    explicit Reporter(const QUrl& story = QUrl(), QObject *parent = 0);

    QUrl            get_story()         const   { return story; }

    virtual void    start_covering_story() = 0;
    virtual void    stop_covering_story() = 0;

    virtual void    save(QSettings& settings) = 0;
    virtual void    load(QSettings& settings) = 0;

signals:
    void            signal_new_headline(HeadlinePointer headline);

protected:
    QUrl            story;
};

SPECIALIZE_SHAREDPTR(Reporter, Reporter)        // "ReporterPointer"
