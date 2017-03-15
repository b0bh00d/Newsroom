#pragma once

#include <QtCore/qglobal.h>

#if defined(TRANSMISSION_LIBRARY)
#  define TRANSMISSIONSHARED_EXPORT Q_DECL_EXPORT
#else
#  define TRANSMISSIONSHARED_EXPORT Q_DECL_IMPORT
#endif

typedef enum
{
    None           = 0x0,
    ShelveFinished = 0x1,
    ShelveStopped  = 0x2,
    ShelveIdle     = 0x4,
    ShelveEmpty    = 0x8,
} Interest;
