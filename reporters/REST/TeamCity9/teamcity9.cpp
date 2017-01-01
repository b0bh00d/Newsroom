#include "teamcity9.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

TeamCity9::TeamCity9(QObject *parent)
    : QNAM(nullptr),
      poll_timer(nullptr),
      poll_timeout(60),
      first_update(true),
      IReporter(parent)
{
    report_template << "Project \"<b>${PROJECT_NAME}</b>\" :: Builder \"<b>${BUILDER_NAME}</b>\" :: Build #<b>${BUILD_NUMBER}</b>";
    report_template << "State: ${STATE}";
    report_template << "Status: ${STATUS}";
    report_template << "Completed: ${COMPLETED}";
    report_template << "ETA: <b>${ETA}</b>";
}

// IPlugin
QStringList TeamCity9::DisplayName() const
{
    return QStringList() << QObject::tr("Team City v9") << QObject::tr("Supports the Team City REST API for v9.x");
}

QByteArray TeamCity9::PluginID() const
{
    return "{A34020FD-80CC-48D4-9EC0-DFD52B912B2D}";
}

QStringList TeamCity9::Requires() const
{
    // parameter names ending with an asterisk are required
    return QStringList() << "Username:*"     << "string" <<
                            "Password:*"     << "password" <<
                            "Project Name:*" << "string" <<

                            // if an explicit builder is not provided,
                            // then all the builders of the project will
                            // be monitored in the same Headline stream
                            "Builder:"       << "string" <<

                            // how many seconds between polls? (default: 60)
                            "Polling (sec):" << "integer:60" <<

                            "Format:"        << QString("multiline:%1").arg(report_template.join("<br>\n"));
}

bool TeamCity9::SetRequirements(const QStringList& parameters)
{
    error_message.clear();

    if(parameters.count() < 3)
    {
        error_message = QStringLiteral("TeamCity9: Not enough parameters provided.");
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
    if(parameters.count() > 5 && !parameters[5].isEmpty())
        report_template = parameters[5].split("<br>");

    if(username.isEmpty())
    {
        error_message = QStringLiteral("TeamCity9: The Username parameter is required.");
        return false;
    }

    if(password.isEmpty())
    {
        error_message = QStringLiteral("TeamCity9: The Password parameter is required.");
        return false;
    }

    if(project_name.isEmpty())
    {
        error_message = QStringLiteral("TeamCity9: The Project ID parameter is required.");
        return false;
    }

    return true;
}

void TeamCity9::SetStory(const QUrl& url)
{
    story = url;
}

bool TeamCity9::CoverStory()
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

bool TeamCity9::FinishStory()
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

    return true;
}

void TeamCity9::create_request(const QString& url_str, States state)
{
    QUrl url(url_str);
    url.setUserName(username);
    url.setPassword(password);

    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Content-type"), QByteArray("application/json"));
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));

    QNetworkReply* reply = QNAM->get(request);
    bool connected = connect(reply, &QNetworkReply::readyRead, this, &TeamCity9::slot_get_read);
    ASSERT_UNUSED(connected);
    connected = connect(reply, &QNetworkReply::finished, this, &TeamCity9::slot_get_complete);
    ASSERT_UNUSED(connected);
    connected = connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                        this, &TeamCity9::slot_get_failed);
    ASSERT_UNUSED(connected);

    ReplyData reply_data;
    reply_data.state = state;

    active_replies[reply] = reply_data;
}

void TeamCity9::process_reply(QNetworkReply *reply)
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
            error_message = QString("TeamCity9: The project id \"%1\" was not found on server \"%2\".").arg(project_name).arg(story.toString());
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
            error_message = QString("TeamCity9: The build id \"%1\" was not found for project \"%2\" on server \"%3\".")
                                    .arg(builder_name)
                                    .arg(project_name)
                                    .arg(story.toString());
        }

        if(error_message.isEmpty())
        {
            // we can begin monitoring

            poll_timer = new QTimer(this);
            poll_timer->setInterval(poll_timeout * 1000);
            connect(poll_timer, &QTimer::timeout, this, &TeamCity9::slot_poll);
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

void TeamCity9::process_status(const QJsonObject& status)
{
    int count = status["count"].toInt();
    if(count == 0 && first_update)
    {
        first_update = false;

        QString status = QString("Project \"<b>%1</b>\"").arg(project_name);
        if(!builder_name.isEmpty())
            status += QString(" :: Builder \"<b>%1</b>\"").arg(builder_name);
        status += "<br>State: idle";
        emit signal_new_data(status.toUtf8());

        return;
    }

    // we have to produce a delta:  we have to check current builds
    // for new builds, and check cached status for builds that
    // have completed.

    QVector<QJsonObject> finals;
    QSet<int> c, p, finished_builds, new_builds;

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

    ReportMap report_map;

    for(int x = 0;x < count;++x)
    {
        QJsonObject build = builder_array.at(x).toObject();

        int build_id = build["id"].toInt();

        uint now = QDateTime::currentDateTime().toTime_t();

        if(new_builds.contains(build_id))
        {
            // new build entry
            int complete = 0;
            if(build.contains("percentageComplete"))
                complete = build["percentageComplete"].toInt();

            build_status[build_id] = build;

            ETAData eta_data;
            eta_data.start = now;
            eta_data.initial_completed = complete;
            eta_data.last_completed = complete;
            eta_data.last_changed = eta_data.start;

            eta[build_id] = eta_data;

            populate_report_map(report_map, build, builder_name);
            QString status = render_report(report_map);

            emit signal_new_data(status.toUtf8());
        }
        else if(!finished_builds.contains(build_id))
        {
            // pre-existing build
            QJsonObject cached_json = build_status[build_id];
            if(cached_json != build)
            {
                int complete = 0;
                if(build.contains("percentageComplete"))
                    complete = build["percentageComplete"].toInt();

                // update data for the hung-build check
                if(complete != eta[build_id].last_completed)
                {
                    eta[build_id].last_completed = complete;
                    eta[build_id].last_changed = now;
                }

                // after a few updates, calculate an eta

                QString eta_str;
                uint completed_delta = complete - eta[build_id].initial_completed;
                if(completed_delta > 5)
                {
                    // how much clock time has passed since we started monitoring?
                    uint time_delta = now - eta[build_id].start;
                    uint average_per_point = time_delta / completed_delta;
                    uint percent_left = 100 - complete;
                    uint time_left = percent_left * average_per_point;
                    QDateTime target = QDateTime::currentDateTime();
                    eta_str = target.addSecs(time_left).toString("h:mm ap");

                    // check for hung build if no progress after 5 minutes
                    if((now - eta[build_id].last_changed) > 300)
                        eta_str = QString("<font color=\"#ff0000\">%1</font>").arg(eta_str);
                }

                build_status[build_id] = build;

                populate_report_map(report_map, build, builder_name, eta_str);
                QString status = render_report(report_map);

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
// curl -X GET --url "https://TeamCity9.lightwave3d.com/private_1/httpAuth/app/rest/buildTypes/id:DaveV_Windows/builds/running:false?count=1&start=0" --user "bhood:pylg>Swok4" --header "Content-type:application/json" --header "Accept:application/json"
            QString build_url = QString("%1/httpAuth/app/rest/buildTypes/id:%2/builds/running:false?count=%3&start=0")
                                            .arg(story.toString())
                                            .arg(buildTypeId)
                                            .arg(buildTypeIds[buildTypeId]);
            create_request(build_url, States::GettingFinal);
        }
    }

    first_update = false;
}

void TeamCity9::process_final(QNetworkReply *reply)
{
    ReplyData& data = active_replies[reply];

    ReportMap report_map;

    QJsonDocument d = QJsonDocument::fromJson(data.buffer);
    QJsonObject build = d.object();

    populate_report_map(report_map, build, builder_name);
    QString status = render_report(report_map);

    emit signal_new_data(status.toUtf8());
}

void TeamCity9::slot_get_read()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        ReplyData& data = active_replies[reply];
        data.buffer += reply->readAll();
    }
}

void TeamCity9::populate_report_map(ReportMap& report_map,
                                   const QJsonObject& build,
                                   const QString& builder_id,
                                   const QString& eta_str)
{
    report_map.clear();

    report_map["PROJECT_NAME"] = project_name;
    if(!builder_name.isEmpty())
        report_map["BUILDER_NAME"] = builder_name;
    else if(!builder_id.isEmpty() && json_builders.contains(builder_id))
        report_map["BUILDER_NAME"] = json_builders[builder_id]["name"].toString();
    else if(json_builders.count() == 1)
        report_map["BUILDER_NAME"] = json_builders[json_builders.keys()[0]]["name"].toString();
    report_map["BUILDER_ID"] = builder_id;
    report_map["BUILD_ID"] = QString::number(build["id"].toInt());
    report_map["BUILD_NUMBER"] = build["number"].toString();
    report_map["STATE"] = build["state"].toString();
    report_map["STATUS"] = build["status"].toString();
    if(!report_map["STATE"].compare("finished"))
        report_map["COMPLETED"] = QString("100% @ %1").arg(QDateTime::currentDateTime().toString("h:mm ap"));
    else
        report_map["COMPLETED"] = QString("%1%").arg(QString::number(build["percentageComplete"].toInt()));
    if(eta_str.isEmpty() && !report_map["STATE"].compare("running"))
        report_map["ETA"] = "(pending)";
    else
        report_map["ETA"] = eta_str;
}

QString TeamCity9::render_report(const ReportMap& report_map)
{
    QString report = report_template.join("<br>\n");
    foreach(const QString& key, report_map.keys())
    {
        QString token = QString("${%1}").arg(key);
        report.replace(token, report_map[key]);
    }

    return report;
}

void TeamCity9::slot_get_complete()
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

void TeamCity9::slot_get_failed(QNetworkReply::NetworkError code)
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

void TeamCity9::slot_poll()
{
    QString url_str;
    if(builder_name.isEmpty())
    {
        QJsonObject project_json = json_projects[project_name];
        url_str = QString("%1/httpAuth/app/rest/builds?locator=project:%2,running:true,defaultFilter:false")
                            .arg(story.toString())
                            .arg(project_json["id"].toString());
    }
    else
        url_str = QString("%1/httpAuth/app/rest/builds?locator=buildType:(id:%2),running:true,defaultFilter:false")
                            .arg(story.toString())
                            .arg(json_builders.keys()[0]);
    create_request(url_str, States::GettingStatus);
}
