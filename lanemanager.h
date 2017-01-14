#pragma once

#include <QObject>

#include <QtCore/QRect>

#include "types.h"
#include "specialize.h"

#include "headline.h"
#include "storyinfo.h"

class Chyron;

/// @class LaneManager
/// @brief Manages lane positions for Chyrons
///
/// The LaneManager is responsible for positioning Chyrons on the screen.
/// It will account for Chyrons of the same type, and stack lanes
/// intelligently.

class LaneManager : public QObject
{
    Q_OBJECT
public:
    explicit LaneManager(const QFont& font, const QString& stylesheet, QObject *parent = 0);

    void    subscribe(Chyron* chyron);
    void    unsubscribe(Chyron* chyron);

    const QRect&  get_base_lane_position(Chyron* chyron);
    QRect&  get_lane_boundaries(Chyron* chyron);

private:    // typedefs and enums
    struct LaneData
    {
        Chyron*     owner;
        QRect       lane;               // This is a static value that marks the lane position
        QRect       lane_boundaries;    // This is passed to the Chyron for modification, and is based on 'lane'
    };
    SPECIALIZE_SHAREDPTR(LaneData, LaneData)        // "LaneDataPointer"
    SPECIALIZE_LIST(LaneDataPointer, Lane)          // "LaneList"

    struct DashboardData
    {
        QString     id;
        LaneList    chyrons;
        HeadlinePointer lane_header;    // Small headline that will display the group id
    };
    SPECIALIZE_SHAREDPTR(DashboardData, Dashboard)  // "DashboardPointer"
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
};

SPECIALIZE_SHAREDPTR(LaneManager, LaneManager)      // "LaneManagerPointer"
