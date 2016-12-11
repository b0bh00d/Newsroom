#include "teamcity.h"

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
    state = States::None;

    // get project listing first
    QString projects_url = QString("%1/httpAuth/app/rest/projects").arg(story.toString());
    QUrl url(projects_url);
    url.setUserName(username);
    url.setPassword(password);

    state = States::GettingProjects;
    reply_buffer.clear();

    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Content-type"), QByteArray("application/json"));
    request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));

    QNAM = new QNetworkAccessManager(this);

    QNetworkReply* reply = QNAM->get(request);
    bool connected = connect(reply, &QNetworkReply::readyRead, this, &TeamCity::slot_get_read);
    ASSERT_UNUSED(connected);
    connected = connect(reply, &QNetworkReply::finished, this, &TeamCity::slot_get_complete);
    ASSERT_UNUSED(connected);
    connected = connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                        this, &TeamCity::slot_get_failed);
    ASSERT_UNUSED(connected);

    return true;
}

bool TeamCity::FinishStory()
{
    error_message = QStringLiteral("TeamCity: Not implemented.");
    return false;
}

// IPluginREST
QStringList TeamCity::Requires() const
{
    return QStringList() << "Username"      << "string" <<
                            "Password"      << "password" <<
                            "Project Name"  << "string" <<
                            "Builder"       << "string";
}

bool TeamCity::SetRequirements(const QStringList& parameters)
{
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

void TeamCity::process_reply()
{
    if(state == States::GettingProjects)
    {
        json_projects.clear();

        QJsonDocument d = QJsonDocument::fromJson(reply_buffer);
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
            json_projects[name] = project;
        }

        // get the builders for the project we are watching

        if(!json_projects.contains(project_name))
            error_message = QString("TeamCity: The project id \"%1\" was not found on server \"%2\".").arg(project_name).arg(story.toString());
        else
        {
            QJsonObject project = json_projects[project_name];

            QString builders_url = QString("%1%2").arg(story.toString()).arg(project["href"].toString());

            QUrl url(builders_url);
            url.setUserName(username);
            url.setPassword(password);

            state = States::GettingBuilders;
            reply_buffer.clear();

            QNetworkRequest request(url);
            request.setRawHeader(QByteArray("Content-type"), QByteArray("application/json"));
            request.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));

            QNAM = new QNetworkAccessManager(this);

            QNetworkReply* reply = QNAM->get(request);
            bool connected = connect(reply, &QNetworkReply::readyRead, this, &TeamCity::slot_get_read);
            ASSERT_UNUSED(connected);
            connected = connect(reply, &QNetworkReply::finished, this, &TeamCity::slot_get_complete);
            ASSERT_UNUSED(connected);
            connected = connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                                this, &TeamCity::slot_get_failed);
            ASSERT_UNUSED(connected);
        }
    }
    else if(state == States::GettingBuilders)
    {
        json_builders.clear();

        QJsonDocument d = QJsonDocument::fromJson(reply_buffer);
        QJsonObject sett2 = d.object();
        QJsonObject buildtypes = sett2["buildTypes"].toObject();
        int count = buildtypes["count"].toInt();
        QJsonArray builder_array = buildtypes["buildType"].toArray();
        for(int x = 0;x < count;++x)
        {
            QJsonObject builder = builder_array.at(x).toObject();
            QString name = builder["name"].toString();
            json_builders[name] = builder;
        }

        if(!builder_name.isEmpty())
        {
            if(!json_builders.contains(builder_name))
                error_message = QString("TeamCity: The build id \"%1\" was not found for project \"%2\" on server \"%3\".")
                        .arg(builder_name).arg(project_name).arg(story.toString());
        }
    }
    else if(state == States::GettingDetails)
    {
//        QJsonDocument d = QJsonDocument::fromJson(aaptr->reply_buffer);
//        process_detail(d, aaptr);

//        if(--history_index < retrieve_start)
//        {
//            QJsonObject builder = json_builders[ui->combo_Builders->currentIndex()];
//            ui->statusBar->showMessage(QString("%1 history entries retrieved for builder '%2'").arg(retrieve_end - retrieve_start + 1).arg(builder["name"].toString()), 10000);

//            for(int x = COL_NUMBER;x < COL_LAST;++x)
//                ui->tree_BuildHistory->resizeColumnToContents(x);

//            set_control_states();
//        }
//        else
//            triggers.enqueue(CallbackPair(&MainWindow::get_history_entry, aaptr));
    }
}

void TeamCity::slot_get_read()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
        reply_buffer += reply->readAll();
}

void TeamCity::slot_get_complete()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if(reply && reply->error() == QNetworkReply::NoError)
    {
        // should have valid data in 'reply_buffer'
        reply_buffer += reply->readAll();
        process_reply();
    }

    if(reply)
    {
        reply->deleteLater();
        reply = nullptr;
    }

    QNAM->deleteLater();
    QNAM = nullptr;
}

void TeamCity::slot_get_failed(QNetworkReply::NetworkError /*code*/)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if(reply)
    {
        reply->deleteLater();
        reply = nullptr;
    }

    reply_buffer.clear();

    QNAM->deleteLater();
    QNAM = nullptr;

//    if(code == QNetworkReply::OperationCanceledError)
//        QMessageBox::information(this, tr("Operation Aborted"),
//                                       tr("The network operation was canceled."));
//    else if(code == QNetworkReply::AuthenticationRequiredError)
//    {
//        QMessageBox::critical(this, tr("Authentication Failure Detected"),
//                                    tr("Did you enter your user name and password correctly?"));
//        tc_password.clear();
//    }
//    else
//        QMessageBox::critical(this, tr("Network Failure Detected"),
//                                    tr("A network error code %1 was returned for the last operation.").arg(code));
}
