#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>

#include "chartapi.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

YahooChartAPI::PollerMap YahooChartAPI::poller_map;

YahooChartAPI::YahooChartAPI(QObject *parent)
    : poll_timeout(60),
      last_timestamp(0),
      display_graph(true),
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
    return 1;
}

RequirementsFormats YahooChartAPI::RequiresFormat() const
{
    return RequirementsFormats::Simple;
}

bool YahooChartAPI::RequiresUpgrade(int /*version*/, QStringList& /*parameters*/)
{
    error_message.clear();
    return false;
}

QStringList YahooChartAPI::Requires(int /*version*/) const
{
    QStringList definitions;

    // "simple" requirements format

    // parameter names ending with an asterisk are required
    definitions << "Symbol:"        << "string:^dji"
                << "Alias:"         << "string"

                // how many seconds between polls? (default: 60)
                << "Polling (sec):" << QString("integer:%1").arg(poll_timeout)

                // should we display the graph instead of just text?
                << "Display graph instead of just text" << "check:true"

                << "Format:"        << QString("multiline:%1").arg(report_template.join("<br>\n"));

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
    td.setHtml(QString("<b>%1</b><br>&#8657;%2<br>&#8659;%3<br><b>%4</b><br>%5 (%6%)<br><font color=\"#%7\">%8 (%9%)</font>")
                                        .arg(report_map["TICKER"])
                                        .arg(QString::number(volume_high, 'f', 2))
                                        .arg(QString::number(volume_low, 'f', 2))
                                        .arg(current_average_str)
                                        .arg(report_map["PREVIOUS_CLOSE_OFFSET_AMOUNT"])
                                        .arg(report_map["PREVIOUS_CLOSE_OFFSET_PERCENTAGE"])
                                        .arg(font_color)
                                        .arg(report_map["OPEN_OFFSET_AMOUNT"])
                                        .arg(report_map["OPEN_OFFSET_PERCENTAGE"]));

    QSizeF doc_size = td.documentLayout()->documentSize();
    QRect new_bounds = QRect(bounds.left(), bounds.top(), bounds.width() - doc_size.width() - 5, bounds.height());
    QRect graph_bounds = new_bounds.adjusted(1, 0, -(LatestPointMarkerWidth * 2 + 1), 0);

    painter.save();

    painter.translate(bounds.right() - doc_size.width(), 5);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, painter.pen().color());
    td.documentLayout()->draw(&painter, ctx);

    painter.restore();

    if(chart_data->market_closed)
    {
        td.setHtml(tr("<html><head></head><body><td><tr><h1 align=center>MARKET<br>CLOSED</h1><br>re-opens %1</tr><td></body></html>")
                    .arg(chart_data->next_open.toString()));
        doc_size = td.documentLayout()->documentSize();

        painter.save();

        painter.translate((new_bounds.width() - doc_size.width()) / 2, (new_bounds.height() - doc_size.height()) / 2);

        ctx.palette.setColor(QPalette::Text, painter.pen().color().darker(250));
        td.documentLayout()->draw(&painter, ctx);

        painter.restore();
    }

    // draw the simple chart on the Headline
    painter.save();

    painter.drawLine(new_bounds.left(), new_bounds.top(), new_bounds.left(), new_bounds.bottom());
    painter.drawLine(new_bounds.left(), new_bounds.bottom(), new_bounds.right(), new_bounds.bottom());
    painter.drawLine(new_bounds.right(), new_bounds.bottom(), new_bounds.right(), new_bounds.top());

    if(volume_low != volume_high)
    {
        VolumeVector volume_ticks;

        // bottom is volume_low; top is volume_high
        int chart_height = new_bounds.height();
        float chart_range = volume_high - volume_low;
        if(chart_range < chart_height)
        {
            // scale range up into height
            int pixel_increment = round((chart_range / chart_height) + 0.5);
            float volume_increment = chart_range / chart_height;
            float i = volume_low;
            int p = 0;
            while(i < volume_high)
            {
                volume_ticks.append(qMakePair(i, p));
                i+=volume_increment;
                p+=pixel_increment;
            }
        }
        else
        {
            // scale range down into height
            int pixel_increment = 1;
            float volume_increment = chart_range / chart_height;
            float i = volume_low;
            int p = 0;
            while(i < volume_high)
            {
                volume_ticks.append(qMakePair(i, p));
                i+=volume_increment;
                p+=pixel_increment;
            }
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

        foreach(const TickPair& p, chart_data->history)
        {
            time_offset = ((p.first - open_timestamp) / 60) * width_increment;

            volume_offset = 0;
            foreach(const VolumePair& tick, volume_ticks)
            {
                if(tick.first > p.second)
                {
                    volume_offset = tick.second;
                    break;
                }
            }

            if(chart_data->opening_point.isNull())
            {
                chart_data->opening_point.setX(new_bounds.left() + time_offset);
                chart_data->opening_point.setY(new_bounds.bottom() - volume_offset);

                // highlight the opening volume point on the graph
                painter.save();
                  painter.setPen(painter.pen().color().darker());
                  painter.drawLine(QPoint(new_bounds.left()+1, chart_data->opening_point.y()), QPoint(new_bounds.right()-1, chart_data->opening_point.y()));
                painter.restore();
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

        painter.save();
          painter.setPen(QColor(LightSkyBlueRed, LightSkyBlueGreen, LightSkyBlueBlue));
          painter.drawLine(QPoint(last_point.x(), chart_data->opening_point.y() + ((chart_data->opening_point.y() < last_point.y()) ? 1 : -1)), QPoint(last_point.x(), last_point.y()));
        painter.restore();

        // highlight the current volume point
        painter.save();
          painter.setBrush(painter.pen().color());
          painter.setPen(QColor(0, 0, 255));
          painter.drawEllipse(last_point, LatestPointMarkerWidth, LatestPointMarkerHeight);
        painter.restore();
    }

    painter.restore();
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

    if(!last_timestamp)
    {
        // first update.  check to see if we are within the market's
        // operating timeframe (daily and weekly).  if not, then there's
        // no reason to keep asking for data.  we'll "go to sleep" until
        // the market is supposed to be open.

        QDateTime now = QDateTime::currentDateTime();
        QTime my_time = now.time();

        // QDateTime seems to be adjusting the time_t values automatically
        // for the local gmtoffset, so we don't have to do it ourselves

        QDateTime open_datetime = QDateTime::fromTime_t(chart_data->open_timestamp);
        QDateTime close_datetime = QDateTime::fromTime_t(chart_data->close_timestamp);

        QTime open_time = open_datetime.time();
        QTime close_time = close_datetime.time();

        // account for weekends when the market is closed
        int dow = now.date().dayOfWeek();
        chart_data->next_open = open_datetime.addDays((dow < Fri || dow == Sun) ? 1 : (dow == Fri ? 3 : 2)).toLocalTime();

        // do we need to sleep until the market is actually open?
        if(my_time < open_time || my_time > close_time)
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

            emit signal_new_data(status_str.toUtf8());

            QTimer::singleShot(100, this, &YahooChartAPI::FinishStory);
            QTimer::singleShot(seconds_till_open * 1000, this, &YahooChartAPI::CoverStory);

            chart_data->market_closed = true;
        }
    }

    if(!chart_data->market_closed && timestamp > last_timestamp)
    {
        last_timestamp = timestamp;

        chart_data->current_average = (chart_data->current_high + chart_data->current_low) / 2.0f;

        int old_timestamp = chart_data->close_timestamp;
        chart_data->close_timestamp = timestamp;

        ReportMap report_map;
        populate_report_map(report_map, chart_data);

        chart_data->close_timestamp = old_timestamp;

        QString status_str = render_report(report_map, report_template);

        emit signal_new_data(status_str.toUtf8());

        QDateTime close_datetime = QDateTime::fromTime_t(chart_data->close_timestamp);
        QTime close_time = close_datetime.time();
        QDateTime last_datetime = QDateTime::fromTime_t(last_timestamp);
        QTime last_time = last_datetime.time();

        if(last_time >= close_time)
        {
            // we need to sleep until the market is open again

            QDateTime open_datetime = QDateTime::fromTime_t(chart_data->open_timestamp);
            QDateTime next_open = open_datetime.addDays(1).toLocalTime();
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

            chart_data->market_closed = true;
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
