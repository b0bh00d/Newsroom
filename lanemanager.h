#pragma once

#include "types.h"
#include "specialize.h"

#include "dashboard.h"
#include "headline.h"

/// @class LaneManager
/// @brief Manages lane positions for Chyrons
///
/// The LaneManager is responsible for positioning Chyrons on the screen.
/// It will account for Chyrons of the same 'entry type', and stack lanes
/// intelligently.

class LaneManager : public QObject
{
    Q_OBJECT
public:
    explicit LaneManager(const QFont& font, const QString& stylesheet, QObject *parent = 0);
    ~LaneManager();

    void    subscribe(Chyron* chyron);
    void    unsubscribe(Chyron* chyron, UnsubscribeAction action = UnsubscribeAction::Immediate);
    void    shelve(Chyron* chyron);

    void    anim_queue(Chyron *chyron, QAbstractAnimation* anim);
    void    anim_clear(Chyron* chyron);
    bool    anim_in_progress();

    const QRect&  get_base_lane_position(Chyron* chyron);
    QRect&  get_lane_boundaries(Chyron* chyron);

private slots:
    void    slot_dashboard_chyron_unsubscribed(LaneDataPointer lane);
    void    slot_dashboard_empty(LaneDataPointer lane);
    void    slot_animation_started();
    void    slot_animation_completed();

private:    // typedefs and enums
    SPECIALIZE_LIST(DashboardPointer, Dashboard)    // "DashboardList"
    SPECIALIZE_MAP(AnimEntryType, DashboardList, Dashboard)    // "DashboardMap"

    SPECIALIZE_MAP(AnimEntryType, LaneList, Lane)   // "LaneMap"
    SPECIALIZE_MAP(Chyron*, LaneDataPointer, Data)  // "DataMap"

private:    // methods
    void            calculate_base_lane_position(LaneDataPointer data);

private:    // datamembers
    LaneMap         lane_map;
    DataMap         data_map;
    DashboardMap    dashboard_map;

    QFont           headline_font;
    QString         headline_stylesheet;

    int             animation_count;
};

SPECIALIZE_SHAREDPTR(LaneManager, LaneManager)      // "LaneManagerPointer"
