#pragma once

#include <QtCore/QObject>
#include <QtCore/QtPlugin>

#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QByteArray>
#include <QtCore/QSharedPointer>

/// @class IReporter
/// @brief Base interface for Newsroom Reporter plug-ins.
///
/// All Newsroom Reporter plug-ins support this minimal interface.

class IReporter : public QObject
{
    Q_OBJECT
public:     // methods
    IReporter(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IReporter() {}

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
      This method returns a list of parameter names that the plug-in requires in
      order to perform its function.

      Parameter names that end with an asterisk (*) indicate a required value,
      all others are considered optional.  Required fields will be highlighted.

      Each parameter should specify one of the following types:

      - string
        + This is a regular, single-line string value
      - password
        + This is identical to 'string', except the input field will handle as a password
      - integer
        + An integer type (validated using the regular expression "\d+")
      - double
        + A floating-point type (validate with a QDoubleValidator)
      - multiline
        + A multiline string value (edited with a QPlainTextEdit widget)

      Parameter types can specify default values by adding a colon (:) followed
      by a value as a suffix (e.g., "integer:10").  This default value will
      become the placeholder value in single-line edit fields.  In the case of
      "multiline" editing, the default vlaue (if any) will be inserted into the
      edit field for the user to modify.

      Newroom will post a dialog requesting these parameters and types from
      the user, and then provide them back to the plug-in via the SetRequirements()
      method.

      \returns A list of string pairs representing parameter names and types.
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

typedef QSharedPointer<IReporter> IReporterPointer;

/// @class IReporterFactory
/// @brief A factory interface for generating IPlugin-based plug-ins
///
/// All Newsroom Reporter plug-ins must provide this factory interface.
/// This interface is the actual Qt interface employed by Newsroom.

class IReporterFactory : public QObject
{
    Q_OBJECT
public:
    virtual IReporterPointer newInstance() = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(IReporterFactory, "org.lucidgears.Newsroom.IReporterFactory")
QT_END_NAMESPACE
