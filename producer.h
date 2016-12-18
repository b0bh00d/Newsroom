#pragma once

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <ireporter.h>

#include "types.h"
#include "specialize.h"
#include "headline.h"

/// @class Producer
/// @brief Manages a Reporter covering a Story
///
/// A Producer accepts reports from a Reporter and then submits them as Headlines
/// for display on an assigned Chyron.
///
/// The Producer class interfaces with a Reporter plug-in, and hooks it into the
/// functioning of the Newsroom.

class Producer : public QObject
{
    Q_OBJECT
public:
    explicit Producer(IReporterPointer reporter,
                      const QUrl& story,
                      const QFont& font,
                      const QString& normal_stylesheet,
                      const QString& alert_stylesheet,
                      const QStringList& alert_keywords,
                      LocalTrigger trigger_type,
                      bool limit_content,
                      int limit_content_to,
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
//    void    slot_poll();

private:        // methods
    void    file_headline(const QString& data);

private:        // data members
    IReporterPointer  reporter_plugin;

    QUrl            story;

    QFont           headline_font;
    QString         headline_stylesheet_normal;
    QString         headline_stylesheet_alert;
    QStringList     headline_alert_keywords;
    LocalTrigger    trigger_type;

    bool            limit_content;
    int             limit_content_to;
};

SPECIALIZE_SHAREDPTR(Producer, Producer)    // "ProducerPointer"
