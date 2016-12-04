#pragma once

#include "headline.h"
#include "reporter.h"

/// @class ReporterLocal
/// @brief A Reporter that covers a local story
///
/// This Reporter is a specialization that that covers local stories (files).

class ReporterLocal : public Reporter
{
    Q_OBJECT
public:
    explicit ReporterLocal(const QUrl& story,
                           const QFont& font,
                           const QString& normal_stylesheet,
                           const QString& alert_stylesheet,
                           const QStringList& alert_keywords,
                           LocalTrigger trigger_type,
                           QObject *parent = 0);

    virtual void    start_covering_story();
    virtual void    stop_covering_story();

    virtual void    save(QSettings& settings);
    virtual void    load(QSettings& settings);

protected slots:
    void            slot_poll();

protected:
    LocalTrigger    trigger_type;

    QFileInfo       target;
    int             stabilize_count;
//    QDateTime       last_modified;
    qint64          seek_offset;
    qint64          last_size;

    QTimer*         poll_timer;

    QString         report;
};
