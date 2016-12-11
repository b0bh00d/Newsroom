#pragma once

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include <ipluginrest.h>

#include "../../../specialize.h"

#include "teamcity_global.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

class TEAMCITYSHARED_EXPORT TeamCity : public IPluginREST
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IPluginREST" FILE "")
    Q_INTERFACES(IPluginREST)

public:
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }

    QStringList DisplayName() const Q_DECL_OVERRIDE;
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    QStringList Requires() const Q_DECL_OVERRIDE;
    bool SetStory(const QUrl& url, const QStringList& parameters) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;
    QString Headline() Q_DECL_OVERRIDE;

private slots:
    void    slot_get_read();
    void    slot_get_complete();
    void    slot_get_failed(QNetworkReply::NetworkError code);

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, QJsonObject, JsonObject)    // "JsonObjectMap"

    enum class States {
        None,
        GettingProjects,
        GettingBuilders,
        GettingHistory,
        GettingDetails,
    };

private:    // methods
    void        process_reply();

private:    // data members
    QString     error_message;

    States      state;

    QUrl        server;
    QString     username;
    QString     password;
    QString     project_name;
    QString     builder_name;

    QNetworkAccessManager*  QNAM;

    JsonObjectMap json_projects, json_builders, json_details;

    QByteArray  reply_buffer;
};
