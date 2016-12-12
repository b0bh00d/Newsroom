#pragma once

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QByteArray>

#include "iplugin.h"

/// @class IPluginURL
/// @brief A plug-in that knows how to handle a particular REST API
///
/// This Interface class defines the interfaces for use with a Qt-based plug-in
/// that knows how to interact with a URL.  For example, a REST plug-in for
/// TeamCity would know how to query a server to retrieve information about
/// the status of a particular project.

class IPluginURL : public IPlugin
{
    Q_OBJECT
public:     // methods
    virtual ~IPluginURL() {}

    /*!
      This method returns a list of parameter names that it requires in order
      to perform its function.  Each parameter name is followed by a type,
      one of "string", "password", "integer" or "double".  Parameter names that
      end with an asterisks (*) indicate a required value, all others are
      considered optional.

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
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(IPluginURL, "org.lucidgears.Newsroom.IPluginURL")
QT_END_NAMESPACE
