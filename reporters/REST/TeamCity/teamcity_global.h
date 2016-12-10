#pragma

#include <QtCore/qglobal.h>

#if defined(TEAMCITY_LIBRARY)
#  define TEAMCITYSHARED_EXPORT Q_DECL_EXPORT
#else
#  define TEAMCITYSHARED_EXPORT Q_DECL_IMPORT
#endif
