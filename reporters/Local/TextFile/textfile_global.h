#pragma

#include <QtCore/qglobal.h>

#if defined(TEXTFILE_LIBRARY)
#  define TEXTFILE_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define TEXTFILE_SHARED_EXPORT Q_DECL_IMPORT
#endif
