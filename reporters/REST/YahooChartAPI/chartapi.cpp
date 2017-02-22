#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "chartapi.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

// 'lock-to-max-range', 'ensure-indicators-are-visible'
const int ParametersVersion = 2;

YahooChartAPI::PollerMap YahooChartAPI::poller_map;

YahooChartAPI::YahooChartAPI(QObject *parent)
    : poll_timeout(60),
      last_timestamp(0),
      display_graph(true),
      lock_to_max_range(true),
      volume_min(std::numeric_limits<float>::max()),
      volume_max(0.0f),
      ensure_indicators_are_visible(false),
      IReporter2(parent)
{
    report_template << "Ticker: <b>${TICKER}</b> (${ALIAS})";
    report_template << "Time: ${TIMESTAMP}";
    report_template << "Average: ${AVERAGE}";
    report_template << "Previous Close: ${PREVIOUS_CLOSE}";
    report_template << "Closing Offsets: ${PREVIOUS_CLOSE_OFFSET_AMOUNT} (${PREVIOUS_CLOSE_OFFSET_PERCENTAGE}%)";
}

// IReporter

QStringList YahooChartAPI::DisplayName() const
{
    return QStringList() << QObject::tr("Yahoo Chart API") <<
                            QObject::tr("Uses the Yahoo Chart API to return NYSE information about a specific ticker symbol");
}

QByteArray YahooChartAPI::PluginID() const
{
    return "{535B9CF3-49E3-48A5-B7F3-053FB904A91A}";
}

float YahooChartAPI::Supports(const QUrl& entity) const
{
    // QUrl::isLocalFile() will return true if the URL ends with
    // an HTML file reference (making it kind of useless by itself,
    // actually).

    if(entity.isLocalFile() && QFile::exists(entity.toLocalFile()))
        return 0.0f;

    // ensure they aren't handing us an incorrect path...

    QString entity_str = entity.toString().toLower();
    if(entity_str.endsWith(".htm") || entity_str.endsWith(".html"))
        return 0.0f;

    if(!entity_str.contains("chartapi.finance.yahoo.com"))
        return 0.0f;

    return 1.0f;
}

int YahooChartAPI::RequiresVersion() const
{
    return ParametersVersion;
}

RequirementsFormats YahooChartAPI::RequiresFormat() const
{
    return RequirementsFormats::Simple;
}

bool YahooChartAPI::RequiresUpgrade(int version, QStringList& parameters)
{
    error_message.clear();

    if(version == 0)
        version = ParametersVersion;

    bool upgraded = false;
    while(version < ParametersVersion)
    {
        ++version;

        if(version == 2)
        {
            // version 2 added
            // - "Lock graph to largest volume range"
            // - "Ensure indicators are always visible"

            parameters.insert(4, "");   // use the default
            parameters.insert(4, "");   // use the default

            upgraded = true;
        }
    }

    return upgraded;
}

QStringList YahooChartAPI::Requires(int target_version) const
{
    QStringList definitions;

    if(target_version == 0)
        target_version = ParametersVersion;

    // "simple" requirements format

    // parameter names ending with an asterisk are required

    // version 1 (base)
    definitions << "Symbol:"        << "string:^dji"
                << "Alias:"         << "string"

                // how many seconds between polls? (default: 60)
                << "Polling (sec):" << QString("integer:%1").arg(poll_timeout)

                // should we display the graph instead of just text?
                << "Display graph instead of just text" << "check:true"

                << "Format:"        << QString("multiline:%1").arg(report_template.join("<br>\n"));

    int version = 1;
    while(version < target_version)
    {
        ++version;

        if(version == 2)
        {
            // version 2 added
            // - "Lock graph to largest volume range"
            // - "Ensure indicators are always visible"

            // insert updates in reverse order!

                                  // Scale volume to include indicators?
            definitions.insert(8, QString("check:%1").arg(ensure_indicators_are_visible ? "true" : "false"));
            definitions.insert(8, "Ensure indicators are always visible");
                                  // Retain the largest range seen for the graph volume?
            definitions.insert(8, QString("check:%1").arg(lock_to_max_range ? "true" : "false"));
            definitions.insert(8, "Lock graph to largest volume range");
        }
    }

    return definitions;
}

bool YahooChartAPI::SetRequirements(const QStringList& parameters)
{
    error_message.clear();

    // default ticker symbol
    ticker = "^dji";
    ticker_alias = "Dow Jones Industrial Average";
    last_timestamp = 0;

    if(!parameters.count())
    {
        error_message = QStringLiteral("YahooChartAPI: Not enough parameters provided.");
        return false;
    }

    if(parameters.count() && !parameters[Param::Ticker].isEmpty())
        ticker = parameters[Param::Ticker];

    if(parameters.count() > Param::Alias && !parameters[Param::Alias].isEmpty())
        ticker_alias = parameters[Param::Alias];

    if(parameters.count() > Param::Poll && !parameters[Param::Poll].isEmpty())
    {
        poll_timeout = parameters[Param::Poll].toInt();
        // the chart API only updates every 60 seconds, so anything more
        // frequent is a waste of bandwidth
        poll_timeout = poll_timeout < 60 ? 60 : poll_timeout;
    }

    if(parameters.count() > Param::Graph && !parameters[Param::Template].isEmpty())
        display_graph = !parameters[Param::Graph].toLower().compare("true");

    if(parameters.count() > Param::MaxRange && !parameters[Param::MaxRange].isEmpty())
        lock_to_max_range = !parameters[Param::MaxRange].toLower().compare("true");

    if(parameters.count() > Param::Indicators && !parameters[Param::Indicators].isEmpty())
        ensure_indicators_are_visible = !parameters[Param::Indicators].toLower().compare("true");

    if(parameters.count() > Param::Template && !parameters[Param::Template].isEmpty())
    {
        QString report_template_str = parameters[Param::Template];
        report_template_str.remove('\r');
        report_template_str.remove('\n');
        report_template = report_template_str.split("<br>");
    }

    return true;
}

void YahooChartAPI::SetStory(const QUrl& url)
{
    story = url;
}

bool YahooChartAPI::CoverStory()
{
    error_message.clear();

    if(!chart_data.isNull())
        chart_data.clear();
    chart_data = ChartDataPointer(new ChartData());

    poller = acquire_poller(story, ticker, poll_timeout);
    if(poller.isNull())
        return false;

    // this call to add_interest() will filter messages to just those
    // relating to this ticker.  the YahooChartAPIPoller will invoke
    // our public methods for each event type (started, progress, etc.)
    // since Qt does not provide a canonical way of creating dynamic
    // signals and slots at runtime--we can't construct signals/slots
    // for specific project/builder combinations.

    poller->add_interest(ticker, this);

    return true;
}

bool YahooChartAPI::FinishStory()
{
    error_message.clear();

    if(!poller.isNull())
    {
        poller->remove_interest(ticker, this);
        poller.clear();     // we're done with it; don't hang on

        release_poller(ticker);
    }

    return true;
}

void YahooChartAPI::Secure(QStringList& /*parameters*/) const
{
}

void YahooChartAPI::Unsecure(QStringList& /*parameters*/) const
{
}

// IReporter2

bool YahooChartAPI::UseReporterDraw() const
{
    return display_graph;
}

const int LatestPointMarkerWidth = 3;
const int LatestPointMarkerHeight = 3;
const int LightSkyBlueRed = 135;
const int LightSkyBlueGreen = 206;
const int LightSkyBlueBlue = 250;

void YahooChartAPI::ReporterDraw(const QRect& bounds, QPainter& painter)
{
    // use some shorthand for the current volume range

    float volume_low = chart_data->current_low_low;
    float volume_high = chart_data->current_high_high;

    if(lock_to_max_range)
    {
        bool update = false;

        if(volume_low < volume_min)
        {
            volume_min = volume_low;
            update = true;
        }
        if(volume_high > volume_max)
        {
            volume_max = volume_high;
            update = true;
        }

        // keep the indicators accurate on the chart
        if(update)
        {
            chart_data->opening_point.setX(0);
            chart_data->opening_point.setY(0);
            chart_data->previous_close_point.setX(0);
            chart_data->previous_close_point.setY(0);
        }
    }
    else
    {
        volume_min = volume_low;
        volume_max = volume_high;
    }

    if(ensure_indicators_are_visible)
    {
        if(chart_data->previous_close < volume_min)
            volume_min = chart_data->previous_close - 10;
        else if(chart_data->previous_close > volume_max)
            volume_max = chart_data->previous_close + 10;

        if(chart_data->history.length())
        {
            const TickPair& p = chart_data->history.front();
            if(p.second < volume_min)
                volume_min = p.second - 10;
            else if(p.second > volume_max)
                volume_max = p.second + 10;
        }
    }

    ReportMap report_map;
    populate_report_map(report_map, chart_data);

    QTextDocument td;
    td.setDocumentMargin(0);

    QString current_average_str(report_map["AVERAGE"]);
    if(chart_data->market_closed)
        current_average_str = QString("<i>%1</i>").arg(report_map["AVERAGE"]);
    QString font_color = QString("%1%2%3")
                            .arg(QString::number(LightSkyBlueRed, 16))
                            .arg(QString::number(LightSkyBlueGreen, 16))
                            .arg(QString::number(LightSkyBlueBlue, 16)).toUpper();

    td.setHtml(QString("<b>%1</b><br>&#8657; %2 %3<br>&#8659; %4<br><b>%5</b><br>%6 (%7%) %8<br><font color=\"#%9\">%10 (%11%)</font>")
                            .arg(report_map["TICKER"])
                            .arg(chart_data->volume_max_str)
                            .arg(report_map["RANGE_LOCKED"])
                            .arg(chart_data->volume_min_str)
                            .arg(current_average_str)
                            .arg(report_map["PREVIOUS_CLOSE_OFFSET_AMOUNT"])
                            .arg(report_map["PREVIOUS_CLOSE_OFFSET_PERCENTAGE"])
                            .arg(report_map["INDICATORS_VISIBLE"])
                            .arg(font_color)
                            .arg(report_map["OPEN_OFFSET_AMOUNT"])
                            .arg(report_map["OPEN_OFFSET_PERCENTAGE"]));

    QSizeF doc_size = td.documentLayout()->documentSize();
    QRect new_bounds = QRect(bounds.left(), bounds.top(), bounds.width() - doc_size.width() - 5, bounds.height());
    QRect graph_bounds = new_bounds.adjusted(1, 0, -(LatestPointMarkerWidth * 2 + 1), 0);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    painter.save();

    painter.translate(bounds.right() - doc_size.width(), 5);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, painter.pen().color());
    td.documentLayout()->draw(&painter, ctx);

    painter.restore();

    // draw some chart background stuff first, if necessary

    if(chart_data->market_closed)
    {
#if 0
        // this code selects a UNICODE clock symbol of the right open
        // or close time for the locale

        QDateTime now = QDateTime::currentDateTime();
        QTime my_time = now.time();

        QDateTime open_datetime = QDateTime::fromTime_t(chart_data->open_timestamp);
        QTime open_time = open_datetime.time();
        QDateTime close_datetime = QDateTime::fromTime_t(chart_data->close_timestamp);
        QTime close_time = close_datetime.time();

        int clock_code = 128336;        // "one pm"
        if(my_time < open_time)
        {
            if(open_time.minute() == 30)
                clock_code = 128348;    // "one-thirty"
            clock_code += open_time.hour() - 1; // clock starts @ 1
        }
        else if(my_time > close_time)
        {
            if(close_time.minute() == 30)
                clock_code = 128348;    // "one-thirty"
            if(close_time.hour() >= 13 && close_time.hour() <= 23)
                clock_code += close_time.hour() - 13;
        }
#endif

        QTextDocument td2;
        td2.setDocumentMargin(0);

        td2.setPageSize(QSizeF(new_bounds.width() + 10, new_bounds.height() + 10));
        td2.setHtml(tr("<center><font size=\"+2\"><b><i>MARKET&nbsp;CLOSED</i></b></font><br>re-opens %1</center>")
                    .arg(chart_data->next_open.toString("ddd MMM d hh:mm:ss")));

        td2.adjustSize();

        doc_size = td2.documentLayout()->documentSize();

        painter.save();

        painter.translate((new_bounds.width() - doc_size.width()) / 2, new_bounds.bottom() - doc_size.height());

        QAbstractTextDocumentLayout::PaintContext ctx2;
        ctx2.palette.setColor(QPalette::Text, painter.pen().color().darker(250));
        td2.documentLayout()->draw(&painter, ctx2);

        painter.restore();
    }
    else if(ticker_alias.length())
    {
        // put the alias in the background of the chart

        QTextDocument td2;
        td2.setDocumentMargin(0);

        td2.setPageSize(QSizeF(new_bounds.width(), new_bounds.height()));
        td2.setHtml(tr("<center><font size=\"+2\"><b>%1</b></font></center>").arg(ticker_alias));

        td2.adjustSize();

        doc_size = td2.documentLayout()->documentSize();

        painter.save();

        painter.translate((new_bounds.width() - doc_size.width()) / 2, new_bounds.bottom() - doc_size.height());

        QAbstractTextDocumentLayout::PaintContext ctx2;
        ctx2.palette.setColor(QPalette::Text, painter.pen().color().darker(300));
        td2.documentLayout()->draw(&painter, ctx2);

        painter.restore();
    }

    painter.restore();      // Antialiasing

    // draw the simple chart on the Headline
    painter.save();

    painter.drawLine(new_bounds.left(), new_bounds.top(), new_bounds.left(), new_bounds.bottom());
    painter.drawLine(new_bounds.left(), new_bounds.bottom(), new_bounds.right(), new_bounds.bottom());
    painter.drawLine(new_bounds.right(), new_bounds.bottom(), new_bounds.right(), new_bounds.top());

//    painter.setClipRect(new_bounds);

    if(volume_min != volume_max)
    {
        VolumeVector volume_ticks;

        // bottom is volume_min; top is volume_max
        int chart_height = new_bounds.height();
        float chart_range = volume_max - volume_min;
        if(chart_range < chart_height)
        {
            // scale range up into height
            int pixel_increment = round((chart_range / chart_height) + 0.5);
            float volume_increment = chart_range / chart_height;
            float i = volume_min;
            int p = 0;
            while(i < volume_max)
            {
                volume_ticks.append(qMakePair(i, p));
                i+=volume_increment;
                p+=pixel_increment;
            }
            volume_ticks.append(qMakePair(volume_max, p));
        }
        else
        {
            // scale range down into height
            int pixel_increment = 1;
            float volume_increment = chart_range / chart_height;
            float i = volume_min;
            int p = 0;
            while(i < volume_max)
            {
                volume_ticks.append(qMakePair(i, p));
                i+=volume_increment;
                p+=pixel_increment;
            }
            volume_ticks.append(qMakePair(volume_max, p));
        }

        // left is open_timestamp; right is close_timestamp
        int open_timestamp = chart_data->open_timestamp;
        int close_timestamp = chart_data->close_timestamp;

        // get the time range of the available data
        int time_low = std::numeric_limits<int>::max();
        int time_high = 0;

        foreach(const TickPair& p, chart_data->history)
        {
            if(p.first < time_low)
                time_low = p.first;
            if(p.first > time_high)
                time_high = p.first;
        }

        int width_increment = 1;
        int chart_width = graph_bounds.width();
        int time_range = (close_timestamp - open_timestamp) / 60;
        int data_time_range = (time_high - time_low) / 60;

        if(chart_width < time_range)
        {
            if(data_time_range > chart_width)
                // we use a moving window of time
                open_timestamp += (data_time_range - chart_width) * 60;
        }
        else
        {
            // we have to scale the time range to fit the space
            width_increment = round((chart_width / time_range * 1.0f) + 0.5);
            // how many time intervals can we display within this width with
            // this increment?
            int interval = chart_width / width_increment;
            open_timestamp += (time_range - interval) * 60;
        }

        int time_offset = 0;
        int volume_offset = 0;
        QPoint last_point;

        chart_data->opening_point.setX(0);
        chart_data->opening_point.setY(0);

        // calculate the previous close on the graph

        if(chart_data->previous_close_point.isNull())
        {
            volume_offset = 0;
            int last_volume_offset = 0;
            foreach(const VolumePair& tick, volume_ticks)
            {
                if(!last_volume_offset)
                    tick.second;

                if(tick.first > chart_data->previous_close)
                {
                    volume_offset = last_volume_offset;
                    break;
                }

                last_volume_offset = tick.second;
            }

            if(!volume_offset)
            {
                const VolumePair& tick = volume_ticks.back();
                if(tick.first == chart_data->previous_close)
                    volume_offset = tick.second;
            }

            if(volume_offset)
            {
                chart_data->previous_close_point.setX(new_bounds.left());
                chart_data->previous_close_point.setY(new_bounds.bottom() - volume_offset);
            }
        }

        foreach(const TickPair& p, chart_data->history)
        {
            time_offset = ((p.first - open_timestamp) / 60) * width_increment;
            if(time_offset < 0)
                time_offset = 0;

            volume_offset = 0;
            int last_volume_offset = 0;
            foreach(const VolumePair& tick, volume_ticks)
            {
                if(!last_volume_offset)
                    last_volume_offset = tick.second;

                if(tick.first > p.second)
                {
                    volume_offset = last_volume_offset;
                    break;
                }

                last_volume_offset = tick.second;
            }

            if(!volume_offset)
            {
                const VolumePair& tick = volume_ticks.back();
                if(tick.first == p.second)
                    volume_offset = tick.second;
            }

            Q_ASSERT(volume_offset != 0);

            if(chart_data->opening_point.isNull())
            {
                chart_data->opening_point.setX(new_bounds.left() + time_offset);
                chart_data->opening_point.setY(new_bounds.bottom() - volume_offset);

                // highlight the opening volume point on the graph
                painter.save();
                painter.setPen(QColor(LightSkyBlueRed, LightSkyBlueGreen, LightSkyBlueBlue));
                painter.drawLine(QPoint(new_bounds.left()+1, chart_data->opening_point.y()), QPoint(new_bounds.right()-1, chart_data->opening_point.y()));
                painter.restore();

                if(!chart_data->previous_close_point.isNull())
                {
                    painter.save();
                    painter.setPen(QPen(painter.pen().color().darker(), 1, Qt::DashLine));
                    painter.drawLine(QPoint(new_bounds.left()+1, chart_data->previous_close_point.y()), QPoint(new_bounds.right()-1, chart_data->previous_close_point.y()));
                    painter.restore();
                }
            }

            if(p.first < open_timestamp)
                continue;

            if(last_point.isNull())
                painter.drawPoint(chart_data->opening_point);
            else
                painter.drawLine(last_point, QPoint(graph_bounds.left() + time_offset, graph_bounds.bottom() - volume_offset));

            last_point.setX(graph_bounds.left() + time_offset);
            last_point.setY(graph_bounds.bottom() - volume_offset);
        }

        // draw a line to the opening indicator, if it is off the screen
        if(chart_data->history.length())
        {
            painter.save();
            painter.setPen(QColor(LightSkyBlueRed, LightSkyBlueGreen, LightSkyBlueBlue));

            const TickPair& p = chart_data->history.front();
            if(p.second < volume_min)
                  painter.drawLine(last_point,QPoint(last_point.x(), graph_bounds.bottom() - 1));
            else if(p.second > volume_max)
                  painter.drawLine(last_point,QPoint(last_point.x(), graph_bounds.top() + 1));
            painter.restore();
        }

        // draw a line to the previous-close indicator, if it is off the screen
        if(chart_data->previous_close_point.isNull())
        {
            painter.save();
            painter.setPen(QPen(painter.pen().color().darker(), 1, Qt::DashLine));

            if(chart_data->previous_close < volume_min)
                painter.drawLine(last_point,QPoint(last_point.x(), graph_bounds.bottom() - 1));
            else if(chart_data->previous_close > volume_max)
                painter.drawLine(last_point,QPoint(last_point.x(), graph_bounds.top() + 1));

            painter.restore();
        }

        // highlight the current volume point
        painter.save();
          painter.setBrush(painter.pen().color());
          painter.setPen(QColor(0, 0, 255));
          painter.drawEllipse(last_point, LatestPointMarkerWidth, LatestPointMarkerHeight);
        painter.restore();
    }

    painter.restore();

    painter.restore();  // Antialiasing
}

// YahooChartAPI

QString YahooChartAPI::format_duration(int seconds)
{
    QString duration;
    int hours = seconds / 60 / 60;
    seconds -= hours * 60 * 60;
    if(hours)
        duration += QString("%1h ").arg(hours);
    int mins = seconds / 60;
    seconds -= mins * 60;

    duration += QString("%2m").arg(mins);
    return duration;
}

int YahooChartAPI::market_holiday_offset()
{
    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();

    if(today.month() == Jan)
    {
        // "New Years Day"/"Martin Luther King, Jr. Day"

        if(today.day() == 2 && today.dayOfWeek() == Mon)
            return 1;   // fell on a Sunday this year; assume Monday closed
        // Note: the Saturday case for "New Years" is handled in December
        if(today.day() == 1 && today.dayOfWeek() >= Mon && today.dayOfWeek() <= Fri)
            return 1;

        // "Martin Luther King, Jr. Day" is third Monday in January
        if(today.dayOfWeek() == Mon)
        {
            int mondays = 0;
            QDate jan(today.year(), today.month(), 1);
            while(jan <= today)
            {
                if(jan.dayOfWeek() == Mon)
                    ++mondays;
                jan = jan.addDays(1);
            }

            return mondays == 3 ? 1 : 0;
        }
    }

    else if(today.month() == Feb)
    {
        // "President's Day" is third Monday in January
        if(today.dayOfWeek() == Mon)
        {
            int mondays = 0;
            QDate feb(today.year(), today.month(), 1);
            while(feb <= today)
            {
                if(feb.dayOfWeek() == Mon)
                    ++mondays;
                feb = feb.addDays(1);
            }

            return mondays == 3 ? 1 : 0;
        }
    }

    else if(today.month() == Mar)
    {
        // "Good Friday"
        if((today.year() == 2018 && today.day() == 30) ||
           (today.year() == 2024 && today.day() == 29))
            return 1;
    }

    else if(today.month() == Apr)
    {
        // "Good Friday"
        if((today.year() == 2017 && today.day() == 14) ||
           (today.year() == 2019 && today.day() == 19) ||
           (today.year() == 2020 && today.day() == 10) ||
           (today.year() == 2021 && today.day() == 2) ||
           (today.year() == 2022 && today.day() == 15) ||
           (today.year() == 2023 && today.day() == 7))
            return 1;
    }

    else if(today.month() == May)
    {
        // "Memorial Day" is last Monday in May
        if(today.dayOfWeek() == Mon)
        {
            int mondays = 0;    // if this is > 1 when we're done, then this is not the last Monday
            while(today.month() == May)
            {
                if(today.dayOfWeek() == Mon)
                    ++mondays;
                today = today.addDays(1);
            }

            return mondays == 1 ? 1 : 0;
        }
    }

    else if(today.month() == Jul)
    {
        // "Independence Day" is July 4th
        if(today.day() == 5 && today.dayOfWeek() == Mon)
            return 1;
        if(today.day() == 3 && today.dayOfWeek() == Fri)
            return 1;
        if(today.day() == 4)// && (today.dayOfWeek() >= Mon && today.dayOfWeek() <= Fri))
            return 1;
    }

    else if(today.month() == Sep)
    {
        // "Labor Day" is first Monday in September
        if(today.dayOfWeek() == Mon)
        {
            int mondays = 0;
            QDate sep(today.year(), today.month(), 1);
            while(sep <= today)
            {
                if(sep.dayOfWeek() == Mon)
                    ++mondays;
                sep = sep.addDays(1);
            }

            return mondays == 1 ? 1 : 0;
        }
    }

    else if(today.month() == Nov)
    {
        // "Thanksgiving Day" is fourth Thursday in November
        if(today.dayOfWeek() == Thu)
        {
            int thursdays = 0;
            QDate nov(today.year(), today.month(), 1);
            while(nov <= today)
            {
                if(nov.dayOfWeek() == Mon)
                    ++thursdays;
                nov = nov.addDays(1);
            }

            return thursdays == 4 ? 1 : 0;
        }
    }

    else if(today.month() == Dec)
    {
        // "Christmas"
        if(today.day() == 26 && today.dayOfWeek() == Mon)
            return 1;       // fell on a Sunday this year; assume Monday closed
        if(today.day() == 24 && today.dayOfWeek() == Fri)
            return 1;       // falls on a Saturday this year; assume Friday closed
        if(today.day() == 25 && today.dayOfWeek() >= Mon && today.dayOfWeek() <= Fri)
            return 1;

        // "New Years"
        if(today.day() == 31 && today.dayOfWeek() == Fri)
            return 1;       // falls on Saturday this year; assume Friday closed
    }

    return 0;
}

void YahooChartAPI::ticker_update(const QString& status)
{
    // 'status' is in CSV format
    //   - lines with colons are key:value pairs
    //   - the first line without a colon is the first ticker report

    QStringList values;
    int timestamp = 0;

    chart_data->history.clear();

    QStringList lines = status.split("\n");
    foreach(const QString& line, lines)
    {
        if(line.contains(":"))
        {
            QStringList items = line.split(":");
            if(!items[0].compare("values"))
                values = items[1].split(",");
            else if(!items[0].compare("high"))
            {
                // this is a range of high values for the chart
                bool ok;
                QStringList subitems = items[1].split(",");
                chart_data->current_high_low = subitems[0].toFloat(&ok);
                chart_data->current_high_high = subitems[1].toFloat(&ok);
            }
            else if(!items[0].compare("low"))
            {
                // this is a range of low values for the chart
                bool ok;
                QStringList subitems = items[1].split(",");
                chart_data->current_low_low = subitems[0].toFloat(&ok);
                chart_data->current_low_high = subitems[1].toFloat(&ok);
            }
            else if(!items[0].compare("previous_close"))
            {
                bool ok;
                chart_data->previous_close = items[1].toFloat(&ok);
            }
            else if(!items[0].compare("Timestamp"))
            {
                // this is a range of time for the chart.  it will probably
                // be the regular market hours (09:30am EST -> 04:00pm EST)

                QStringList subvalues = items[1].split(",");
                chart_data->open_timestamp = subvalues[0].toInt();
                chart_data->close_timestamp = subvalues[1].toInt();
            }
        }
        else
        {
            // now we have a list of CSV rows of values.length()

            if(line.contains(","))
            {
                QStringList items = line.split(",");
                for(int i = 0;i < values.length();++i)
                {
                    if(!values[i].compare("Timestamp"))
                        timestamp = items[i].toInt();
                    else if(!values[i].compare("low"))
                    {
                        bool ok;
                        chart_data->current_low = items[i].toFloat(&ok);
                    }
                    else if(!values[i].compare("high"))
                    {
                        bool ok;
                        chart_data->current_high = items[i].toFloat(&ok);
                    }
                }

                // maintain history for ReporterDraw()
                chart_data->history.append(qMakePair(timestamp, (chart_data->current_high + chart_data->current_low) / 2.0f));
            }
        }
    }

    chart_data->volume_max_str = QString::number(chart_data->current_high_high, 'f', 2);
    chart_data->volume_min_str = QString::number(chart_data->current_low_low, 'f', 2);

    if(lock_to_max_range)
    {
        QString font_color = QString("%1%2%3")
                                .arg(QString::number(LightSkyBlueRed, 16))
                                .arg(QString::number(LightSkyBlueGreen, 16))
                                .arg(QString::number(LightSkyBlueBlue, 16)).toUpper();

        if(chart_data->current_low_low != chart_data->previous_low_low)
            chart_data->volume_min_str = QString("<font color=\"#%1\">%2</font>")
                        .arg(font_color)
                        .arg(QString::number(chart_data->current_low_low, 'f', 2));
        chart_data->previous_low_low = chart_data->current_low_low;

        if(chart_data->current_high_high != chart_data->previous_high_high)
            chart_data->volume_max_str = QString("<font color=\"#%1\">%2</font>")
                        .arg(font_color)
                        .arg(QString::number(chart_data->current_high_high, 'f', 2));
        chart_data->previous_high_high = chart_data->current_high_high;
    }

    QDateTime now = QDateTime::currentDateTime();
    QTime my_time = now.time();

    QDateTime close_datetime = QDateTime::fromTime_t(chart_data->close_timestamp);
    QTime close_time = close_datetime.time();

    if(!last_timestamp)
    {
        // first update.  check to see if we are within the market's
        // operating timeframe (daily and weekly).  if not, then there's
        // no reason to keep asking for data.  we'll "go to sleep" until
        // the market is supposed to be open.

        // QDateTime seems to be adjusting the time_t values automatically
        // for the local gmtoffset, so we don't have to do it ourselves

        int holiday_offset = market_holiday_offset();

        QDateTime open_datetime = QDateTime::fromTime_t(chart_data->open_timestamp);
        QTime open_time = open_datetime.time();

        // account for weekends when the market is closed (we also account
        // for market holidays)

        int dow = close_datetime.date().dayOfWeek();
        chart_data->next_open = open_datetime.addDays(holiday_offset + ((dow < Fri) ? 1 : 3)).toLocalTime();

        // do we need to sleep until the market is actually open?
        if(holiday_offset || my_time < open_time || my_time > close_time)
        {
            int seconds_till_open = chart_data->next_open.toTime_t() - QDateTime::currentDateTime().toLocalTime().toTime_t();
            QString duration = format_duration(seconds_till_open);

            QStringList wait_template;
            wait_template << "Ticker: <b>${TICKER}</b> (${ALIAS})";
            wait_template << "Closed: ${TIMESTAMP}";
            wait_template << "Final: ${PREVIOUS_CLOSE} / ${PREVIOUS_CLOSE_OFFSET_AMOUNT} (${PREVIOUS_CLOSE_OFFSET_PERCENTAGE}%)<br>";
            if(my_time < open_time)
                wait_template << QString("<b>Market closed; re-opens in %1</b>").arg(duration);
            else
                wait_template << QString("<b>Market closed; re-opens %1</b>").arg(chart_data->next_open.toString());

            chart_data->current_average = (chart_data->current_high + chart_data->current_low) / 2.0f;

            ReportMap report_map;
            populate_report_map(report_map, chart_data);
            QString status_str = render_report(report_map, wait_template);

#if defined(QT_DEBUG) && defined(PLAYBACK)
            // this code will play back all the cached historical data,
            // drawing the chart one step at a time.  it's used to check
            // for anomalous drawing and data handling issues.
            TickVector play_values = chart_data->history;
            chart_data->history.clear();
            while(play_values.length())
            {
                chart_data->history.append(play_values.front());
                play_values.pop_front();

                emit signal_new_data(status_str.toUtf8());
                QCoreApplication::processEvents();
                QThread::msleep(500);
            }
#else
            emit signal_new_data(status_str.toUtf8());
#endif

            QTimer::singleShot(100, this, &YahooChartAPI::FinishStory);
            QTimer::singleShot(seconds_till_open * 1000, this, &YahooChartAPI::CoverStory);
            if(seconds_till_open > 60)
                QTimer::singleShot(60000, this, &YahooChartAPI::slot_headline_sleep);

            chart_data->market_closed = true;
        }
    }

    if(!chart_data->market_closed)
    {
        chart_data->current_average = (chart_data->current_high + chart_data->current_low) / 2.0f;

        if(my_time > close_time)
        {
            // we need to sleep until the market is open again

            QDateTime open_datetime = QDateTime::fromTime_t(chart_data->open_timestamp);
            QDateTime next_open = open_datetime.addDays(market_holiday_offset() + 1).toLocalTime();
            int seconds_till_open = next_open.toTime_t() - QDateTime::currentDateTime().toLocalTime().toTime_t();

            QStringList wait_template;
            wait_template << "Ticker: <b>${TICKER}</b> (${ALIAS})";
            wait_template << "Closed: ${TIMESTAMP}";
            wait_template << "Final: ${PREVIOUS_CLOSE} / ${PREVIOUS_CLOSE_OFFSET_AMOUNT} (${PREVIOUS_CLOSE_OFFSET_PERCENTAGE}%)<br>";
            wait_template << QString("<b>Market closed; re-opens %1</b>").arg(next_open.toString());

            ReportMap report_map;
            populate_report_map(report_map, chart_data);
            QString status_str = render_report(report_map, wait_template);

            emit signal_new_data(status_str.toUtf8());

            QTimer::singleShot(100, this, &YahooChartAPI::FinishStory);
            QTimer::singleShot(seconds_till_open * 1000, this, &YahooChartAPI::CoverStory);
            if(seconds_till_open > 60)
                QTimer::singleShot(60000, this, &YahooChartAPI::slot_headline_sleep);

            chart_data->market_closed = true;
        }
        else
        {
            last_timestamp = timestamp;

            int old_timestamp = chart_data->close_timestamp;
            chart_data->close_timestamp = timestamp;

            ReportMap report_map;
            populate_report_map(report_map, chart_data);

            chart_data->close_timestamp = old_timestamp;

            QString status_str = render_report(report_map, report_template);

            emit signal_new_data(status_str.toUtf8());
        }
    }
}

void YahooChartAPI::ticker_update(const QDomDocument& /*status*/)
{
}

void YahooChartAPI::ticker_update(const QJsonObject& /*status*/)
{
}

void YahooChartAPI::error(const QString& error_message)
{
    emit signal_new_data(error_message.toUtf8());
}

void YahooChartAPI::populate_report_map(ReportMap& report_map, ChartDataPointer chart_data)
{
    // single-line arrow
    QString symbol("&#8596;");
    if((chart_data->current_average - chart_data->previous_close) < 0.0f)
        symbol = "&#8595;";
    else if((chart_data->current_average - chart_data->previous_close) > 0.0f)
        symbol = "&#8593;";

    // double-line arrow
//    QString symbol("&#8660;");
//    if((chart_data->current_average - previous) < 0.0f)
//        symbol = "&#8659;";
//    else if((chart_data->current_average - previous) > 0.0f)
//        symbol = "&#8657;";

    report_map["TICKER"] = ticker;
    report_map["ALIAS"] = ticker_alias;
    report_map["AVERAGE"] = QString::number(chart_data->current_average, 'f', 2);
    report_map["PREVIOUS_CLOSE"] = QString::number(chart_data->previous_close, 'f', 2);
    report_map["PREVIOUS_CLOSE_OFFSET_AMOUNT"] = QString("%1%2").arg(symbol).arg(QString::number(fabs(chart_data->current_average - chart_data->previous_close), 'f', 2));
    report_map["PREVIOUS_CLOSE_OFFSET_PERCENTAGE"] = QString("%1%2").arg(symbol).arg(QString::number(fabs(((chart_data->current_average - chart_data->previous_close) / chart_data->previous_close) * 100.f), 'f', 2));
    report_map["TIMESTAMP"] = QDateTime::fromTime_t(chart_data->close_timestamp).toLocalTime().toString();

    report_map["RANGE_LOCKED"] = "&#128275;";
    if(lock_to_max_range)
        report_map["RANGE_LOCKED"] = "&#128274;";

    report_map["INDICATORS_VISIBLE"] = "";
    if(ensure_indicators_are_visible)
        report_map["INDICATORS_VISIBLE"] = "&#128065;";

    report_map["OPEN_OFFSET_AMOUNT"] = "";
    report_map["OPEN_OFFSET_PERCENTAGE"] = "";

    if(chart_data->history.length())
    {
        const TickPair& p = chart_data->history.front();
        symbol = "&#8596;";
        if(chart_data->current_average < p.second)
            symbol = "&#8595;";
        else if(chart_data->current_average > p.second)
            symbol = "&#8593;";

        report_map["OPEN_OFFSET_AMOUNT"] = QString("%1%2").arg(symbol).arg(QString::number(fabs(chart_data->current_average - p.second), 'f', 2));
        report_map["OPEN_OFFSET_PERCENTAGE"] = QString("%1%2").arg(symbol).arg(QString::number(fabs(((chart_data->current_average - p.second) / p.second) * 100.f), 'f', 2));
    }
}

QString YahooChartAPI::render_report(const ReportMap& report_map, const QStringList& report_template)
{
    QString report = report_template.join("<br>");
    foreach(const QString& key, report_map.keys())
    {
        QString token = QString("${%1}").arg(key);
        report.replace(token, report_map[key]);
    }

    return report;
}

QString YahooChartAPI::capitalize(const QString& str)
{
    QString tmp = str.toLower();
    tmp[0] = str[0].toUpper();
    return tmp;
}

PollerPointer YahooChartAPI::acquire_poller(const QUrl& story, const QString& ticker, int timeout)
{
    if(!poller_map.contains(ticker))
    {
        PollerData pd;
        pd.poller = PollerPointer(new YahooChartAPIPoller(TickerFormat::CSV, story, ticker, timeout));
        poller_map[ticker] = pd;
    }

    PollerData& pd = poller_map[ticker];
    ++pd.reference_count;
    return pd.poller;
}

void YahooChartAPI::release_poller(const QString &ticker)
{
    if(!poller_map.contains(ticker))
        return;

    PollerData& pd = poller_map[ticker];
    if(--pd.reference_count == 0)
        poller_map.remove(ticker);
}

void YahooChartAPI::slot_headline_sleep()
{
    emit signal_highlight(.6, 30000);
}
