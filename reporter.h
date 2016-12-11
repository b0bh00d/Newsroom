#pragma once

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <iplugin>

#include "types.h"
#include "specialize.h"
#include "headline.h"

/// @class ReporterWrapper
/// @brief Covers a story
///
/// A Reporter covers an assigned story, submitting Headlines for display
/// on an assigned Chyron.
///
/// ReporterWrapper is a thin wrapper class that interfaces with a Reporter
/// plug-in, and hooks it into the functioning of the Newsroom application.

class ReporterWrapper : public QObject
{
    Q_OBJECT
public:
    explicit ReporterWrapper(QObject* reporter_plugin,
                             const QUrl& story,
                             const QFont& font,
                             const QString& normal_stylesheet,
                             const QString& alert_stylesheet,
                             const QStringList& alert_keywords,
                             LocalTrigger trigger_type,
                             QObject *parent = 0);

    QUrl    get_story() const { return story; }

    bool    start_covering_story();
    bool    stop_covering_story();

    void    save(QSettings& settings);
    void    load(QSettings& settings);

signals:
    void    signal_new_headline(HeadlinePointer headline);

protected slots:
    void    slot_new_data(const QByteArray& data);
    void    slot_poll();

protected:
    QObject*        reporter_plugin;
    IPluginLocal*   local_instance;
    IPluginREST*    REST_instance;

    QUrl            story;

    QFont           headline_font;
    QString         headline_stylesheet_normal;
    QString         headline_stylesheet_alert;
    QStringList     headline_alert_keywords;
    LocalTrigger    trigger_type;

    QTimer*         poll_timer;

    QFileInfo       target;
    int             stabilize_count;
    qint64          last_size;
};

SPECIALIZE_SHAREDPTR(ReporterWrapper, Reporter)    // "ReporterPointer"
