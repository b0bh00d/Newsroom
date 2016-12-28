#pragma once

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include <ireporter.h>

#include "../../../specialize.h"

#include "teamcity_global.h"

/// @class TeamCity9
/// @brief A Reporter for Newsroom that covers Team City buids
///
/// TeamCity9 is a Reporter plug-in for Newsroom that knows how to read
/// and report on one or more builds using Team City's REST API.
///
/// This particular Reporter instance converses with Team City using its
/// v9 REST API calls.

class TEAMCITY9SHARED_EXPORT TeamCity9 : public IReporter
{
    Q_OBJECT
public:
    TeamCity9(QObject* parent = nullptr);

    // IReporter
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }
    QStringList DisplayName() const Q_DECL_OVERRIDE;
    QString PluginClass() const Q_DECL_OVERRIDE { return "URL"; }
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    bool Supports(const QString& /*file*/) const Q_DECL_OVERRIDE { return false; }
    QStringList Requires() const Q_DECL_OVERRIDE;
    bool SetRequirements(const QStringList& parameters) Q_DECL_OVERRIDE;
    void SetStory(const QUrl& url) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;

private slots:
    void    slot_get_read();
    void    slot_get_complete();
    void    slot_get_failed(QNetworkReply::NetworkError code);
    void    slot_poll();

private:    // typedefs and enums
    enum class States {
        None,
        GettingProjects,
        GettingBuilders,
        GettingStatus,
        GettingFinal,
    };

private:    // classes
    struct ReplyData
    {
        States      state;
        QByteArray  buffer;
    };

    struct ETAData
    {
        int         initial_completed;
        uint        start;

        // these values will be used to try and detect a hung build
        int         last_completed;
        uint        last_changed;
    };

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, QJsonObject, JsonObject)    // "JsonObjectMap"
    SPECIALIZE_MAP(int, QJsonObject, Status)            // "StatusMap"
    SPECIALIZE_MAP(QNetworkReply*, ReplyData, Reply)    // "ReplyMap"
    SPECIALIZE_MAP(int, ETAData, ETA)                   // "ETAMap"
    SPECIALIZE_MAP(QString, QString, Report)            // "ReportMap"

private:    // methods
    void            process_reply(QNetworkReply *reply);
    void            process_status(const QJsonObject& status);
    void            process_final(QNetworkReply *reply);
    void            create_request(const QString& url_str, States state);
    void            populate_report_map(ReportMap& report_map,
                                        const QJsonObject& build,
                                        const QString& builder_id = QString(),
                                        const QString& eta_str = QString());
    QString         render_report(const ReportMap& report_map);

private:    // data members
    QString     username;
    QString     password;
    QString     project_name;
    QString     builder_name;

    QNetworkAccessManager*  QNAM;
    ReplyMap    active_replies;

    JsonObjectMap json_projects, json_builders;
    StatusMap   build_status;

    QTimer*     poll_timer;
    int         poll_timeout;

    ETAMap      eta;

    bool        first_update;   // first update should report something if there are no active builds

    QStringList report_template;
};
