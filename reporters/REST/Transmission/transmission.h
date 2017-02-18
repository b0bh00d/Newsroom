#pragma once

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include <ireporter.h>

#include "../../../specialize.h"

#include "transmissionglobal.h"
#include "transmissionpoller.h"

/// @class Transmission
/// @brief A Reporter for display torrent information for a Transmission Client
///
/// Transmission is a Reporter plug-in for Newsroom that displays information
/// about a torrent occupying a specific slot in a Tranmission Client instance.
///
/// Because Transmission does not provide an actual REST API (as of this
/// writing), a small helper function written in Python is also included with
/// this Reporter for feeding the results of 'transmission-remote --list' to
/// anybody asking via HTTP.

class TRANSMISSIONSHARED_EXPORT Transmission : public IReporter2
{
    Q_OBJECT
public:
    Transmission(QObject* parent = nullptr);

    // these methods are directly invoked by TransmissionPoller since we
    // cannot create dynamic run-time signals and slots in a canonical
    // fashion
    void    status(const QJsonObject& status, float maxratio = 0.0f);
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

    // IReporter2
    bool UseReporterDraw() const Q_DECL_OVERRIDE;
    void ReporterDraw(const QRect& bounds, QPainter& painter) Q_DECL_OVERRIDE;

private:    // typedefs and enums
    typedef enum
    {
        Slot,
        Poll,
        Template,
        Count,
    } Param;

    SPECIALIZE_MAP(QString, QString, Report)            // "ReportMap"

private:    // methods
    void            populate_report_map(ReportMap& report_map, const QJsonObject& status);
    QString         render_report(const ReportMap& report_map);
    QString         capitalize(const QString& str);

private:    // data members
    bool        owner_draw;
    int         my_slot;
    float       max_ratio;
    int         poll_timeout;

    QJsonObject latest_status;

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
    static  PollerPointer   acquire_poller(const QUrl& target, int timeout);
    static  void            release_poller(const QUrl& target);
};
