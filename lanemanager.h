#pragma once

#include <QObject>

#include <QtCore/QRect>

#include "types.h"
#include "specialize.h"

class Chyron;

class LaneManager : public QObject
{
    Q_OBJECT
public:
    explicit LaneManager(QObject *parent = 0);

    void    subscribe(Chyron* chyron);
    void    unsubscribe(Chyron* chyron);

    const QRect&  get_base_lane_position(Chyron* chyron);
    QRect&  get_lane_boundaries(Chyron* chyron);

private:    // typedefs and enums
    struct LaneData
    {
        Chyron*     owner;
        QRect       lane;               ///< This is a static value that marks the lane position
        QRect       lane_boundaries;    ///< This is passed to the Chyron for modification, and is based on 'lane'
    };

    SPECIALIZE_SHAREDPTR(LaneData, LaneData)        // "LaneDataPointer"
    SPECIALIZE_LIST(LaneDataPointer, Lane)          // "LaneList"
    SPECIALIZE_MAP(AnimEntryType, LaneList, Lane)   // "LaneMap"
    SPECIALIZE_MAP(Chyron*, LaneDataPointer, Data)  // "DataMap"

private:    // methods
    void        calculate_base_lane_position(Chyron* chyron);

private:    // datamembers
    LaneMap     lane_map;
    DataMap     data_map;
};

SPECIALIZE_SHAREDPTR(LaneManager, LaneManager)      // "LaneManagerPointer"
