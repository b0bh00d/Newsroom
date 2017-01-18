#include "teamcity9.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

TeamCity9::PollerMap TeamCity9::poller_map;

TeamCity9::TeamCity9(QObject *parent)
    : poll_timeout(60),
      IReporter(parent)
{
    report_template << "Project \"<b>${PROJECT_NAME}</b>\" :: Builder \"<b>${BUILDER_NAME}</b>\" :: Build #<b>${BUILD_NUMBER}</b>";
    report_template << "State: ${STATE}";
    report_template << "Status: ${STATUS}";
    report_template << "Completed: ${COMPLETED}";
    report_template << "ETA: <b>${ETA}</b>";
}

// IReporter

QStringList TeamCity9::DisplayName() const
{
    return QStringList() << QObject::tr("Team City v9") <<
                            QObject::tr("Supports the Team City REST API for v9.x");
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
                            "Polling (sec):" << QString("integer:%1").arg(poll_timeout) <<

                            "Format:"        << QString("multiline:%1").arg(report_template.join("<br>\n"));
}

bool TeamCity9::SetRequirements(const QStringList& parameters)
{
    error_message.clear();

    if(parameters.count() < Param::Builder)
    {
        error_message = QStringLiteral("TeamCity9: Not enough parameters provided.");
        return false;
    }

    username     = parameters[Param::Username];
    password     = QString(QByteArray::fromHex(parameters[Param::Password].toUtf8()));
    project_name = parameters[Param::Project];
    if(parameters.count() > Param::Builder)
        builder_name = parameters[Param::Builder];

    if(parameters.count() > Param::Poll && !parameters[Param::Poll].isEmpty())
    {
        poll_timeout = parameters[Param::Poll].toInt();
        poll_timeout = poll_timeout < 30 ? 60 : poll_timeout;
    }

    if(parameters.count() > Param::Template && !parameters[Param::Template].isEmpty())
    {
        QString report_template_str = parameters[Param::Template];
        report_template_str.remove('\r');
        report_template_str.remove('\n');
        report_template = report_template_str.split("<br>");
    }

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
    error_message.clear();

    poller = acquire_poller(story, username, password, poll_timeout);
    if(poller.isNull())
        return false;

    // this will filter messages to just those relating to this project
    // and (optionally) builder.  the TeamCity9Poller will invoke our
    // public methods for each event type (started, progress, etc.) since
    // Qt does not provide a canonical way of creating dynamic signals and
    // slots at runtime.

    poller->add_interest(project_name, builder_name, this);

    // do an initial Headline
    QString status = QString("Project \"<b>%1</b>\"").arg(project_name);
    if(!builder_name.isEmpty())
        status += QString(" :: Builder \"<b>%1</b>\"").arg(builder_name);
    status += "<br>State: idle";
    emit signal_new_data(status.toUtf8());

    return true;
}

bool TeamCity9::FinishStory()
{
    error_message.clear();

    poller->remove_interest(project_name, builder_name, this);
    poller.clear();     // we're done with it; don't hang on

    release_poller(story);

    return true;
}

void TeamCity9::Secure(QStringList& parameters) const
{
    if(parameters.count() < Param::Project || parameters[Param::Password].isEmpty())
        return;

    // TODO: This needs to use an improved obfuscation algorithm.
    // Simple hex is too easy to decode.  By the same token, though,
    // being open source, anybody can see HOW it's being encoded,
    // so what's the point?

    parameters[Param::Password] = QString(QByteArray(parameters[Param::Password].toUtf8()).toHex());
}

void TeamCity9::Unsecure(QStringList& parameters) const
{
    if(parameters.count() < Param::Project || parameters[Param::Password].isEmpty())
        return;
    parameters[Param::Password] = QString(QByteArray::fromHex(parameters[Param::Password].toUtf8()));
}

// TeamCity9

void TeamCity9::build_started(const QJsonObject& status)
{
    // a new build in which we may be interested has started

    int build_id = status["id"].toInt();

    uint now = QDateTime::currentDateTime().toTime_t();

    int complete = 0;
    if(status.contains("percentageComplete"))
        complete = status["percentageComplete"].toInt();

    build_status[build_id] = status;

    ETAData eta_data;
    eta_data.start = now;
    eta_data.initial_completed = complete;
    eta_data.last_completed = complete;
    eta_data.last_changed = eta_data.start;

    eta[build_id] = eta_data;

    ReportMap report_map;
    populate_report_map(report_map, status, builder_name);
    QString status_str = render_report(report_map);

    emit signal_new_data(status_str.toUtf8());
}

void TeamCity9::build_progress(const QJsonObject& status)
{
    int build_id = status["id"].toInt();

    uint now = QDateTime::currentDateTime().toTime_t();

    bool send_update = false;

    QJsonObject cached_json = build_status[build_id];
    send_update = (cached_json != status);

    int complete = 0;
    if(status.contains("percentageComplete"))
        complete = status["percentageComplete"].toInt();

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

        QJsonObject running_info = status["running_info"].toObject();
        if(running_info.contains("probablyHanging"))
        {
            bool hanging = running_info["probablyHanging"].toBool();
            if(hanging)
            {
                eta_str = QString("%1 (possibly hung)").arg(eta_str);
                send_update = true;
            }
        }
    }

    if(send_update)
    {
        build_status[build_id] = status;

        ReportMap report_map;
        populate_report_map(report_map, status, builder_name, eta_str);
        QString status_str = render_report(report_map);

        emit signal_new_data(status_str.toUtf8());
    }
}

void TeamCity9::build_final(const QJsonObject& status)
{
    int build_id = status["id"].toInt();

    if(build_status.contains(build_id))
        build_status.remove(build_id);
    if(eta.contains(build_id))
        eta.remove(build_id);

    ReportMap report_map;

    // convert the odd TeamCity timestamp to an ISO 8601
    // format so QDateTime can grok it...
    QString iso_8601 = status["finishDate"].toString();
    iso_8601.insert(18, ':');
    iso_8601.insert(13, ':');
    iso_8601.insert(11, ':');
    iso_8601.insert(6, '-');
    iso_8601.insert(4, '-');
    QDateTime finish_timestamp = QDateTime::fromString(iso_8601, Qt::ISODate).toLocalTime();
    report_map["COMPLETED"] = QString("100% @ %1").arg(finish_timestamp.toString("h:mm ap"));

    populate_report_map(report_map, status, builder_name);
    QString report = render_report(report_map);

    emit signal_new_data(report.toUtf8());
}

void TeamCity9::error(const QString& error_message)
{
    emit signal_new_data(error_message.toUtf8());
}

void TeamCity9::populate_report_map(ReportMap& report_map,
                                   const QJsonObject& build,
                                   const QString& builder_id,
                                   const QString& eta_str)
{
    report_map["PROJECT_NAME"] = project_name;

    report_map["BUILDER_NAME"] = "";
    if(!builder_name.isEmpty())
        report_map["BUILDER_NAME"] = builder_name;
    else if(build.contains("name"))
        report_map["BUILDER_NAME"] = build["name"].toString();
    else if(build.contains("buildType"))
    {
        QJsonObject build_type = build["buildType"].toObject();
        if(build_type.contains("name"))
            report_map["BUILDER_NAME"] = build_type["name"].toString();
    }

    report_map["BUILDER_ID"] = "";
    if(!builder_id.isEmpty())
        report_map["BUILDER_ID"] = builder_id;
    else if(build.contains("buildTypeId"))
        report_map["BUILDER_ID"] = build["buildTypeId"].toString();

    report_map["BUILD_ID"] = QString::number(build["id"].toInt());
    report_map["BUILD_NUMBER"] = build["number"].toString();
    report_map["STATE"] = capitalize(build["state"].toString());
    if(build.contains("statusText"))
        report_map["STATUS"] = build["statusText"].toString();
    else
        report_map["STATUS"] = capitalize(build["status"].toString());
    if(!report_map.contains("COMPLETED"))
        report_map["COMPLETED"] = QString("%1%").arg(QString::number(build["percentageComplete"].toInt()));
    if(eta_str.isEmpty() && !report_map["STATE"].compare("running"))
        report_map["ETA"] = "(pending)";
    else
        report_map["ETA"] = eta_str;

    report_map["AGENT"] = "";
    if(build.contains("agent"))
    {
        QJsonObject agent = build["agent"].toObject();
        report_map["AGENT"] = agent["name"].toString();
    }

    if(build.contains("properties"))
    {
        QJsonObject properties = build["properties"].toObject();
        int count = properties["count"].toInt();
        if(count)
        {
            QJsonArray properties_array = properties["property"].toArray();
            for(int i = 0;i < count;++i)
            {
                QJsonObject property = properties_array.at(i).toObject();
                QString property_id = QString("PROPERTY_%1").arg(property["name"].toString().toUpper());
                report_map[property_id] = property["value"].toString();
            }
        }
    }
}

QString TeamCity9::render_report(const ReportMap& report_map)
{
    QString report = report_template.join("<br>");
    foreach(const QString& key, report_map.keys())
    {
        QString token = QString("${%1}").arg(key);
        report.replace(token, report_map[key]);
    }

    return report;
}

QString TeamCity9::capitalize(const QString& str)
{
    QString tmp = str.toLower();
    tmp[0] = str[0].toUpper();
    return tmp;
}

PollerPointer TeamCity9::acquire_poller(const QUrl& target, const QString& username, const QString& password, int timeout)
{
    if(!poller_map.contains(target))
    {
        PollerData pd;
        pd.poller = PollerPointer(new TeamCity9Poller(target, username, password, timeout));
        poller_map[target] = pd;
    }

    PollerData& pd = poller_map[target];
    ++pd.reference_count;
    return pd.poller;
}

void TeamCity9::release_poller(const QUrl& target)
{
    if(!poller_map.contains(target))
        return;

    PollerData& pd = poller_map[target];
    if(--pd.reference_count == 0)
        poller_map.remove(target);
}
