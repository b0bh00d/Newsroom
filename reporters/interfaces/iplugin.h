#pragma once

#include <QtPlugin>

#include <QtCore/QString>
#include <QtCore/QByteArray>

/// @class IPlugin
/// @brief Base interface for Newsroom plug-ins.
///
/// All Newsroom Reporter plug-ins support this minimal interface.

class IPlugin
{
public:     // methods
    virtual ~IPlugin() {}

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
      Start covering the Story.  The plug-in should begin any actions required
      to retrieve status information from the REST API when this method is invoked.

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
