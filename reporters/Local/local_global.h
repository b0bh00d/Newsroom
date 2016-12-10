#pragma

#include <QtCore/qglobal.h>

#if defined(LOCAL_LIBRARY)
#  define LOCALSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LOCALSHARED_EXPORT Q_DECL_IMPORT
#endif
