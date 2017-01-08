#pragma once

#include "specialize.h"
#include "chyron.h"
#include "producer.h"
#include "reporters/interfaces/ireporter.h"

struct StaffInfo
{
    // Staff and equipment assigned to cover a Story
    IReporterPointer    reporter;
    ProducerPointer     producer;
    ChyronPointer       chyron;
};
SPECIALIZE_MAP(StoryInfoPointer, StaffInfo, Staff)      // "StaffMap"
