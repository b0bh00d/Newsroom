#include "teamcity.h"

TeamCity::TeamCity(QObject *parent)
    : QNAM(nullptr),
      poll_timer(nullptr),
      poll_timeout(60),
      IPluginURL(parent)
{}

// IPlugin
QStringList TeamCity::DisplayName() const
{
    return QStringList() << QObject::tr("Team City v9") << QObject::tr("Supports the Team City REST API for v9.x");
}

QByteArray TeamCity::PluginID() const
{
    return "{A34020FD-80CC-48D4-9EC0-DFD52B912B2D}";
}

void TeamCity::SetStory(const QUrl& url)
{
    story = url;
}

bool TeamCity::CoverStory()
{
    if(QNAM)
        return false;       // they're calling us a second time

    error_message.clear();

    QNAM = new QNetworkAccessManager(this);

    // get projects listing first
    QString projects_url = QString("%1/httpAuth/app/rest/projects").arg(story.toString());
    create_request(projects_url, States::GettingProjects);

    return true;
}

bool TeamCity::FinishStory()
{
    error_message.clear();

    if(poll_timer)
    {
        poll_timer->stop();
        poll_timer->deleteLater();
        poll_timer = nullptr;
    }

    if(QNAM)
    {
        foreach(QNetworkReply* reply, active_replies.keys())
        {
            reply->abort();
            reply->deleteLater();
        }

        active_replies.clear();

        QNAM->deleteLater();
        QNAM = nullptr;
    }

    return false;
}

// IPluginURL
QStringList TeamCity::Requires() const
{
    // parameter names ending with an asterisk are required
    return QStringList() << "Username*"     << "string" <<
                            "Password*"     << "password" <<
                            "Project Name*" << "string" <<
                            "Builder"       << "string" <<      // if an explicit builder is not provided,
                                                                // then all the builders of the project will
                                                                // be monitored in the same Headline stream
                            "Polling (sec)" << "integer:60";    // how many seconds between polls? (default: 60)
}

bool TeamCity::SetRequirements(const QStringList& parameters)
{
    error_message.clear();

    if(parameters.count() < 3)
    {
        error_message = QStringLiteral("TeamCity: Not enough parameters provided.");
        return false;
    }

    username     = parameters[0];
    password     = parameters[1];
    project_name = parameters[2];
    if(parameters.count() > 3)
        builder_name = parameters[3];
    if(parameters.count() > 4 && !parameters[4].isEmpty())
    {
        poll_timeout = parameters[4].toInt();
        poll_timeout = poll_timeout < 30 ? 60 : poll_timeout;
    }

    if(username.isEmpty())
    {
        error_message = QStringLiteral("TeamCity: The Username parameter is required.");
        return false;
    }

    if(password.isEmpty())
    {
        error_message = QStringLiteral("TeamCity: The Password parameter is required.");
        return false;
    }

    if(project_name.isEmpty())
    {
        error_message = QStringLiteral("TeamCity: The Project ID parameter is required.");
        return false;
    }

    return true;
}

void TeamCity::create_request(const QString& url_str, States state)
{
    QUrl url(url_str);
    url.setUserName(username);
    url.setPassword(password);

    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Content-type"), QByteArray("application/json"));
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));

    QNetworkReply* reply = QNAM->get(request);
    bool connected = connect(reply, &QNetworkReply::readyRead, this, &TeamCity::slot_get_read);
    ASSERT_UNUSED(connected);
    connected = connect(reply, &QNetworkReply::finished, this, &TeamCity::slot_get_complete);
    ASSERT_UNUSED(connected);
    connected = connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                        this, &TeamCity::slot_get_failed);
    ASSERT_UNUSED(connected);

    ReplyData reply_data;
    reply_data.state = state;

    active_replies[reply] = reply_data;
}

void TeamCity::process_reply(QNetworkReply *reply)
{
    ReplyData& data = active_replies[reply];
    if(data.state == States::GettingProjects)
    {
        json_projects.clear();

        QJsonDocument d = QJsonDocument::fromJson(data.buffer);
        QJsonObject sett2 = d.object();
        int count = sett2["count"].toInt();
        QJsonArray project_array = sett2.value(QString("project")).toArray();
        for(int x = 0;x < count;++x)
        {
            QJsonObject project = project_array.at(x).toObject();
            QString id = project["id"].toString();
            if(id == "_Root")
                continue;
            QString name = project["name"].toString();
            if(!name.compare(project_name))
                json_projects[name] = project;
        }

        // get the builders for the project we are watching

        if(!json_projects.contains(project_name))
            error_message = QString("TeamCity: The project id \"%1\" was not found on server \"%2\".").arg(project_name).arg(story.toString());
        else
        {
            QJsonObject project = json_projects[project_name];

            QString builders_url = QString("%1%2").arg(story.toString()).arg(project["href"].toString());

            create_request(builders_url, States::GettingBuilders);
        }
    }
    else if(data.state == States::GettingBuilders)
    {
        json_builders.clear();

        QJsonDocument d = QJsonDocument::fromJson(data.buffer);
        QJsonObject sett2 = d.object();
        QJsonObject buildtypes = sett2["buildTypes"].toObject();
        int count = buildtypes["count"].toInt();
        QJsonArray builder_array = buildtypes["buildType"].toArray();
        for(int x = 0;x < count;++x)
        {
            QJsonObject builder = builder_array.at(x).toObject();
            QString name = builder["name"].toString();
            QString id = builder["id"].toString();
            if(builder_name.isEmpty() || !name.compare(builder_name))
                json_builders[id] = builder;
        }

        if(!builder_name.isEmpty() && json_builders.isEmpty())
        {
            error_message = QString("TeamCity: The build id \"%1\" was not found for project \"%2\" on server \"%3\".")
                                    .arg(builder_name)
                                    .arg(project_name)
                                    .arg(story.toString());
        }

        if(error_message.isEmpty())
        {
            // we can begin monitoring

            poll_timer = new QTimer(this);
            poll_timer->setInterval(poll_timeout * 1000);
            connect(poll_timer, &QTimer::timeout, this, &TeamCity::slot_poll);
            poll_timer->start();

            slot_poll();        // get an initial update
        }
    }
    else if(data.state == States::GettingStatus)
    {
        QJsonDocument d = QJsonDocument::fromJson(data.buffer);
        QJsonObject sett2 = d.object();
        process_status(sett2);
    }
    else if(data.state == States::GettingFinal)
        process_final(reply);
}

void TeamCity::process_status(const QJsonObject& status)
{
    // we have to produce a delta:  we have to check current builds
    // for new builds, and check cached status for builds that
    // have completed.

    QVector<QJsonObject> finals;
    QSet<int> c, p, finished_builds, new_builds;

    int count = status["count"].toInt();
    QJsonArray builder_array = status["build"].toArray();
    for(int x = 0;x < count;++x)
    {
        QJsonObject build = builder_array.at(x).toObject();
        c.insert(build["id"].toInt());
    }

    foreach(int build_key, build_status.keys())
        p.insert(build_key);

    // what previously watched builds are no longer in the current list?
    // we need to get final results for these
    finished_builds = p - c;

    // what new builds do we need to start watching?
    // these need to be added to the json_status[] list
    new_builds = c - p;

    for(int x = 0;x < count;++x)
    {
        QJsonObject build = builder_array.at(x).toObject();

        int build_number = build["number"].toString().toInt();
        int build_id = build["id"].toInt();

        uint now = QDateTime::currentDateTime().toTime_t();

        if(new_builds.contains(build_id))
        {
            // new build entry
            QString id = build["buildTypeId"].toString();//.split('_')[1];
            int complete = 0;
            if(build.contains("percentageComplete"))
                complete = build["percentageComplete"].toInt();

            QString status = QString("Project \"<b>%1</b>\" :: Builder \"<b>%2</b>\" :: Build #<b>%3</b><br>")
                                        .arg(project_name)
                                        .arg(builder_name.isEmpty() ? json_builders[id]["name"].toString() : builder_name)
                                        .arg(build_number);
            status += QString("State: %1<br>").arg(build["state"].toString());
            status += QString("Status: %1<br>").arg(build["status"].toString());
            status += QString("Completed: %1%").arg(complete);

            build_status[build_id] = build;

            ETAData eta_data;
            eta_data.start = now;
            eta_data.initial_completed = complete;
            eta_data.last_completed = complete;
            eta_data.last_changed = eta_data.start;

            eta[build_id] = eta_data;

            if(status.endsWith('\n'))
                status.chop(1);
            emit signal_new_data(status.toUtf8());
        }
        else if(!finished_builds.contains(build_id))
        {
            // pre-existing build
            QJsonObject cached_json = build_status[build_id];
            if(cached_json != build)
            {
                QString id = build["buildTypeId"].toString();//.split('_')[1];
                int complete = 0;
                if(build.contains("percentageComplete"))
                    complete = build["percentageComplete"].toInt();

                QString status = QString("Project \"<b>%1</b>\" :: Builder \"<b>%2</b>\" :: Build #<b>%3</b><br>")
                                        .arg(project_name)
                                        .arg(builder_name.isEmpty() ? json_builders[id]["name"].toString() : builder_name)
                                        .arg(build_number);

                // status change
                status += QString("State: %1<br>").arg(build["state"].toString());
                status += QString("Status: %1<br>").arg(build["status"].toString());
                status += QString("Completed: %1%").arg(complete);

                // update data for the hung-build check
                if(complete != eta[build_id].last_completed)
                {
                    eta[build_id].last_completed = complete;
                    eta[build_id].last_changed = now;
                }

                // after a few updates, calculate an eta

                uint completed_delta = complete - eta[build_id].initial_completed;
                if(completed_delta > 5)
                {
                    // how much clock time has passed since we started monitoring?
                    uint time_delta = now - eta[build_id].start;
                    uint average_per_point = time_delta / completed_delta;
                    uint percent_left = 100 - complete;
                    uint time_left = percent_left * average_per_point;
                    QDateTime target = QDateTime::currentDateTime();
                    QString eta_str = target.addSecs(time_left).toString("h:mm ap");

                    // check for hung build if no progress after 5 minutes
                    if((now - eta[build_id].last_changed) > 300)
                        status += QString(" / <b><font color=\"#ff0000\">ETA: %1</font></b>").arg(eta_str);
                    else
                        status += QString(" / <b>ETA: %1</b>").arg(eta_str);
                }

                build_status[build_id] = build;

                if(status.endsWith('\n'))
                    status.chop(1);
                emit signal_new_data(status.toUtf8());
            }
        }
    }

    if(finished_builds.count())
    {
        foreach(int build_id, finished_builds)
        {
            finals.push_back(build_status[build_id]);
            build_status.remove(build_id);
            eta.remove(build_id);
       }
    }

    // get final results for any finished builds
    if(finals.count())
    {
        // Make a call for each buildTypeId in the finals[]
        QMap<QString, int> buildTypeIds;
        foreach(QJsonObject build, finals)
        {
            QString buildTypeId = build["buildTypeId"].toString();
            if(buildTypeIds.contains(buildTypeId))
                buildTypeIds[buildTypeId] = 0;
            ++buildTypeIds[buildTypeId];
        }

        foreach(const QString& buildTypeId, buildTypeIds.keys())
        {
// curl -X GET --url "https://teamcity.lightwave3d.com/private_1/httpAuth/app/rest/buildTypes/id:DaveV_Windows/builds/running:false?count=1&start=0" --user "bhood:pylg>Swok4" --header "Content-type:application/json" --header "Accept:application/json"
            QString build_url = QString("%1/httpAuth/app/rest/buildTypes/id:%2/builds/running:false?count=%3&start=0")
                                            .arg(story.toString())
                                            .arg(buildTypeId)
                                            .arg(buildTypeIds[buildTypeId]);
            create_request(build_url, States::GettingFinal);
        }
    }
}

void TeamCity::process_final(QNetworkReply *reply)
{
    ReplyData& data = active_replies[reply];

    QJsonDocument d = QJsonDocument::fromJson(data.buffer);
    QJsonObject build = d.object();

    QString id = build["buildTypeId"].toString();
    int build_id = build["number"].toString().toInt();
    int complete = 100;
    if(build.contains("percentageComplete"))
        complete = build["percentageComplete"].toInt();

    QString status = QString("Project \"<b>%1</b>\" :: Builder \"<b>%2</b>\" :: Build #<b>%3</b><br>")
                                .arg(project_name)
                                .arg(builder_name.isEmpty() ? json_builders[id]["name"].toString() : builder_name)
                                .arg(build_id);

    // status change
    status += QString("State: %1<br>").arg(build["state"].toString());
    status += QString("Status: %1<br>").arg(build["status"].toString());
    status += QString("Completed: %1% @ %2").arg(complete).arg(QDateTime::currentDateTime().toString("h:mm ap"));

    emit signal_new_data(status.toUtf8());
}

void TeamCity::slot_get_read()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        ReplyData& data = active_replies[reply];
        data.buffer += reply->readAll();
    }
}

void TeamCity::slot_get_complete()
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

void TeamCity::slot_get_failed(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        ReplyData& data = active_replies[reply];
        QString reply_str(data.buffer);
    }

    if(code == QNetworkReply::OperationCanceledError)
        error_message = tr("<b>Network Error</b><br>The network operation was canceled.");
    else if(code == QNetworkReply::AuthenticationRequiredError)
        error_message = tr("<b>Network Error</b><br>Authentication failed.<br>Did you enter your user name and password correctly?");
    else
        error_message = tr("<b>Network Error</b><br>A network error code %1 was returned for the last operation.").arg(code);

    emit signal_new_data(error_message.toUtf8());
}

void TeamCity::slot_poll()
{
    QJsonObject project_json = json_projects[project_name];
    QString url_str = QString("%1/httpAuth/app/rest/builds?locator=project:%2,running:true,defaultFilter:false").arg(story.toString()).arg(project_json["id"].toString());
    create_request(url_str, States::GettingStatus);
}
