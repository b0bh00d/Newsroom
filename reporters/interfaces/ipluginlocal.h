#pragma once

#include <QtCore/QtPlugin>
#include <QtCore/QString>
#include <QtCore/QByteArray>

/// @class IPluginLocal
/// @brief A plug-in that knows how to handle a local file system entity.
///
/// This Interface class defines the interfaces for use with a Qt-based plug-in
/// that knows how to process an entity on the local file system.

class IPluginLocal
{
public:     // methods
    virtual ~IPluginLocal() {}

    /*!
      If a method returns an unexpected value, the host may retrieve any
      associated erorr string by calling this method.

      \returns A string contains some kind of explanation of the unexpected result.
     */
    virtual QString ErrorString() const = 0;

    /*!
      This method returns a displayable name for the REST plug-in, something
      meaningful to the user for when they are selecting plug-ins to handle a
      URL.

      \returns A displayable name for the plug-in.
     */
    virtual QString DisplayName() const = 0;

    /*!
      This method returns a unique identifier for the plug-in.  This identifier
      should be guaranteed to be globally unique (hint: GUID).  It will not be
      shown to the user.

      \returns A unique identifier for the plug-in.
     */
    virtual QByteArray PluginID() const = 0;

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

    /*!
      Start covering the Story.

      \returns A true value if Story coverage began, otherwise false.
     */
    virtual bool CoverStory() = 0;

    /*!
      Stops covering the Story.  The plug-in should stop any actions started
      by CoverStory().

      \returns A true value if Story coverage was successfully stopped, otherwise false.
     */
    virtual bool FinishStory() = 0;

    /*!
      Retrieves a headline for the currently covered Story.  The returned value
      can be empty if no headline is currently available.

      \returns A headline to be displayed by a Chyron, if one is available..
     */
    virtual QString Headline() = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(IPluginLocal, "org.lucidgears.Newsroom.IPluginLocal")
QT_END_NAMESPACE
