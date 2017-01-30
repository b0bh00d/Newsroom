#pragma once

#include <QtCore/qglobal.h>

#if defined(TEAMCITY9_LIBRARY)
#  define TEAMCITY9SHARED_EXPORT Q_DECL_EXPORT
#else
#  define TEAMCITY9SHARED_EXPORT Q_DECL_IMPORT
#endif

typedef enum
{
    None           = 0x0,
    PendingChanges = 0x1,
} Interest;
