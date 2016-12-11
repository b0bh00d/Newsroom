#pragma once

#include <QObject>
#include <QtPlugin>

#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QByteArray>

/// @class IPlugin
/// @brief Base interface for Newsroom plug-ins.
///
/// All Newsroom Reporter plug-ins support this minimal interface.

class IPlugin : public QObject
{
    Q_OBJECT
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
      URL.  It can also optionally return a tooltip value that the host can
      display to help clarify the support provided by the plug-in.

      \returns A displayable name for the plug-in.
     */
    virtual QStringList DisplayName() const = 0;

    /*!
      This method returns the class of the plug-in.  This is used by the host
      to determine the category of Story that the plug-in can cover.

      \returns A string representing the plug-in class ("Local", "REST", etc.)
     */
    virtual QString PluginClass() const = 0;

    /*!
      This method returns a unique identifier for the plug-in.  This identifier
      should be guaranteed to be globally unique (hint: GUID).  It will not be
      shown to the user.

      \returns A unique identifier for the plug-in.
     */
    virtual QByteArray PluginID() const = 0;

    /*!
      Sets the Story (QUrl) for the plug-in to cover.

      \param story A QUrl that indicates the target of the coverage to be performed.
     */
    virtual void SetStory(const QUrl& story) = 0;

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

signals:
    void        signal_new_data(const QByteArray& data);

protected:
    QString         error_message;
    QUrl            story;
};
