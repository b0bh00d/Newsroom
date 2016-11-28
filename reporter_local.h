#pragma once

#include "article.h"
#include "reporter.h"

class ReporterLocal : public Reporter
{
    Q_OBJECT
public:
    explicit ReporterLocal(const QUrl& story, LocalTrigger trigger_type, QObject *parent = 0);

    virtual ArticlePointer get_article() const;

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
