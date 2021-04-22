#pragma once

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include <QtXml/QDomDocument>

#include "../../../specialize.h"

#include "chartapi_global.h"

/// @class YahooChartAPIPoller
/// @brief Centralized poller for a Yahoo Chart API endpoint
///
/// This class polls the Yahoo Chart API server for information about a given
/// ticker symbol.  It posts the information it retrieves for any plug-in
/// instances to receive.  It is intended to provide a single contact to
/// the API instead of having N-number of plug-in instances retrieving the
/// exact same info.

class YAHOOCHARTAPISHARED_EXPORT YahooChartAPIPoller : public QObject
{
    Q_OBJECT
public:
    YahooChartAPIPoller(TickerFormat format, const QUrl& story, const QString& ticker, int timeout, QObject* parent = nullptr);
    ~YahooChartAPIPoller();

    // this filtering mechanism is used because Qt does not provide
    // a canonical means of creating runtime, dynamic signals/slots
    void    add_interest(const QString& ticker, QObject* me, int flags = 0);
    void    remove_interest(const QString& ticker, QObject* me);

private slots:
    void    slot_get_read();
    void    slot_get_complete();
    void    slot_get_failed(QNetworkReply::NetworkError code);
    void    slot_request_pump();
    void    slot_poll();

private:    // typedefs and enums
    enum class TickerEvents
    {
        None,
        Update,
    };

    enum class ReplyStates
    {
        None,
        GettingUpdate,
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
        ReplyStates state{ReplyStates::None};
        QString     url;
        QStringList data;
    };

    struct ReplyData
    {
        ReplyStates state{ReplyStates::None};
        QByteArray  buffer;
        QStringList data;       // project + builder names and ids
    };

    struct InterestData
    {
        QObject*        party{nullptr};
        int             flags{0};
    };

private:    // typedefs and enums
    SPECIALIZE_MAP(QNetworkReply*, ReplyData, Reply)    // "ReplyMap"
    SPECIALIZE_LIST(RequestData, Request)               // "RequestList"
    SPECIALIZE_MAP(QString, bool, PendingRequests)      // "PendingRequestsMap"
    SPECIALIZE_LIST(InterestData, Interested)           // "InterestedList"
    SPECIALIZE_MAP(QString, InterestedList, Interested) // "InterestedMap"

private:    // methods
    void            notify_interested_parties(TickerEvents event, const QString& ticker, const QString& status);
    void            notify_interested_parties(TickerEvents event, const QString& ticker, const QJsonObject& status = QJsonObject());
    void            notify_interested_parties(TickerEvents event, const QString& ticker, const QDomDocument& status = QDomDocument());
    void            notify_interested_parties(const QString& ticker, const QString& message);
    void            enqueue_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList(), Priorities priority = Priorities::BackOfQueue);
    void            enqueue_request_unique(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            create_request(const QString& url_str, ReplyStates state, const QStringList& request_data = QStringList());
    void            process_reply(QNetworkReply *reply);

private:    // data members
    TickerFormat    format{TickerFormat::None};
    QString         target;
    QString         ticker;

    QNetworkAccessManager*  QNAM{nullptr};
    ReplyMap        active_replies;

    QTimer*         request_timer{nullptr};
    QTimer*         poll_timer{nullptr};
    int             poll_timeout{0};

    RequestList     requests;
    PendingRequestsMap  pending_requests;

    InterestedMap   interested_parties;
};

SPECIALIZE_SHAREDPTR(YahooChartAPIPoller, Poller)           // "PollerPointer"
