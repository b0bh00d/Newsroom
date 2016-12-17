#pragma once

#include <QObject>
#include <QtPlugin>

#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QByteArray>
#include <QtCore/QSharedPointer>

/// @class IPlugin
/// @brief Base interface for Newsroom plug-ins.
///
/// All Newsroom Reporter plug-ins support this minimal interface.

class IPlugin : public QObject
{
    Q_OBJECT
public:     // methods
    IPlugin(QObject* parent = nullptr) : QObject(parent) {}
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
      This method tells the host if it can process the indicated file.  It is
      up to the plug-in to determine if it is a file format whose contents it
      can monitor, and from which provide meaningful Headlines.

      \param file A fully qualify path to an entity on the local file system.
      \returns A Boolean true if the plug-in can provide meaningful Headlines for this file, otherwise false.
     */
    virtual bool Supports(const QString& file) const = 0;

    /*!
      This method returns a list of parameter names that it requires in order
      to perform its function.  Each parameter name is followed by a type,
      one of "string", "password", "integer" or "double".

      Parameter names that end with an asterisks (*) indicate a required value,
      all others are considered optional.

      Parameter types can specify default values by adding a colon (:) followed
      by a value as a suffix (e.g., "integer:10").  This default value will
      become the placeholder value in the assigned edit field.

      The host should post a dialog requesting these parameters and types from
      the user, and then provide them back to the plug-in via the SetStory()
      method.

      \returns A string list of parameter names and types.
     */
    virtual QStringList Requires() const = 0;

    /*!
      Sets the parameters dictated by the Required() method.

      \param parameters A list of string values that match the parameters/types indicated by the Requires() method.  The plug-in will convert the string values into the appropriate data types, as needed.
      \returns A true value if the provided parameters are sufficient to perform the plug-in's function, otherwise false.
     */
    virtual bool SetRequirements(const QStringList& parameters) = 0;

    /*!
      Sets the Story (QUrl) for the plug-in to cover.

      \param story A QUrl that indicates the target of the coverage to be performed.
     */
    virtual void SetStory(const QUrl& story) = 0;

    /*!
      Local Reporters can cover stories as notices or in depth.  Notices just
      make mention of a change in the story (file changes), while in-depth
      coverage actually produces tangible content (file data deltas).

      \param notices_only True if only file changes will be reported, otherwise file deltas will be provided.
     */
    virtual void SetCoverage(bool notices_only = false) = 0;

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

typedef QSharedPointer<IPlugin> IPluginPointer;

class IPluginFactory : public QObject
{
    Q_OBJECT
public:
    virtual IPluginPointer newInstance() = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(IPluginFactory, "org.lucidgears.Newsroom.IPluginFactory")
QT_END_NAMESPACE
