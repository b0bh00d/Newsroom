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
      associated error string by calling this method.

      \returns A string contains some kind of explanation of the unexpected result.
     */
    virtual QString ErrorString() const = 0;

    /*!
      This method returns a displayable name for the Reporter plug-in, something
      meaningful to the user for when they are selecting plug-ins to handle a
      Story.  It can also optionally return a tooltip value that the host can
      display to help clarify the support provided by the plug-in.

      The tooltip should be a brief but informative description of the plug-in,
      and should include enough information so the user can decide if this
      plug-in is the right tool for the job.

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
      shown to the user, but will be used internally to identify the plug-in and
      any data associated with it.

      \returns A unique identifier for the plug-in (among all other plug-ins).
     */
    virtual QByteArray PluginID() const = 0;

    /*!
      This method tells the host if it can process the indicated entity.  It is
      up to the plug-in to determine if it is contains content it can monitor,
      and from which file meaningful reports.

      \param file An entity of some kind (typically a local file path or URL-formatted resource).
      \returns A Boolean true if the plug-in can provide meaningful reports for this entity, otherwise false.
     */
    virtual bool Supports(const QUrl& story) const = 0;

    /*!
      This method returns a list of names identifying parameters that the
      plug-in requires in order to perform its function.

      Parameter names that end with an asterisk (*) indicate a required value,
      all others are considered optional.  Required fields will be highlighted
      and enforced.

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
      - combo
        + A combo box

      Parameter types can specify default values by adding a colon (:) followed
      by a value as a suffix (e.g., "integer:10").  This default value will
      become the place holder value in single-line edit fields.

      In the case of "multiline" editing, the default valaue (if any) will be
      inserted into the edit field for the user to directly modify.

      For "combo", combo box values are provided in a comma-separated list.
      The default index can be specified by following an item with an asterisk,
      otherwise the first item in the list will be selected by default.

      The host application will post a dialog requesting these parameters and
      types from the user, and then provide them back to the plug-in via the
      SetRequirements() method.

      \returns A list of string pairs representing parameter names and types with optional defaults.
     */
    virtual QStringList Requires() const = 0;

    /*!
      Sets the parameters dictated by the Required() method.

      \param parameters A list of string values that match the parameters/types indicated by the Requires() method.  The plug-in will convert the string values into the appropriate data types, as needed.
      \returns A true value if the provided parameters are sufficient to perform the plug-in's function, otherwise false.
     */
    virtual bool SetRequirements(const QStringList& parameters) = 0;

    /*!
      Sets the Story (QUrl) for the plug-in to cover.  This method will only
      be called if the Supports() method returned true.

      \param story A QUrl that indicates the target of the coverage to be performed.
     */
    virtual void SetStory(const QUrl& story) = 0;

    /*!
      Start covering the Story.  The plug-in should begin any actions required
      to retrieve report information from the entity provided to SetStory()
      when this method is invoked.

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
      This method gives the Reporter plug-in the opportunity to secure any
      sensitive data provided to it by SetRequirements() (e.g., passwords)
      before it gets stored to disc.  The method of securing is left entirely
      up to the plug-in, and it is is the plug-ins responsibility to undo any
      security actions when such data is provided to the Unsecure() or
      SetRequirements() methods.

      Data should be modified in-place.
     */
    virtual void Secure(QStringList& parameters) const = 0;

    /*!
      Any parameters secured by a previous invocation of the Secure() method
      should be released by this call.  This will allow the data to be modified
      by the user.

      Data should be modified in-place.
     */
    virtual void Unsecure(QStringList& parameters) const = 0;

signals:
    void        signal_new_data(const QByteArray& data);

protected:
    QString     error_message;
    QUrl        story;
};

typedef QSharedPointer<IReporter> IReporterPointer;

/// @class IReporterFactory
/// @brief A factory interface for generating IPlugin-based plug-ins
///
/// All Reporter plug-ins must provide this factory interface.  This interface
/// is the actual Qt interface employed by the host application.

class IReporterFactory : public QObject
{
    Q_OBJECT
public:
    virtual IReporterPointer newInstance() = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(IReporterFactory, "org.Newsroom.IReporterFactory")
QT_END_NAMESPACE
