#ifndef FACEPLUGIN_GLOBAL_H
#define FACEPLUGIN_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FACEPLUGIN_LIBRARY)
#  define FACEPLUGINSHARED_EXPORT Q_DECL_EXPORT
#else
#  define FACEPLUGINSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // FACEPLUGIN_GLOBAL_H
