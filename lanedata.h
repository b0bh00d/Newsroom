#pragma once

#include <QtCore/QRect>

#include "types.h"
#include "specialize.h"

class Chyron;

/// @class LaneData
/// @brief Contains lane-specific data for the LaneManager
///
/// This C structure was broken out of LaneManager so it could be referenced
/// by both LaneManager and Dashboard.

struct LaneData
{
    Chyron*     owner;
    QRect       lane;               // This is a static value that marks the lane position
    QRect       lane_boundaries;    // This is passed to the Chyron for modification, and is based on 'lane'

    LaneData() : owner(nullptr) {}
};
SPECIALIZE_SHAREDPTR(LaneData, LaneData)        // "LaneDataPointer"
SPECIALIZE_LIST(LaneDataPointer, Lane)          // "LaneList"
