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
};
SPECIALIZE_LIST(SeriesInfo, SeriesInfo)             // "SeriesInfoList"
