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
#include "storyinfo.h"
#include "chyron.h"

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
    explicit Producer(ChyronPointer chyron,
                      IReporterPointer reporter,
                      StoryInfoPointer story_info,
                      StyleListPointer style_list,
                      QObject *parent = 0);
    ~Producer();

    StoryInfoPointer get_story()    const { return story_info; }
    IReporterPointer get_reporter() const { return reporter; }
    ChyronPointer    get_chyron()   const { return chyron; }

    bool    is_covering_story()     const { return covering_story; }
    bool    start_covering_story();
    bool    stop_covering_story();

signals:
    void    signal_new_headline(HeadlinePointer headline);

public slots:
    void    slot_start_covering_story();        // used for delayed starts

protected slots:
    void    slot_new_data(const QByteArray& data);
    void    slot_headline_going_out_of_scope(HeadlinePointer);
    void    slot_headline_highlight(qreal opacity, int timeout);
//    void    slot_poll();

private:        // typedefs and enums
    SPECIALIZE_LIST(HeadlinePointer, Headline)   // "HeadlineList"

private:        // methods
    void    file_headline(const QString& data);

private:        // data members
    bool                covering_story;

    IReporterPointer    reporter;
    ChyronPointer       chyron;
    StoryInfoPointer    story_info;
    StyleListPointer    style_list;

    HeadlineList        headlines;
};

SPECIALIZE_SHAREDPTR(Producer, Producer)    // "ProducerPointer"
//Q_DECLARE_METATYPE(ProducerPointer)
