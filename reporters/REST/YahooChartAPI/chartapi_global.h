#pragma once

#include <QtCore/qglobal.h>

#if defined(YAHOOCHARTAPI_LIBRARY)
#  define YAHOOCHARTAPISHARED_EXPORT Q_DECL_EXPORT
#else
#  define YAHOOCHARTAPISHARED_EXPORT Q_DECL_IMPORT
#endif

enum class TickerFormat
{
    None,
    CSV,
    JSON,
    XML,
};
