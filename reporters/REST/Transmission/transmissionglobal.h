#pragma once

#include <QtCore/qglobal.h>

#if defined(TRANSMISSION_LIBRARY)
#  define TRANSMISSIONSHARED_EXPORT Q_DECL_EXPORT
#else
#  define TRANSMISSIONSHARED_EXPORT Q_DECL_IMPORT
#endif
