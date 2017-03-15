#pragma once

#include <QObject>

#include <QtGui/QFont>

#include <QtCore/QString>

#include "lanedata.h"
#include "headline.h"
#include "storyinfo.h"

/// @class Dashboard
/// @brief Manages multiple Chyrons in a Dashboard configuration
///
/// A Dashboard is a special kind of Chyron manager within the LaneManager
/// paradigm.  It stacks its Chyrons in order, decorates them with a header,
/// and manages their presence or absence in a fluid manner.
///
/// This class is only used--and usable--by the LaneManager.
///
/// (This really should be a nested LaneManager class, but since Qt cannot
/// currently handle nested QObject-derived classes with Signals and Slots,
/// it has to be in a "public" header.)

class QAbstractAnimation;

class Dashboard: public QObject
{
    Q_OBJECT
public:
    Dashboard(StoryInfoPointer story_info, const QFont& headline_font, const QString& headline_stylesheet, QObject *parent = nullptr);
    ~Dashboard();

    bool    is_managing(Chyron* chyron);
    bool    is_empty();
    bool    is_id(const QString& id);

    void    add_lane(LaneDataPointer lane);
    void    remove_lane(LaneDataPointer lane, UnsubscribeAction action);
    void    shift(LaneDataPointer exiting);
    void    calculate_base_lane_position(LaneDataPointer data, const QRect& r_desktop, int r_offset_w, int r_offset_h);

    QRect   get_header_geometry();
    void    get_header_geometry(QRect& r);
    void    get_header_geometry(int& x, int& y, int& w, int& h);
    QPoint  get_header_position();
    QSize   get_header_size();

    void    anim_queue(LaneDataPointer lane, QAbstractAnimation* anim);
    void    anim_clear(LaneDataPointer lane);
    bool    anim_in_progress();

signals:
    void    signal_chyron_unsubscribed(LaneDataPointer lane);
//    void    signal_unsubscribe_chyron(LaneDataPointer lane);
    void    signal_empty(LaneDataPointer lane);
    void    signal_animation_started();
    void    signal_animation_completed();

protected slots:
    void    slot_animation_complete();
    void    slot_process_anim_queue();

protected:      // typedefs and enums
    SPECIALIZE_PAIR(LaneDataPointer, QAbstractAnimation*, Anim) // "AnimPair"
    SPECIALIZE_LIST(AnimPair, Anim)                 // "AnimList"
    SPECIALIZE_QUEUE(LaneDataPointer, Unsubscribe)  // "UnsubscribeQueue"

protected:    // methods
    void    _remove_lane(LaneDataPointer lane);
    void    process_unsubscribe_queue();
    void    animation_complete();
    void    resume_chyrons();

protected:      // data members
    QString     id;
    LaneList    chyrons;
    HeadlinePointer lane_header;    // Small headline that will display the group id

    AnimList    anim_tracker;
    UnsubscribeQueue unsubscribe_queue;
};
SPECIALIZE_SHAREDPTR(Dashboard, Dashboard)          // "DashboardPointer"
