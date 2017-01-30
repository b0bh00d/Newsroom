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

    // this filtering mechanism is used because Qt does not provide
    // a canonical means of creating runtime, dynamic signals/slots
    void    add_interest(const QString& project_name, const QString& builder_name, QObject* me, int flags = Interest::None);
    void    remove_interest(const QString& project_name, const QString& builder_name, QObject* me);

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
        BuildPending,
        BuildStarted,
        BuildProgress,
        BuildFinal,
    };

    enum class ReplyStates
    {
        None,
        GettingProjects,
        GettingBuilders,
        GettingBuilderStatus,
        GettingBuildPending,
        GettingBuildStatus,
        GettingBuildFinal,
    };

    enum class Priorities
    {
        BackOfQueue,
        FrontOfQueue,
    };

    SPECIALIZE_MAP(int /* build id */, QJsonObject, Status)            // "StatusMap"

private:    // classes
    struct RequestData
    {
        ReplyStates state;
        QString     url;
        QStringList data;
    };

    struct ReplyData
    {
        ReplyStates state;
        QByteArray  buffer;
        QStringList data;       // project + builder names and ids
    };

    struct BuilderData
    {
        bool            pause_pending_changes_check;
        bool            limit_pending_changes_check;
        int             pending_changes_check_count;
        QJsonObject     builder_data;
        StatusMap       build_status;
        BuilderEvents   build_event;

        BuilderData() :
            pause_pending_changes_check(false),
            pending_changes_check_count(0),
            limit_pending_changes_check(false),
            build_event(BuilderEvents::None)
        {}

    };
    SPECIALIZE_LIST(BuilderData, Builders)              // "BuildersList"

    struct ProjectData
    {
        QJsonObject     project_data;
        BuildersList    builders;
    };

    struct InterestData
    {
        QObject*        party;
        int             flags;
    };

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, ProjectData, Projects)      // "ProjectsMap"
    SPECIALIZE_MAP(QNetworkReply*, ReplyData, Reply)    // "ReplyMap"
    SPECIALIZE_MAP(QString, BuilderData, BuilderData)   // "BuilderDataMap"
    SPECIALIZE_LIST(RequestData, Request)               // "RequestList"
    SPECIALIZE_MAP(QString, bool, PendingRequests)      // "PendingRequestsMap"
    SPECIALIZE_LIST(InterestData, Interested)           // "InterestedList"
    SPECIALIZE_MAP(QString, InterestedList, Interested) // "InterestedMap"

private:    // methods
    void            notify_interested_parties(BuilderEvents event, const QString& project_name, const QString& builder_name, const QJsonObject& status = QJsonObject());
    void            notify_interested_parties(const QString& project_name, const QString& builder_name, const QString& message);
    void            enqueue_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList(), Priorities priority = Priorities::BackOfQueue);
    void            enqueue_request_unique(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            create_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            process_reply(QNetworkReply *reply);
    bool            any_interest_in_changes_check(const QString& project_name, const QString& builder_name);
    void            process_builder_status(const QJsonObject& status, const QStringList &status_data);
    void            process_build_pending(const QJsonObject& status, const QStringList &status_data);
    void            process_build_status(const QJsonObject& status, const QStringList &status_data);
    void            process_build_final(const QJsonObject& status, const QStringList &status_data);

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

    RequestList requests;
    PendingRequestsMap  pending_requests;

    ProjectsMap projects;

    InterestedMap   interested_parties;
};

SPECIALIZE_SHAREDPTR(TeamCity9Poller, Poller)           // "PollerPointer"
