#pragma once

#include <QWidget>

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QParallelAnimationGroup>

#include "types.h"
#include "specialize.h"
#include "storyinfo.h"

#include "lanemanager.h"
#include "headline.h"

/// @class Chyron
/// @brief Manages headlines submitted by Reporters
///
/// The Chyron class manages the display of headlines submitted by Reporters.

#ifdef HIGHLIGHT_LANES
class HighlightWidget;
#endif

class Chyron : public QObject
{
    Q_OBJECT
public:
    explicit Chyron(StoryInfoPointer story_info,
                    LaneManagerPointer lane_manager,
                    QObject* parent = nullptr);
    ~Chyron();

    void        display();      // Begins displaying Headlines
    void        hide();         // Stops displaying Headlines (and destroys existing)
    void        shelve();       // Stops displaying Headlines (and destroys existing)

    void        suspend();      // Temporarily stops accepting new Headlines
    void        resume();       // Starts accepting new Headlines

    StoryInfoPointer get_settings()      const   { return story_info; }

    // These methods are used by the Dashboard to adjust lanes
    // when a Chyron is deleted.  This does an immediate move of
    // any visible Headlines in the current lane.

    void                unsubscribed();
    QAbstractAnimation* shift_left(int amount, bool auto_start = true);
    QAbstractAnimation* shift_right(int amount, bool auto_start = true);
    QAbstractAnimation* shift_up(int amount, bool auto_start = true);
    QAbstractAnimation* shift_down(int amount, bool auto_start = true);

    // This method is used by the Producer to signal to the Chyron that a
    // Reporter-drawn Headline needs to be highlighted by adjusting its opacity.
    void        highlight_headline(HeadlinePointer hl, qreal opacity, int timeout);

signals:
    void        signal_headline_going_out_of_scope(HeadlinePointer headline);

public slots:
    void        slot_file_headline(HeadlinePointer headline);

protected slots:
    void        slot_age_headlines();
    void        slot_headline_posted();
    void        slot_headline_expired();
    void        slot_train_expire_headlines();

    void        slot_headline_mouse_enter();
    void        slot_headline_mouse_exit();

protected:  // typedefs and enums
    SPECIALIZE_LIST(HeadlinePointer, Headline)          // "HeadlineList"
    SPECIALIZE_QUEUE(HeadlinePointer, Transition)       // "TransitionQueue"
    SPECIALIZE_MAP(QPropertyAnimation*, HeadlinePointer, PropertyAnimation) // "PropertyAnimationMap"
    SPECIALIZE_MAP(HeadlinePointer, bool, Entering)     // "EnteringMap"
    SPECIALIZE_MAP(HeadlinePointer, bool, Exiting)      // "ExitingMap"
    SPECIALIZE_MAP(Headline*, double, Opacity)          // "OpacityMap"

protected:  // methods
    void        initialize_headline(HeadlinePointer headline);
    void        start_headline_entry(HeadlinePointer headline);
    void        start_headline_exit(HeadlinePointer headline);
    void        dashboard_expire_headlines();
    void        headline_posted(HeadlinePointer headline);

protected:  // data members
    StoryInfoPointer story_info;

    QTimer*         age_timer{nullptr};

    TransitionQueue incoming_headlines;
    HeadlineList    headline_list;
    HeadlineList    reduce_list;

    PropertyAnimationMap prop_anim_map;
    EnteringMap     entering_map;
    ExitingMap      exiting_map;

    LaneManagerPointer  lane_manager;

    OpacityMap      opacity_map;

#ifdef HIGHLIGHT_LANES
    HighlightWidget*    highlight{nullptr};
#endif

    bool            visible{true};
    bool            suspended{false};
};
SPECIALIZE_SHAREDPTR(Chyron, Chyron)        // "ChyronPointer"
