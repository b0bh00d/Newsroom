#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>

#include "iplugin.h"

/// @class IPluginLocal
/// @brief A plug-in that knows how to handle a local file system entity.
///
/// This Interface class defines the interfaces for use with a Qt-based plug-in
/// that knows how to process an entity on the local file system.

class IPluginLocal : public IPlugin
{
public:     // methods
    virtual ~IPluginLocal() {}

    /*!
      This method tells the host if it can process the indicated file.  It is
      up to the plug-in to determine if it is a file format whose contents it
      can monitor, and from which provide meaningful Headlines.

      \param file A fully qualify path to an entity on the local file system.
      \returns A Boolean true if the plug-in can provide meaningful Headlines for this file, otherwise false.
     */
    virtual bool CanGrok(const QString& file) const = 0;

    /*!
      Sets the Story (file system entity) for the Local plug-in to cover.

      \param file A fully qualify path to an entity on the local file system.
     */
    virtual void SetStory(const QString& file) = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(IPluginLocal, "org.lucidgears.Newsroom.IPluginLocal")
QT_END_NAMESPACE
