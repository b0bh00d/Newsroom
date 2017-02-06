#pragma once

#include "specialize.h"
#include "producer.h"

SPECIALIZE_LIST(ProducerPointer, Producer)          // "ProducerList"
struct SeriesInfo
{
    // Series are collections of related Stories
    // (entirely contained within Producers)

    QString             name;
    ProducerList        producers;

    bool                compact_mode;
    int                 compact_compression;

    SeriesInfo()
        : compact_mode(false),
          compact_compression(25)
    {}
};
SPECIALIZE_SHAREDPTR(SeriesInfo, SeriesInfo)        // "SeriesInfoPointer"
SPECIALIZE_LIST(SeriesInfoPointer, SeriesInfo)      // "SeriesInfoList"

Q_DECLARE_METATYPE(SeriesInfoPointer)
