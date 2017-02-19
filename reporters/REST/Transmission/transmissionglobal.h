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
    IgnoreFinished = 0x1,
    IgnoreStopped  = 0x2,
    IgnoreIdle     = 0x4,
} Interest;
