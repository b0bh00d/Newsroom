#include "teamcity9poller.h"
#include "teamcity9.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

TeamCity9Poller::TeamCity9Poller(const QUrl& target, const QString& username, const QString& password, int timeout, QObject *parent)
    : QObject(parent),
      target(target),
      username(username),
      password(password),
      poll_timeout(timeout)
{
    QNAM = new QNetworkAccessManager(this);

    request_timer = new QTimer(this);
    request_timer->setInterval(200);
    connect(request_timer, &QTimer::timeout, this, &TeamCity9Poller::slot_request_pump);
    request_timer->start();

    poll_timer = new QTimer(this);
    poll_timer->setInterval(poll_timeout * 1000);
    connect(poll_timer, &QTimer::timeout, this, &TeamCity9Poller::slot_poll);

    // prime the pump and get projects listing first
    enqueue_request(QString("%1/httpAuth/app/rest/projects").arg(target.toString()), ReplyStates::GettingProjects);
}

TeamCity9Poller::~TeamCity9Poller()
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

void TeamCity9Poller::add_interest(const QString& project_name, const QString& builder_name, QObject* me, int flags)
{
    // make sure it's a TeamCity9 instance
    auto teamcity9 = dynamic_cast<TeamCity9*>(me);
    if(!teamcity9)
        return;

    auto key = QString("%1::%2").arg(project_name.toLower()).arg(builder_name.toLower());
    if(!interested_parties.contains(key))
        interested_parties[key] = InterestedList();

    InterestData id;
    id.party = me;
    id.flags = flags;

    interested_parties[key].append(id);

    if(poll_timer->isActive())
    {
        poll_timer->start();    // reset it
        slot_poll();            // get a new update now
    }
}

void TeamCity9Poller::remove_interest(const QString& project_name, const QString& builder_name, QObject* me)
{
    auto key = QString("%1::%2").arg(project_name.toLower()).arg(builder_name.toLower());
    if(!interested_parties.contains(key))
        return;

    for(auto i = 0;i < interested_parties[key].length();++i)
    {
        if(interested_parties[key][i].party == me)
        {
            interested_parties[key].removeAt(i);
            break;
        }
    }
}

void TeamCity9Poller::notify_interested_parties(const QString& project_name, const QString& builder_name, const QString& message)
{
    if(project_name.isEmpty() && builder_name.isEmpty())
    {
        // notify them all
        foreach(const QString& key, interested_parties.keys())
        {
            auto& interested = interested_parties[key];
            for(auto iter = interested.begin();iter != interested.end();++iter)
            {
                auto teamcity9 = dynamic_cast<TeamCity9*>(iter->party);
                if(!teamcity9)
                    continue;
                teamcity9->error(message);
            }
        }

        return;
    }

    QStringList keys;
    keys << QString("%1::").arg(project_name.toLower());
    if(!builder_name.isEmpty())
        keys << QString("%1::%2").arg(project_name.toLower()).arg(builder_name.toLower());

    foreach(const QString& key, keys)
    {
        if(!interested_parties.contains(key))
            continue;

        auto& interested = interested_parties[key];
        for(auto iter = interested.begin();iter != interested.end();++iter)
        {
            auto teamcity9 = dynamic_cast<TeamCity9*>(iter->party);
            if(!teamcity9)
                continue;
            teamcity9->error(message);
        }
    }
}

void TeamCity9Poller::notify_interested_parties(BuilderEvents event, const QString& project_name, const QString& builder_name, const QJsonObject& status)
{
    QStringList keys;
    keys << QString("%1::").arg(project_name.toLower());
    if(!builder_name.isEmpty())
        keys << QString("%1::%2").arg(project_name.toLower()).arg(builder_name.toLower());

    foreach(const QString& key, keys)
    {
        if(!interested_parties.contains(key))
            continue;

        auto& interested = interested_parties[key];
        for(auto iter = interested.begin();iter != interested.end();++iter)
        {
            auto teamcity9 = dynamic_cast<TeamCity9*>(iter->party);
            if(!teamcity9)
                continue;

            switch(event)
            {
                case BuilderEvents::BuildPending:
                    if((iter->flags & Interest::PendingChanges) != 0)
                        teamcity9->build_pending(status);
                    break;
                case BuilderEvents::BuildStarted:
                    teamcity9->build_started(status);
                    break;
                case BuilderEvents::BuildProgress:
                    teamcity9->build_progress(status);
                    break;
                case BuilderEvents::BuildFinal:
                    teamcity9->build_final(status);
                    break;
                default:
                    break;
            }
        }
    }
}

void TeamCity9Poller::enqueue_request(const QString& url_str, ReplyStates state, const QStringList& request_data, Priorities priority)
{
    RequestData rd{state, url_str, request_data};
    if(priority == Priorities::BackOfQueue)
        requests.push_back(rd);
    else
        requests.insert(0, rd);
    pending_requests[rd.url] = true;
}

void TeamCity9Poller::enqueue_request_unique(const QString& url_str, ReplyStates state, const QStringList& request_data)
{
    if(pending_requests.contains(url_str))
        return;
    enqueue_request(url_str, state, request_data);
}

void TeamCity9Poller::create_request(const QString& url_str, ReplyStates state, const QStringList& request_data)
{
    QUrl url(url_str);
    url.setUserName(username);
    url.setPassword(password);

    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Content-type"), QByteArray("application/json"));
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));

    auto reply = QNAM->get(request);
    auto connected = connect(reply, &QNetworkReply::readyRead, this, &TeamCity9Poller::slot_get_read);
    ASSERT_UNUSED(connected)
    connected = connect(reply, &QNetworkReply::finished, this, &TeamCity9Poller::slot_get_complete);
    ASSERT_UNUSED(connected)
    connected = connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                        this, &TeamCity9Poller::slot_get_failed);
    ASSERT_UNUSED(connected)

    ReplyData reply_data{state, QByteArray(), request_data};
    active_replies[reply] = reply_data;

    if(pending_requests.contains(url_str))
        pending_requests.remove(url_str);
}

void TeamCity9Poller::process_reply(QNetworkReply *reply)
{
    auto& data = active_replies[reply];
    if(data.state == ReplyStates::GettingProjects)
    {
        projects.clear();

        auto d = QJsonDocument::fromJson(data.buffer);
        auto sett2 = d.object();
        auto count = sett2["count"].toInt();
        auto project_array = sett2.value(QString("project")).toArray();
        for(auto x = 0;x < count;++x)
        {
            auto project = project_array.at(x).toObject();
            auto project_id = project["id"].toString();
            if(project_id == "_Root")
                continue;

            projects[project_id] = ProjectData();

            enqueue_request(QString("%1%2").arg(target.toString()).arg(project["href"].toString()), ReplyStates::GettingBuilders);
            ++replies_expected;
        }
    }
    else if(data.state == ReplyStates::GettingBuilders)
    {
        auto d = QJsonDocument::fromJson(data.buffer);
        auto project_data = d.object();
        auto project_id = project_data["id"].toString();

        auto& pd = projects[project_id];
        pd.project_data = project_data;
        auto project_name = project_data["name"].toString();

        auto buildtypes = project_data["buildTypes"].toObject();
        auto count = buildtypes["count"].toInt();
        auto builder_array = buildtypes["buildType"].toArray();
        for(auto x = 0;x < count;++x)
        {
            BuilderData bd;
            bd.builder_data = builder_array.at(x).toObject();
            pd.builders.append(bd);
        }

        if(--replies_expected == 0)
        {
            // we're loaded with all our data: we can begin monitoring
            if(!poll_timer->isActive())
            {
                slot_poll();        // get an initial update for all builders
                poll_timer->start();
            }
        }
    }
    else if(data.state == ReplyStates::GettingBuilderStatus)
    {
        // got a status update for a specific Project::Builder
        auto d = QJsonDocument::fromJson(data.buffer);
        auto sett2 = d.object();
        process_builder_status(sett2, data.data);
    }
    else if(data.state == ReplyStates::GettingBuildPending)
    {
        // got a status update for a specific build id
        auto d = QJsonDocument::fromJson(data.buffer);
        auto sett2 = d.object();
        process_build_pending(sett2, data.data);
    }
    else if(data.state == ReplyStates::GettingBuildStatus)
    {
        // got a status update for a specific build id
        auto d = QJsonDocument::fromJson(data.buffer);
        auto sett2 = d.object();
        process_build_status(sett2, data.data);
    }
    else if(data.state == ReplyStates::GettingBuildFinal)
    {
        auto d = QJsonDocument::fromJson(data.buffer);
        auto sett2 = d.object();
        process_build_final(sett2, data.data);
    }
}

bool TeamCity9Poller::any_interest_in_changes_check(const QString& project_name, const QString& builder_name)
{
    // see if interested parties for this project::builder want
    // notifications of pending changes

    QStringList keys;
    keys << QString("%1::").arg(project_name.toLower());
    keys << QString("%1::%2").arg(project_name.toLower()).arg(builder_name.toLower());

    auto check_pending{false};
    foreach(const QString& key, keys)
    {
        if(!interested_parties.contains(key))
            continue;

        auto& interested = interested_parties[key];
        for(auto interested_iter = interested.begin();interested_iter != interested.end();++interested_iter)
        {
            if((interested_iter->flags & Interest::PendingChanges) != 0)
            {
                check_pending = true;
                break;
            }
        }

        if(check_pending)
            break;
    }

    return check_pending;
}

void TeamCity9Poller::process_builder_status(const QJsonObject& status, const QStringList& status_data)
{
    // 'status' will be about any builds running on a specific builder
    // (builders can have more than one build), something like this:
    // {
    //   "count":1,
    //   "href":"/httpAuth/app/rest/builds?locator=buildType:(id:DaveV_Windows),running:true,defaultFilter:false",
    //   "build":
    //   [
    //     {
    //       "id":12382,
    //       "buildTypeId":"DaveV_Windows",
    //       "number":"246",
    //       "status":"SUCCESS",
    //       "state":"running",
    //       "running":true,
    //       "percentageComplete":51,
    //       "href":"/httpAuth/app/rest/builds/id:12382",
    //       "webUrl":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/viewLog.html?buildId=12382&buildTypeId=DaveV_Windows"
    //     }
    //   ]
    // }

    auto& project = projects[status_data[0]];
    auto builder = project.builders.begin();
    for(;builder != project.builders.end();++builder)
    {
        if(!builder->builder_data["id"].toString().compare(status_data[1]))
            break;
    }
    Q_ASSERT(builder != project.builders.end());

    auto project_name = project.project_data["name"].toString();
    auto builder_name = builder->builder_data["name"].toString();

    auto count = status["count"].toInt();
    if(count == 0 && builder->build_status.isEmpty())
    {
        if(!builder->pause_pending_changes_check && any_interest_in_changes_check(project_name, builder_name))
        {
            builder->build_event = BuilderEvents::BuildPending;
            ++builder->pending_changes_check_count;

            auto url = QString("%1/httpAuth/app/rest/changes?locator=buildType:(id:%2),pending:true").arg(target.toString()).arg(builder->builder_data["id"].toString());
            enqueue_request_unique(url, ReplyStates::GettingBuildPending, QStringList() << status_data[0] << builder->builder_data["id"].toString());
        }

        return;
    }

    // we have to produce a delta:  we have to check current builds
    // for new builds, and check cached status for builds that
    // have completed.

    QSet<int> c, p, finished_builds, new_builds;

    auto builder_array = status["build"].toArray();
    for(auto x = 0;x < count;++x)
    {
        auto build = builder_array.at(x).toObject();
        c.insert(build["id"].toInt());
    }

    foreach(auto build_key, builder->build_status.keys())
        p.insert(build_key);

    // what previously watched builds are no longer in the current list?
    // we need to get final results for these
    finished_builds = p - c;

    // what new builds do we need to start watching?
    // these need to be added to the build_status[] list
    new_builds = c - p;

    for(auto x = 0;x < count;++x)
    {
        auto build = builder_array.at(x).toObject();

        auto build_id = build["id"].toInt();

        if(new_builds.contains(build_id))
        {
            builder->pause_pending_changes_check = false;

            builder->build_status[build_id] = build;
            builder->build_event = BuilderEvents::BuildStarted;

            auto url = QString("%1/httpAuth/app/rest/buildQueue/id:%2").arg(target.toString()).arg(build_id);
            enqueue_request_unique(url, ReplyStates::GettingBuildStatus, QStringList() << status_data[0] << QString::number(build_id));
        }
        else if(!finished_builds.contains(build_id))
        {
            // active build
            auto cached_json = builder->build_status[build_id];
            // is there a change since the last poll?
            if(cached_json != build)
            {
                // yes

                builder->build_status[build_id] = build;
                builder->build_event = BuilderEvents::BuildProgress;

                auto url = QString("%1/httpAuth/app/rest/buildQueue/id:%2").arg(target.toString()).arg(build_id);
                enqueue_request_unique(url, ReplyStates::GettingBuildStatus, QStringList() << status_data[0] << QString::number(build_id));
            }
        }
    }

    QVector<int> finals;
    if(finished_builds.count())
    {
        foreach(auto build_id, finished_builds)
        {
            finals.push_back(build_id);
            builder->build_status.remove(build_id);
       }
    }

    // get final results for all finished builds as priorities
    if(finals.count())
    {
        foreach(auto build_id, finals)
            create_request(QString("%1/httpAuth/app/rest/builds/id:%2").arg(target.toString()).arg(build_id),
                            ReplyStates::GettingBuildFinal,
                            status_data);
    }
}

void TeamCity9Poller::process_build_pending(const QJsonObject& status, const QStringList& status_data)
{
    // 'status' will be a detailed listing about pending changes for an idle builder

    auto& project = projects[status_data[0]];
    auto builder = project.builders.begin();
    for(;builder != project.builders.end();++builder)
    {
        if(!builder->builder_data["id"].toString().compare(status_data[1]))
            break;
    }
    Q_ASSERT(builder != project.builders.end());

    if(status["count"].toInt() != 0)
    {
        notify_interested_parties(builder->build_event, project.project_data["name"].toString(), builder->builder_data["name"].toString(), status);

        // if there are more pending changes to be retrieved, then we've
        // hit the single-call limit.  disable further pending checks
        // because the value displayed to the user will never change
        // (e.g., "100+"), and we'll just needlessly pull data from
        // the server

        if(status.contains("nextHref") ||

        // if the user wants to limit the number of pending changes
        // checks (to reduce data usage, for example), we will pause
        // checks if at least one has been completed.  (note that this
        // is not yet implemented upstream.)

           (builder->limit_pending_changes_check && builder->pending_changes_check_count))
        {
            builder->pending_changes_check_count = 0;
            builder->pause_pending_changes_check = true;
        }
    }
}

void TeamCity9Poller::process_build_status(const QJsonObject& status, const QStringList& status_data)
{
    // 'status' will be a detailed listing about a specific build

    auto& project = projects[status_data[0]];
    auto builder = project.builders.begin();
    for(;builder != project.builders.end();++builder)
    {
        if(builder->build_status.contains(status_data[1].toInt()))
            break;
    }
    Q_ASSERT(builder != project.builders.end());

    notify_interested_parties(builder->build_event, project.project_data["name"].toString(), builder->builder_data["name"].toString(), status);
}

void TeamCity9Poller::process_build_final(const QJsonObject& status, const QStringList &status_data)
{
    // 'status' will be a detailed (result) report for a single build, something like:
    // {
    //   "id":12412,
    //   "buildTypeId":"DaveV_Windows",
    //   "number":"249",
    //   "status":"SUCCESS",
    //   "state":"finished",
    //   "href":"/httpAuth/app/rest/builds/id:12412",
    //   "webUrl":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/viewLog.html?buildId=12412&buildTypeId=DaveV_Windows",
    //   "statusText":"Success",
    //   "buildType":
    //   {
    //     "id":"DaveV_Windows",
    //     "name":"Windows",
    //     "projectName":"DaveV",
    //     "projectId":"DaveV",
    //     "href":"/httpAuth/app/rest/buildTypes/id:DaveV_Windows",
    //     "webUrl":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/viewType.html?buildTypeId=DaveV_Windows"
    //   },
    //   "queuedDate":"20161213T193133+0000",
    //   "startDate":"20161213T193134+0000",
    //   "finishDate":"20161213T205325+0000",
    //   ...

    auto& project = projects[status_data[0]];
    auto builder = project.builders.begin();
    for(;builder != project.builders.end();++builder)
    {
        if(!builder->builder_data["id"].toString().compare(status_data[1]))
            break;
    }
    Q_ASSERT(builder != project.builders.end());

    notify_interested_parties(BuilderEvents::BuildFinal, project.project_data["name"].toString(), builder->builder_data["name"].toString(), status);
}

void TeamCity9Poller::slot_get_read()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        auto& data = active_replies[reply];
        data.buffer += reply->readAll();
    }
}

void TeamCity9Poller::slot_get_complete()
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

void TeamCity9Poller::slot_get_failed(QNetworkReply::NetworkError code)
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

    auto& data = active_replies[reply];
    if(!data.data.isEmpty())
    {
        auto& pd = projects[data.data[0]];
        auto builder_data = pd.builders.begin();
        for(;builder_data != pd.builders.end();++builder_data)
        {
            if(!builder_data->builder_data["id"].toString().compare(data.data[1]))
                break;
        }
        Q_ASSERT(builder_data != pd.builders.end());

        notify_interested_parties(pd.project_data["name"].toString(), builder_data->builder_data["name"].toString(), error_message);
    }
    else
        notify_interested_parties(QString(), QString(), error_message);
}

void TeamCity9Poller::slot_request_pump()
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

void TeamCity9Poller::slot_poll()
{
    // we only request updates for those builders that are actively being watched
    if(interested_parties.isEmpty())
        return;

    foreach(const QString& key, projects.keys())
    {
        auto& project = projects[key];
        for(auto builder = project.builders.begin();builder != project.builders.end();++builder)
        {
            auto project_name = project.project_data["name"].toString();
            auto builder_name = builder->builder_data["name"].toString();

            auto project_key = QString("%1::").arg(project_name.toLower());
            auto builder_key = QString("%1::%2").arg(project_name.toLower()).arg(builder_name.toLower());

            if(interested_parties.contains(project_key.toLower()) || interested_parties.contains(builder_key.toLower()))
            {
                auto builder_id = builder->builder_data["id"].toString();

                auto url = QString("%1/httpAuth/app/rest/builds?locator=buildType:(id:%2),running:true,defaultFilter:false")
                                        .arg(target.toString())
                                        .arg(builder_id);
                enqueue_request_unique(url, ReplyStates::GettingBuilderStatus, QStringList() << key << builder_id);
            }
        }
    }
}
