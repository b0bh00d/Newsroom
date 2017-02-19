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

#include "transmissionglobal.h"

/// @class TransmissionPoller
/// @brief Centralized poller for a Transmission client
///
/// As of this writing, the Transmission client process does not provide
/// a REST API that we can poll, so this class polls a 'transmission-rest'
/// helper written in Python for information about the current torrents
/// being processed by a given Transmission torrent client.  It posts the
/// information it retrieves for any plug-in instances to receive.  It is
/// intended to provide a single contact to a 'transmission-rest' helper
/// instead of having N-number of plug-in instances retrieving the exact
/// same info.
///
/// Transmission does not track torrents by a unique id; rather, it just
/// reports the torrent occupying a slot from "1" to however many torrents
/// are currently being processed.  The torrents within slots are not
/// persistent bound to those slots: the user may delete them at any time,
/// and torrents in higher slots may move down to occupy new positions.

class TRANSMISSIONSHARED_EXPORT TransmissionPoller : public QObject
{
    Q_OBJECT
public:
    TransmissionPoller(const QUrl& target, int timeout, int flags = 0, QObject* parent = nullptr);
    ~TransmissionPoller();

    // this filtering mechanism is used because Qt does not provide
    // a canonical means of creating runtime, dynamic signals/slots
    void    add_interest(int slot, QObject* me, int flags = 0);
    void    remove_interest(int slot, QObject* me);

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
        GettingStatus,
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

        InterestData() : party(nullptr), flags(0) {}
    };

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, ProjectData, Projects)      // "ProjectsMap"
    SPECIALIZE_MAP(QNetworkReply*, ReplyData, Reply)    // "ReplyMap"
    SPECIALIZE_MAP(QString, BuilderData, BuilderData)   // "BuilderDataMap"
    SPECIALIZE_LIST(RequestData, Request)               // "RequestList"
    SPECIALIZE_MAP(QString, bool, PendingRequests)      // "PendingRequestsMap"
    SPECIALIZE_LIST(InterestData, Interested)           // "InterestedList"
//    SPECIALIZE_MAP(QString, InterestedList, Interested) // "InterestedMap"

private:    // methods
    void            notify_interested_parties(int slot, const QJsonObject& status = QJsonObject(), float maxratio = 0.0f);
    void            notify_interested_parties(int slot, const QString& message);
    void            enqueue_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList(), Priorities priority = Priorities::BackOfQueue);
    void            enqueue_request_unique(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            create_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            process_reply(QNetworkReply *reply);
    void            process_client_status(const QJsonObject& status, const QStringList &status_data);

private:    // data members
    QUrl        target;

    QNetworkAccessManager*  QNAM;
    ReplyMap    active_replies;

    QTimer*     request_timer;
    QTimer*     poll_timer;
    int         poll_timeout;

    RequestList requests;
    PendingRequestsMap  pending_requests;

    ProjectsMap projects;

    InterestedList  interested_parties;

    bool        ignore_finished;
    bool        ignore_stopped;
    bool        ignore_idle;
};

SPECIALIZE_SHAREDPTR(TransmissionPoller, Poller)        // "PollerPointer"
