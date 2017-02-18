#include "transmissionpoller.h"
#include "transmission.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

TransmissionPoller::TransmissionPoller(const QUrl& target, int timeout, QObject *parent)
    : target(target),
      QNAM(nullptr),
      poll_timer(nullptr),
      poll_timeout(timeout),
      QObject(parent)
{
    QNAM = new QNetworkAccessManager(this);

    request_timer = new QTimer(this);
    request_timer->setInterval(200);
    connect(request_timer, &QTimer::timeout, this, &TransmissionPoller::slot_request_pump);
    request_timer->start();

    poll_timer = new QTimer(this);
    poll_timer->setInterval(poll_timeout * 1000);
    connect(poll_timer, &QTimer::timeout, this, &TransmissionPoller::slot_poll);

    // prime the pump and get projects listing first
    enqueue_request(target.toString(), ReplyStates::GettingStatus);
}

TransmissionPoller::~TransmissionPoller()
{
    if(request_timer)
    {
        request_timer->stop();
        request_timer->deleteLater();
        request_timer = nullptr;
    }

    if(poll_timer)
    {
        poll_timer->stop();
        poll_timer->deleteLater();
        poll_timer = nullptr;
    }

    if(QNAM)
    {
        QNAM->deleteLater();
        QNAM = nullptr;
    }
}

void TransmissionPoller::add_interest(int slot, QObject* me, int flags)
{
    // make sure it's a Transmission instance
    Transmission* transmission = dynamic_cast<Transmission*>(me);
    if(!transmission)
        return;

    while(interested_parties.length() < slot)
        interested_parties.append(InterestData());

    interested_parties[slot-1].party = me;
    interested_parties[slot-1].flags = flags;

    if(poll_timer->isActive())
    {
        poll_timer->start();    // reset it
        slot_poll();            // get a new update now
    }
}

void TransmissionPoller::remove_interest(int slot, QObject* /*me*/)
{
    if(interested_parties.length() < slot)
        return;
    interested_parties[slot-1] = InterestData();
}

void TransmissionPoller::notify_interested_parties(int slot, const QString& message)
{
    if(slot == -1)
    {
        // notify them all
        InterestedList::iterator iter;
        for(iter = interested_parties.begin();iter != interested_parties.end();++iter)
        {
            Transmission* transmission = dynamic_cast<Transmission*>(iter->party);
            if(!transmission)
                continue;
            transmission->error(message);
        }

        return;
    }

    if(interested_parties.length() < slot)
        return;

    InterestData& data = interested_parties[slot-1];
    if(data.party)
    {
        Transmission* transmission = dynamic_cast<Transmission*>(data.party);
        if(transmission)
            transmission->error(message);
    }
}

void TransmissionPoller::notify_interested_parties(int slot, const QJsonObject& status, float maxratio)
{
    if(interested_parties.length() < slot)
        return;

    InterestData& data = interested_parties[slot-1];
    if(data.party)
    {
        Transmission* transmission = dynamic_cast<Transmission*>(data.party);
        if(transmission)
            transmission->status(status, maxratio);
    }
}

void TransmissionPoller::enqueue_request(const QString& url_str, ReplyStates state, const QStringList& request_data, Priorities priority)
{
    RequestData rd;
    rd.url = url_str;
    rd.state = state;
    rd.data = request_data;
    if(priority == Priorities::BackOfQueue)
        requests.push_back(rd);
    else
        requests.insert(0, rd);
    pending_requests[rd.url] = true;
}

void TransmissionPoller::enqueue_request_unique(const QString& url_str, ReplyStates state, const QStringList& request_data)
{
    if(pending_requests.contains(url_str))
        return;
    enqueue_request(url_str, state, request_data);
}

void TransmissionPoller::create_request(const QString& url_str, ReplyStates state, const QStringList& request_data)
{
    QUrl url(url_str);

    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Content-type"), QByteArray("application/json"));
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));

    QNetworkReply* reply = QNAM->get(request);
    bool connected = connect(reply, &QNetworkReply::readyRead, this, &TransmissionPoller::slot_get_read);
    ASSERT_UNUSED(connected);
    connected = connect(reply, &QNetworkReply::finished, this, &TransmissionPoller::slot_get_complete);
    ASSERT_UNUSED(connected);
    connected = connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                        this, &TransmissionPoller::slot_get_failed);
    ASSERT_UNUSED(connected);

    ReplyData reply_data;
    reply_data.state = state;
    reply_data.data = request_data;

    active_replies[reply] = reply_data;

    if(pending_requests.contains(url_str))
        pending_requests.remove(url_str);
}

void TransmissionPoller::process_reply(QNetworkReply *reply)
{
    ReplyData& data = active_replies[reply];
    if(data.state == ReplyStates::GettingStatus)
    {
        QJsonParseError error;
        QJsonDocument d = QJsonDocument::fromJson(data.buffer, &error);
        if(error.error == QJsonParseError::NoError)
        {
            QJsonObject sett2 = d.object();
            if(!sett2.isEmpty())
                process_client_status(sett2, data.data);
        }

        if(!poll_timer->isActive())
            poll_timer->start();
    }
}

void TransmissionPoller::process_client_status(const QJsonObject& status, const QStringList& /*status_data*/)
{
    // 'status' will look like:
    //
    //    {
    //    "type" : "error",
    //    "data" : "This is the error message.",
    //    }
    //
    // or
    //
    //    {
    //    "type" : "status",
    //    "slots" : [
    //        {
    //            "slot" : 1,
    //            "done" : "100%",
    //            "have" : "3.61 GB",
    //            "eta" : "Unknown",
    //            "up" : "0.0",
    //            "down" : "0.0",
    //            "ratio" : "0.51",
    //            "status" : "Idle",
    //            "name" : "Approaching.the.Unknown.2016.1080p.WEB-DL.DD5.1.H264-FGT"
    //        },
    //        ...
    //        {
    //            "slot" : 10,
    //            "done" : "100%",
    //            "have" : "20.68 GB",
    //            "eta" : "Unknown",
    //            "up" : "0.0",
    //            "down" : "0.0",
    //            "ratio" : "0.58",
    //            "status" : "Seeding",
    //            "name" : "[OZC]Ghost in the Shell Stand Alone Complex Complete Series Batch + Bonus Materials"
    //        },
    //        ],
    //        "count" : 10,
    //    }

    if(!status["type"].toString().compare("error"))
        notify_interested_parties(-1, status["data"].toString());
    else if(!status["type"].toString().compare("status"))
    {
        int count = status["count"].toInt();
        float maxratio = 0.0f;
        if(status.contains("maxratio"))
            maxratio = (float)status["maxratio"].toDouble();
        QJsonArray slots_ = status["slots"].toArray();

        for(int i = 0;i < count;++i)
        {
            QJsonObject slot = slots_.at(i).toObject();
            notify_interested_parties(slot["slot"].toInt(), slot, maxratio);
        }
    }
}

void TransmissionPoller::slot_get_read()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        ReplyData& data = active_replies[reply];
        data.buffer += reply->readAll();
    }
}

void TransmissionPoller::slot_get_complete()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply && reply->error() == QNetworkReply::NoError)
    {
        ReplyData& data = active_replies[reply];
        data.buffer += reply->readAll();
        process_reply(reply);
    }

    if(reply)
    {
        if(active_replies.contains(reply))
            active_replies.remove(reply);

        reply->deleteLater();
        reply = nullptr;
    }
}

void TransmissionPoller::slot_get_failed(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(!active_replies.contains(reply))
        return;

    QString error_message;
    if(code == QNetworkReply::OperationCanceledError)
        error_message = tr("<b>Network Error</b><br>The network operation was canceled.");
    else if(code == QNetworkReply::AuthenticationRequiredError)
        error_message = tr("<b>Network Error</b><br>Authentication failed.<br>Did you enter your user name and password correctly?");
    else
        error_message = tr("<b>Network Error</b><br>A network error code %1 was returned for the last operation.").arg(code);

    notify_interested_parties(-1, error_message);
}

void TransmissionPoller::slot_request_pump()
{
    if(active_replies.count())
        return;

    if(requests.count())
    {
        RequestData rd = requests.front();
        requests.pop_front();
        create_request(rd.url, rd.state, rd.data);
    }
}

void TransmissionPoller::slot_poll()
{
    // we only request updates if there are active watchers
    if(interested_parties.isEmpty())
        return;

    foreach(const InterestData& data, interested_parties)
    {
        if(!data.party)
            continue;
        enqueue_request(target.toString(), ReplyStates::GettingStatus);
        break;
    }
}