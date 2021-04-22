#include "chartapipoller.h"
#include "chartapi.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

YahooChartAPIPoller::YahooChartAPIPoller(TickerFormat format, const QUrl& target, const QString& ticker, int timeout, QObject *parent)
    : QObject(parent),
      format(format),
      ticker(ticker),
      poll_timeout(timeout)
{
    // fix up the URL so we can add our own query elements
    auto story_scheme = target.scheme();
    auto story_authority = target.authority();
    auto story_path = target.path().split("/");
    while(story_path.front().isEmpty())
        story_path.pop_front();

    // first element should be...
    if(story_path[0].compare("instrument"))
        story_path.clear();     // don't recognize that; we'll just construct our own
    else
    {
        // next element should be a version
        auto ok{false};
        story_path[1].toFloat(&ok);
        if(!ok)
            story_path.clear(); // don't recognize that; we'll just construct our own
        else
        {
            // after here, we will be using our own stuff
            while(story_path.length() > 2)
                story_path.pop_back();
        }
    }

    if(story_path.isEmpty())
        story_path << "instrument" << "1.0";
    story_path << QUrl::toPercentEncoding(ticker, "", "^") << "chartdata;type=quote;range=1d";

    switch(format)
    {
        case TickerFormat::CSV:
            story_path << "csv";
            break;
        case TickerFormat::JSON:
            story_path << "json";
            break;
        case TickerFormat::XML:
            story_path << "xml";
            break;
        case TickerFormat::None:
            break;
    }

    this->target = QString("%1://%2/%3/").arg(story_scheme).arg(story_authority).arg(story_path.join("/"));

    QNAM = new QNetworkAccessManager(this);

    request_timer = new QTimer(this);
    request_timer->setInterval(200);
    connect(request_timer, &QTimer::timeout, this, &YahooChartAPIPoller::slot_request_pump);
    request_timer->start();

    poll_timer = new QTimer(this);
    poll_timer->setInterval(poll_timeout * 1000);
    connect(poll_timer, &QTimer::timeout, this, &YahooChartAPIPoller::slot_poll);
    poll_timer->start();

    // get the first update on our ticker symbol
//    enqueue_request(QString("%1/httpAuth/app/rest/projects").arg(target.toString()), ReplyStates::GettingUpdate);

//    enqueue_request_unique(this->target, ReplyStates::GettingUpdate);
}

YahooChartAPIPoller::~YahooChartAPIPoller()
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

void YahooChartAPIPoller::add_interest(const QString& ticker, QObject* me, int flags)
{
    // make sure it's a YahooChartAPI instance
    auto chartapi = dynamic_cast<YahooChartAPI*>(me);
    if(!chartapi)
        return;

    if(!interested_parties.contains(ticker))
        interested_parties[ticker] = InterestedList();

    InterestData id{me, flags};
    interested_parties[ticker].append(id);

    if(poll_timer->isActive())
    {
        poll_timer->start();    // reset it
        slot_poll();            // get a new update now
    }
}

void YahooChartAPIPoller::remove_interest(const QString& ticker, QObject* me)
{
    if(!interested_parties.contains(ticker))
        return;

    for(auto i = 0;i < interested_parties[ticker].length();++i)
    {
        if(interested_parties[ticker][i].party == me)
        {
            interested_parties[ticker].removeAt(i);
            break;
        }
    }
}

void YahooChartAPIPoller::notify_interested_parties(const QString& ticker, const QString& message)
{
    if(ticker.isEmpty())
    {
        // notify them all
        foreach(const QString& key, interested_parties.keys())
        {
            auto& interested = interested_parties[key];
            for(auto iter = interested.begin();iter != interested.end();++iter)
            {
                auto chartapi = dynamic_cast<YahooChartAPI*>(iter->party);
                if(!chartapi)
                    continue;
                chartapi->error(message);
            }
        }

        return;
    }

    if(interested_parties.contains(ticker))
    {
        auto& interested = interested_parties[ticker];
        for(auto iter = interested.begin();iter != interested.end();++iter)
        {
            auto chartapi = dynamic_cast<YahooChartAPI*>(iter->party);
            if(!chartapi)
                continue;
            chartapi->error(message);
        }
    }
}

void YahooChartAPIPoller::notify_interested_parties(TickerEvents event, const QString& ticker, const QJsonObject& status)
{
    if(interested_parties.contains(ticker))
    {
        auto& interested = interested_parties[ticker];
        for(auto iter = interested.begin();iter != interested.end();++iter)
        {
            auto chartapi = dynamic_cast<YahooChartAPI*>(iter->party);
            if(!chartapi)
                continue;

            switch(event)
            {
                case TickerEvents::Update:
                    chartapi->ticker_update(status);
                    break;
                default:
                    break;
            }
        }
    }
}

void YahooChartAPIPoller::notify_interested_parties(TickerEvents event, const QString& ticker, const QDomDocument& status)
{
    if(interested_parties.contains(ticker))
    {
        auto& interested = interested_parties[ticker];
        for(auto iter = interested.begin();iter != interested.end();++iter)
        {
            auto chartapi = dynamic_cast<YahooChartAPI*>(iter->party);
            if(!chartapi)
                continue;

            switch(event)
            {
                case TickerEvents::Update:
                    chartapi->ticker_update(status);
                    break;
                default:
                    break;
            }
        }
    }
}

void YahooChartAPIPoller::notify_interested_parties(TickerEvents event, const QString& ticker, const QString& status)
{
    if(interested_parties.contains(ticker))
    {
        auto& interested = interested_parties[ticker];
        for(auto iter = interested.begin();iter != interested.end();++iter)
        {
            auto chartapi = dynamic_cast<YahooChartAPI*>(iter->party);
            if(!chartapi)
                continue;

            switch(event)
            {
                case TickerEvents::Update:
                    chartapi->ticker_update(status);
                    break;
                default:
                    break;
            }
        }
    }
}


void YahooChartAPIPoller::enqueue_request(const QString& url_str, ReplyStates state, const QStringList& request_data, Priorities priority)
{
    RequestData rd{state, url_str, request_data};
    if(priority == Priorities::BackOfQueue)
        requests.push_back(rd);
    else
        requests.insert(0, rd);
    pending_requests[rd.url] = true;
}

void YahooChartAPIPoller::enqueue_request_unique(const QString& url_str, ReplyStates state, const QStringList& request_data)
{
    if(pending_requests.contains(url_str))
        return;
    enqueue_request(url_str, state, request_data);
}

void YahooChartAPIPoller::create_request(const QString& url_str, ReplyStates state, const QStringList& request_data)
{
    QUrl url(url_str);

    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Content-type"), QByteArray("application/json"));
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));

    auto reply = QNAM->get(request);
    auto connected = connect(reply, &QNetworkReply::readyRead, this, &YahooChartAPIPoller::slot_get_read);
    ASSERT_UNUSED(connected)
    connected = connect(reply, &QNetworkReply::finished, this, &YahooChartAPIPoller::slot_get_complete);
    ASSERT_UNUSED(connected)
    connected = connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                        this, &YahooChartAPIPoller::slot_get_failed);
    ASSERT_UNUSED(connected)

    ReplyData reply_data{state, QByteArray(), request_data};
    active_replies[reply] = reply_data;

    if(pending_requests.contains(url_str))
        pending_requests.remove(url_str);
}

void YahooChartAPIPoller::process_reply(QNetworkReply *reply)
{
    auto& data = active_replies[reply];
    if(data.state == ReplyStates::GettingUpdate)
    {
        if(format == TickerFormat::CSV)
            notify_interested_parties(TickerEvents::Update, ticker, QString(data.buffer));
        else if(format == TickerFormat::JSON)
        {
            auto d = QJsonDocument::fromJson(data.buffer);
            if(!d.isNull())
            {
                auto sett2 = d.object();
                if(!sett2.isEmpty())
                    notify_interested_parties(TickerEvents::Update, ticker, sett2);
            }
        }
        else if(format == TickerFormat::XML)
            notify_interested_parties(TickerEvents::Update, ticker, QString(data.buffer));
    }
}

void YahooChartAPIPoller::slot_get_read()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        auto& data = active_replies[reply];
        data.buffer += reply->readAll();
    }
}

void YahooChartAPIPoller::slot_get_complete()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    if(reply && reply->error() == QNetworkReply::NoError)
    {
        auto& data = active_replies[reply];
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

void YahooChartAPIPoller::slot_get_failed(QNetworkReply::NetworkError code)
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    if(!active_replies.contains(reply))
        return;

    QString error_message;
    if(code == QNetworkReply::OperationCanceledError)
        error_message = tr("<b>Network Error</b><br>The network operation was canceled.");
    else if(code == QNetworkReply::AuthenticationRequiredError)
        error_message = tr("<b>Network Error</b><br>Authentication failed.<br>Did you enter your user name and password correctly?");
    else
        error_message = tr("<b>Network Error</b><br>A network error code %1 was returned for the last operation.").arg(code);

    notify_interested_parties(ticker, error_message);
}

void YahooChartAPIPoller::slot_request_pump()
{
    if(active_replies.count())
        return;

    if(requests.count())
    {
        auto rd = requests.front();
        requests.pop_front();
        create_request(rd.url, rd.state, rd.data);
    }
}

void YahooChartAPIPoller::slot_poll()
{
    // we only request updates for those builders that are actively being watched
    if(interested_parties.isEmpty())
        return;

    enqueue_request_unique(target, ReplyStates::GettingUpdate);
}
