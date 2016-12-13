#pragma once

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include <ipluginurl.h>

#include "../../../specialize.h"

#include "teamcity_global.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

class TEAMCITYSHARED_EXPORT TeamCity : public IPluginURL
{
    Q_OBJECT
public:
    // IPlugin
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }
    QStringList DisplayName() const Q_DECL_OVERRIDE;
    QString PluginClass() const Q_DECL_OVERRIDE { return "URL"; }
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    void SetStory(const QUrl& url) Q_DECL_OVERRIDE;
    void SetCoverage(bool /*notices_only*/) Q_DECL_OVERRIDE {}
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;

    // IPluginURL
    QStringList Requires() const Q_DECL_OVERRIDE;
    bool SetRequirements(const QStringList& parameters) Q_DECL_OVERRIDE;

private slots:
    void    slot_get_read();
    void    slot_get_complete();
    void    slot_get_failed(QNetworkReply::NetworkError code);
    void    slot_poll();

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, QJsonObject, JsonObject)    // "JsonObjectMap"
    SPECIALIZE_MAP(int, QJsonObject, Status)            // "StatusMap"
    SPECIALIZE_QUEUE(QString, Builder)                  // "BuilderQueue"
    SPECIALIZE_MAP(QNetworkReply*, bool, Reply)         // "ReplyMap"
    SPECIALIZE_MAP(QNetworkReply*, QByteArray, Finish)  // "FinishMap"

    enum class States {
        None,
        GettingProjects,
        GettingBuilders,
        GettingStatus,
        GettingResult,
    };

private:    // methods
    void        process_reply(QNetworkReply *reply);
    void        process_status(const QJsonObject& status);
    void        process_final(QNetworkReply *reply);

private:    // data members
    States      state;

    QString     username;
    QString     password;
    QString     project_name;
    QString     builder_name;

    QNetworkAccessManager*  QNAM;
    ReplyMap    active_replies;
    FinishMap   finish_replies;

    JsonObjectMap json_projects, json_builders;
    StatusMap   build_status;

    QByteArray  reply_buffer;

    QTimer*         poll_timer;
    BuilderQueue    status_queue;
};
