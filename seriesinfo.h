#pragma once

#include "specialize.h"
#include "staffinfo.h"

struct SeriesInfo
{
    // Series are collections of related Stories
    QString             name;
    StaffMap            staff;
};
SPECIALIZE_MAP(QString, SeriesInfo, Series)         // "SeriesMap"
