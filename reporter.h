#pragma once

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include "types.h"
#include "specialize.h"
#include "headline.h"

/// @class Reporter
/// @brief Covers a story
///
/// The Reporter class covers an assigned story, submitting Headlines for
/// display on the Chyron.
///
/// This is an Interface class that other types of Reporter inherit from.

class Reporter : public QObject
{
    Q_OBJECT
public:
    explicit Reporter(const QUrl& story,
                      const QFont& font,
                      const QString& normal_stylesheet,
                      const QString& alert_stylesheet,
                      const QStringList& alert_keywords,
                      QObject *parent = 0);

    QUrl            get_story()         const   { return story; }

    virtual void    start_covering_story() = 0;
    virtual void    stop_covering_story() = 0;

    virtual void    save(QSettings& settings) = 0;
    virtual void    load(QSettings& settings) = 0;

signals:
    void            signal_new_headline(HeadlinePointer headline);

protected:
    QUrl            story;

    QFont           headline_font;
    QString         headline_stylesheet_normal;
    QString         headline_stylesheet_alert;
    QStringList     headline_alert_keywords;
};

SPECIALIZE_SHAREDPTR(Reporter, Reporter)        // "ReporterPointer"
