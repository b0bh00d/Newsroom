#pragma once

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include "../../../specialize.h"

#include "teamcity9_global.h"

/// @class TeamCity9Poller
/// @brief Centralized poller for a Team City server user endpoint
///
/// This class polls a Team City server for information.  It posts the
/// information it retrieves for any plug-in instances to receive.  It is
/// intended to provide a single contact to a Team City server instead of
/// having N-number of plug-in instances retrieving the exact same info.

class TEAMCITY9SHARED_EXPORT TeamCity9Poller : public QObject
{
    Q_OBJECT
public:
    TeamCity9Poller(const QUrl& target, const QString& username, const QString& password, int timeout, QObject* parent = nullptr);
    ~TeamCity9Poller();

    void    add_filter(const QString& project_name, const QString& builder_name, QObject* me);
    void    remove_filter(const QString& project_name, const QString& builder_name, QObject* me);

private slots:
    void    slot_get_read();
    void    slot_get_complete();
    void    slot_get_failed(QNetworkReply::NetworkError code);
    void    slot_request_pump();
    void    slot_poll();

private:    // typedefs and enums
    enum class BuilderEvents
    {
        None,
        BuildStarted,
        BuildProgress,
        BuildFinal,
    };

    enum class ReplyStates
    {
        None,
        GettingProjects,
        GettingBuilders,
        GettingStatus,
        GettingFinal,
    };

    SPECIALIZE_MAP(int /* build id */, QJsonObject, Status)            // "StatusMap"

private:    // classes
    struct RequestData
    {
        ReplyStates      state;
        QString     url;
        QStringList data;
    };

    struct ReplyData
    {
        ReplyStates      state;
        QByteArray  buffer;
        QStringList data;       // project + builder names and ids
    };

    struct BuilderData
    {
        bool        first_update;
        QJsonObject builder_data;
        StatusMap   build_status;
    };
    SPECIALIZE_LIST(BuilderData, Builders)              // "BuildersList"

    struct ProjectData
    {
        QJsonObject     project_data;
        BuildersList    builders;
    };

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, ProjectData, Projects)      // "ProjectsMap"
    SPECIALIZE_MAP(QNetworkReply*, ReplyData, Reply)    // "ReplyMap"
    SPECIALIZE_MAP(QString, BuilderData, BuilderData)   // "BuilderDataMap"
    SPECIALIZE_QUEUE(RequestData, Request)              // "RequestQueue"
    SPECIALIZE_MAP(QString, bool, PendingRequests)      // "PendingRequestsMap"
    SPECIALIZE_LIST(QObject*, Interested)               // "InterestedList"
    SPECIALIZE_MAP(QString, InterestedList, Interested) // "InterestedMap"

private:    // methods
    void            notify_interested_parties(BuilderEvents event, const QString& project_name, const QString& builder_name, const QJsonObject& status);
    void            notify_interested_parties(const QString& project_name, const QString& builder_name, const QString& message);
    void            enqueue_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            enqueue_request_unique(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            create_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            process_reply(QNetworkReply *reply);
    void            process_status(const QJsonObject& status, const QStringList &status_data);
    void            process_final(const QJsonObject& status, const QStringList &status_data);

private:    // data members
    int         replies_expected;   // how many initial replies before we start the timer?
    QUrl        target;
    QString     username;
    QString     password;

    QNetworkAccessManager*  QNAM;
    ReplyMap    active_replies;

    QTimer*     request_timer;
    QTimer*     poll_timer;
    int         poll_timeout;

    RequestQueue        requests;
    PendingRequestsMap  pending_requests;

    ProjectsMap projects;

    InterestedMap   interested_parties;
};

SPECIALIZE_SHAREDPTR(TeamCity9Poller, Poller)           // "PollerPointer"
