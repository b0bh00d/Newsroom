#pragma once

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include <ireporter.h>

#include "../../../specialize.h"

#include "teamcity9_global.h"
#include "teamcity9poller.h"

/// @class TeamCity9
/// @brief A Reporter for Newsroom that covers Team City builds
///
/// TeamCity9 is a Reporter plug-in for Newsroom that knows how to read
/// and report on one or more builds using Team City's REST API.
///
/// This particular Reporter instance converses with Team City using its
/// v9 REST API calls.

class TEAMCITY9SHARED_EXPORT TeamCity9 : public IReporter
{
    Q_OBJECT
public:
    TeamCity9(QObject* parent = nullptr);

    // these methods are directly invoked by TeamCity9Poller since we cannot
    // create dynamic run-time signals and slots in a canonical fashion
    void    build_pending(const QJsonObject& status);
    void    build_started(const QJsonObject& status);
    void    build_progress(const QJsonObject& status);
    void    build_final(const QJsonObject& status);
    void    error(const QString& message);

    // IReporter
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }
    QStringList DisplayName() const Q_DECL_OVERRIDE;
    QString PluginClass() const Q_DECL_OVERRIDE { return "REST"; }
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    float Supports(const QUrl& entity) const Q_DECL_OVERRIDE;
    int RequiresVersion() const;
    RequirementsFormats RequiresFormat() const;
    bool RequiresUpgrade(int, QStringList&);
    QStringList Requires(int = 0) const Q_DECL_OVERRIDE;
    bool SetRequirements(const QStringList& parameters) Q_DECL_OVERRIDE;
    void SetStory(const QUrl& url) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;
    void Secure(QStringList& parameters) const Q_DECL_OVERRIDE;
    void Unsecure(QStringList& parameters) const Q_DECL_OVERRIDE;

private:    // classes
    struct ETAData
    {
        int         initial_completed;
        uint        start;

        // these values will be used to try and detect a hung build
        int         last_completed;
        uint        last_changed;
    };

private:    // typedefs and enums
    typedef enum
    {
        Username,
        Password,
        Project,
        Builder,
        Changes,
        Poll,
        Template,
        Count,
    } Param;

    SPECIALIZE_MAP(int, ETAData, ETA)                   // "ETAMap"
    SPECIALIZE_MAP(QString, QString, Report)            // "ReportMap"
    SPECIALIZE_MAP(int, QJsonObject, Status)            // "StatusMap"

private:    // methods
    void            populate_report_map(ReportMap& report_map,
                                        const QJsonObject& build,
                                        const QString& builder_id = QString(),
                                        const QString& eta_str = QString());
    QString         render_report(const ReportMap& report_map);
    QString         capitalize(const QString& str);

private:    // data members
    QString     username;
    QString     password;
    QString     project_name;
    QString     builder_name;

    int         poll_timeout;
    int         last_changes_count;

    bool        check_for_changes;

    StatusMap   build_status;
    ETAMap      eta;

    QStringList report_template;

    PollerPointer poller;

private:    // class-static data
    struct PollerData
    {
        int             reference_count;
        PollerPointer   poller;

        PollerData() : reference_count(0) {}
    };

    SPECIALIZE_MAP(QUrl, PollerData, Poller)            // "PollerMap"

    static  PollerMap       poller_map;
    static  PollerPointer   acquire_poller(const QUrl& target, const QString &username, const QString &password, int timeout);
    static  void            release_poller(const QUrl& target);
};
