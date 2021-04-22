#pragma once

#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <ireporter.h>

#include "../../../specialize.h"

#include "chartapi_global.h"
#include "chartapipoller.h"

/// @class YahooChartAPI
/// @brief A Reporter for Newsroom that understands the Yahoo Chart API
///
/// YahooChartAPI is a Reporter plug-in for retrieving chart data from
/// the Yahoo Chart API and displaying it in a semi-accurate fashion.
/// It defaults to the "^dji" symbol, which represents the Dow Jones
/// Industrial Average, but the user can enter any symbol (e.g., "msft"
/// for Microsoft) and get information about that stock.
///
/// I don't currently know much about the differences and use of the
/// various values provided (open, close, low, high), so I'm winging
/// an average that seems to be pretty accurate, and is at least
/// informational.

class YAHOOCHARTAPISHARED_EXPORT YahooChartAPI : public IReporter2
{
    Q_OBJECT
public:
    YahooChartAPI(QObject* parent = nullptr);

    // these methods are directly invoked by YahooChartAPIPoller since we cannot
    // create dynamic run-time signals and slots in a canonical fashion
    void    ticker_update(const QString& status);
    void    ticker_update(const QDomDocument& status);
    void    ticker_update(const QJsonObject& status);
    void    error(const QString& message);

    // IReporter
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }
    QStringList DisplayName() const Q_DECL_OVERRIDE;
    QString PluginClass() const Q_DECL_OVERRIDE { return "REST"; }
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    float Supports(const QUrl& entity) const Q_DECL_OVERRIDE;
    int RequiresVersion() const override;
    RequirementsFormats RequiresFormat() const override;
    bool RequiresUpgrade(int version, QStringList&parameters) override;
    QStringList Requires(int target_version = 0) const Q_DECL_OVERRIDE;
    bool SetRequirements(const QStringList& parameters) Q_DECL_OVERRIDE;
    void SetStory(const QUrl& url) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;
    void Secure(QStringList&) const Q_DECL_OVERRIDE;
    void Unsecure(QStringList&) const Q_DECL_OVERRIDE;

    // IReporter2
    bool UseReporterDraw() const Q_DECL_OVERRIDE;
    void ReporterDraw(const QRect& bounds, QPainter& painter) Q_DECL_OVERRIDE;

private:    // classes

private:    // typedefs and enums
    enum
    {
        Mon = 1,    // values returned by QDate::dayOfWeek()
        Tue,
        Wed,
        Thu,
        Fri,
        Sat,
        Sun
    };

    enum
    {
        Jan = 1,    // values returned by QDate::month()
        Feb,
        Mar,
        Apr,
        May,
        Jun,
        Jul,
        Aug,
        Sep,
        Oct,
        Nov,
        Dec
    };

    typedef enum
    {
        Ticker,
        Alias,
        Poll,
        Graph,
        MaxRange,   // v2
        Indicators, // v2
        Template,
        Max,
    } Param;

    SPECIALIZE_PAIR(int, float, Tick)               // "TickPair"
    SPECIALIZE_VECTOR(TickPair, Tick)               // "TickVector"

    struct ChartData
    {
        float current_high_low, current_high_high{0.0f};
        float previous_high_low, previous_high_high{0.0f};
        float current_low_low, current_low_high{0.0f};
        float previous_low_low, previous_low_high{0.0f};
        float current_high, current_low{0.0f};
        float open_average{0.0f};
        float current_average{0.0f};
        float previous_close{0.0f};
        int open_timestamp{0};
        int close_timestamp{0};
        QPoint opening_point;
        QPoint previous_close_point;

        bool market_closed{false};
        QDateTime next_open;

        QString volume_max_str;
        QString volume_min_str;

        TickVector history;
    };

    SPECIALIZE_SHAREDPTR(ChartData, ChartData)          // "ChartDataPointer"
    SPECIALIZE_MAP(QString, QString, Report)            // "ReportMap"

    SPECIALIZE_PAIR(float, int, Volume)                 // "VolumePair"
    SPECIALIZE_VECTOR(VolumePair, Volume)               // "VolumeVector"

private slots:
    void            slot_headline_sleep();

private:    // methods
    bool            is_market_holiday(const QDateTime &now) const;
    QString         format_duration(int seconds) const;
    void            populate_report_map(ReportMap& report_map, ChartDataPointer chart_data);
    QString         render_report(const ReportMap& report_map, const QStringList& report_template);
    QString         capitalize(const QString& str);
    QDateTime       calculate_next_open(const QDateTime &open_datetime, const QDateTime &close_datetime, bool &closed_today);

private:    // data members
    QString     ticker;
    QString     ticker_alias;

    int         poll_timeout{60};
    int         last_timestamp{0};
    QByteArray  last_hash;

    bool        display_graph{true};

    QStringList report_template;

    PollerPointer poller;

    ChartDataPointer    chart_data;

    bool        lock_to_max_range{true};
    float       volume_min{std::numeric_limits<float>::max()};
    float       volume_max{0.0f};

    bool        ensure_indicators_are_visible{false};

private:    // class-static data
    struct PollerData
    {
        int             reference_count{0};
        PollerPointer   poller;
    };

    SPECIALIZE_MAP(QString, PollerData, Poller)         // "PollerMap"

    static  PollerMap       poller_map;
    static  PollerPointer   acquire_poller(const QUrl &story, const QString &ticker, int timeout);
    static  void            release_poller(const QString& ticker);
};
