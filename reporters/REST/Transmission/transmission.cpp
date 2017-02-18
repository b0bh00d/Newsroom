#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>

#include "transmission.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

Transmission::PollerMap Transmission::poller_map;

Transmission::Transmission(QObject *parent)
    : owner_draw(true),
      my_slot(1),
      max_ratio(0.0f),
      poll_timeout(10),
      IReporter2(parent)
{
    report_template << "Slot <b>${SLOT}</b>";
    report_template << "Name: ${NAME}";
    report_template << "Status: ${STATUS}";
    report_template << "Ratio: ${RATIO}";
    report_template << "ETA: <b>${ETA}</b>";
}

// IReporter

QStringList Transmission::DisplayName() const
{
    return QStringList() << QObject::tr("Transmission") <<
                            QObject::tr("Reports on the status of torrents in specific slots of a Transmission client");
}

QByteArray Transmission::PluginID() const
{
    return "{35DA31BE-E352-4627-8AC1-A7B9D8A50E4B}";
}

float Transmission::Supports(const QUrl& entity) const
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

    // There's no actual way to determine, just from the URL,
    // if this is indeed pointing at a 'transmission-rest'
    // helper node...unless it actually SAYS it in the URL...

    if(entity_str.contains("api/v1/transmission-rest"))
        return 1.0f;

    // Let them know it might, or might not, be a 'transmission-rest'
    // endpoint.  If it isn't, the user will get a network error
    // for a Headline...and then there'll be no doubt.
    return 0.5f;
}

int Transmission::RequiresVersion() const
{
    return 1;
}

RequirementsFormats Transmission::RequiresFormat() const
{
    return RequirementsFormats::Simple;
}

bool Transmission::RequiresUpgrade(int /*version*/, QStringList& /*parameters*/)
{
    error_message.clear();
    return false;
}

QStringList Transmission::Requires(int /*version*/) const
{
    QStringList definitions;

    // "simple" requirements format

    // parameter names ending with an asterisk are required
    definitions << "Slot to monitor:" << "int:1"

                // how many seconds between polls? (default: 60)
                << "Polling (sec):"   << QString("integer:%1").arg(poll_timeout)

                << "Format:"          << QString("multiline:%1").arg(report_template.join("<br>\n"));

    return definitions;
}

bool Transmission::SetRequirements(const QStringList& parameters)
{
    error_message.clear();

    if(parameters.count() > Param::Slot && !parameters[Param::Slot].isEmpty())
    {
        my_slot = parameters[Param::Slot].toInt();
        if(my_slot < 1)
            my_slot = 1;
    }

    if(parameters.count() > Param::Poll && !parameters[Param::Poll].isEmpty())
        poll_timeout = parameters[Param::Poll].toInt();

    if(parameters.count() > Param::Template && !parameters[Param::Template].isEmpty())
    {
        QString report_template_str = parameters[Param::Template];
        report_template_str.remove('\r');
        report_template_str.remove('\n');
        report_template = report_template_str.split("<br>");
    }

    return true;
}

void Transmission::SetStory(const QUrl& url)
{
    story = url;
}

bool Transmission::CoverStory()
{
    error_message.clear();

    poller = acquire_poller(story, poll_timeout);
    if(poller.isNull())
        return false;

    // this call to add_interest() will filter messages to just those
    // relating to this project and (optionally) builder.  the TransmissionPoller
    // will invoke our public methods for each event type (started, progress,
    // etc.) since Qt does not provide a canonical way of creating dynamic
    // signals and slots at runtime--we can't construct signals/slots for
    // specific project/builder combinations.

    int flags = 0;
    poller->add_interest(my_slot, this, flags);

    // do an initial "idle" Headline
    QString status = QString("Slot #<b>%1</b>").arg(my_slot);
    emit signal_new_data(status.toUtf8());

    return true;
}

bool Transmission::FinishStory()
{
    error_message.clear();

    poller->remove_interest(my_slot, this);
    poller.clear();     // we're done with it; don't hang on

    release_poller(story);

    return true;
}

void Transmission::Secure(QStringList& /*parameters*/) const
{
}

void Transmission::Unsecure(QStringList& /*parameters*/) const
{
}

// IReporter2

bool Transmission::UseReporterDraw() const
{
    return owner_draw;
}

const int LightSkyBlueRed = 135;
const int LightSkyBlueGreen = 206;
const int LightSkyBlueBlue = 250;

const int RosyBrownRed = 238;
const int RosyBrownGreen = 180;
const int RosyBrownBlue = 180;

void Transmission::ReporterDraw(const QRect& bounds, QPainter& painter)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    ReportMap report_map;
    if(!latest_status.isEmpty())
        populate_report_map(report_map, latest_status);

    QTextDocument td;
    td.setDocumentMargin(0);

    if(latest_status.isEmpty())
    {
        td.setHtml("(<b>Pending</b>)");
        QSizeF doc_size = td.documentLayout()->documentSize();

        painter.save();
        painter.translate((bounds.width() - doc_size.width()) / 2, (bounds.height() - doc_size.height()) / 2);

        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.palette.setColor(QPalette::Text, painter.pen().color());
        td.documentLayout()->draw(&painter, ctx);

        painter.restore();

        return;
    }

    painter.setClipRect(bounds);

    // draw an ellipse with angle indicators.
    //
    // the top (outermost) ellipse indicates how much of the torrent we
    // have ("DONE"). concentric inner ellipses indicate multiples of
    // the sharing amount ("RATIO"), with each complete ellipse representing
    // a sharing factor of 100%.  so, for a torrent with a sharing factor of
    // "2", there will eventually be three ellipses, one for completeness,
    // and two for the pair of 100% sharing factors.

    QColor done_color(RosyBrownRed, RosyBrownGreen, RosyBrownBlue);

    int done_angle = 0;
    QString value = report_map["DONE"];
    QRegExp regex("(\\d+)%");
    if(regex.indexIn(value) != -1)
        done_angle = (int)(360 * (regex.cap(1).toInt() / 100.0f) * -1);

    int diameter = bounds.height() - (bounds.height() * .05);
    QRect ellipse_rect(bounds.left(), bounds.top(), diameter, diameter);
    painter.save();
      painter.setPen(done_color);
      painter.drawEllipse(ellipse_rect);
      painter.setBrush(QBrush(done_color));
      painter.drawPie(ellipse_rect, 90*16, -(done_angle - 90)*16);
    painter.restore();

    QColor ratio_color(LightSkyBlueRed, LightSkyBlueGreen, LightSkyBlueBlue);

    // if we know the max sharing ratio, then we will adjust the reduction
    // based on it; otherwise, we do a more conservative reduction and hope
    // they aren't crazy generous (and we run out of slices)...

    int reduction = diameter * 0.08;
    if(max_ratio != 0.0f)
    {
        int slices = round(max_ratio + 0.5f);
        reduction = diameter / (slices + 3);
    }

    float ratio = report_map["RATIO"].toFloat();
    while(ratio > 1.0f)
    {
        ellipse_rect.adjust(reduction, reduction, -reduction, -reduction);

        painter.save();
          painter.setPen(ratio_color);
          painter.setBrush(QBrush(ratio_color));
          painter.drawPie(ellipse_rect, 90*16, -(ratio * 360)*16);
        painter.restore();

        ratio_color = ratio_color.darker(115);
        ratio -= 1.0f;
    }

    ellipse_rect.adjust(reduction, reduction, -reduction, -reduction);

    painter.save();
      painter.setPen(ratio_color);
      painter.setBrush(QBrush(ratio_color));
      painter.drawPie(ellipse_rect, 90*16, -(ratio * 360)*16);
    painter.restore();

    QString status(report_map["STATUS"]);
    QString name(report_map["NAME"]);
    QString label = QString("%1: <i>%2</i>").arg(status).arg(name);
    td.setHtml(label);

    int left_margin = diameter + (bounds.width() * 0.03);
    int text_width = bounds.width() - left_margin;
    QSizeF doc_size = td.documentLayout()->documentSize();
    for(;;)
    {
        doc_size = td.documentLayout()->documentSize();
        if(doc_size.width() < text_width)
            break;
        name = QString("%1...").arg(name.left(name.length() - 4));
        label = QString("%1: <i>%2</i>").arg(status).arg(name);
        td.setHtml(label);
    }

    painter.save();
      painter.translate(bounds.left() + left_margin, bounds.top() + ((bounds.height() - doc_size.height()) / 2));
      QAbstractTextDocumentLayout::PaintContext ctx;
      ctx.palette.setColor(QPalette::Text, painter.pen().color());
      td.documentLayout()->draw(&painter, ctx);
    painter.restore();

    painter.restore();  // Antialiasing
}

// Transmission

void Transmission::status(const QJsonObject& status, float maxratio)
{
    latest_status = status;
    max_ratio = maxratio;

    QString status_str;

    ReportMap report_map;
    populate_report_map(report_map, status);
    status_str = render_report(report_map);

    emit signal_new_data(status_str.toUtf8());
}

void Transmission::error(const QString& error_message)
{
    emit signal_new_data(error_message.toUtf8());
}

void Transmission::populate_report_map(ReportMap& report_map, const QJsonObject& status)
{
    QString max_ratio_str = QString::number(max_ratio, 'f', 2);

    report_map["SLOT"]   = QString::number(my_slot);
    report_map["DONE"]   = status["done"].toString();
    report_map["HAVE"]   = status["have"].toString();
    report_map["ETA"]    = status["eta"].toString();
    report_map["UP"]     = status["up"].toString();
    report_map["DOWN"]   = status["down"].toString();
    report_map["RATIO"]  = status["ratio"].toString();
    report_map["STATUS"] = status["status"].toString();
    if(!report_map["STATUS"].toLower().compare("stopped") && !max_ratio_str.compare(report_map["RATIO"]))
        report_map["STATUS"] = "Finished";  // consider this complete
    report_map["NAME"]   = status["name"].toString();
}

QString Transmission::render_report(const ReportMap& report_map)
{
    QString report = report_template.join("<br>");
    foreach(const QString& key, report_map.keys())
    {
        QString token = QString("${%1}").arg(key);
        report.replace(token, report_map[key]);
    }

    return report;
}

QString Transmission::capitalize(const QString& str)
{
    QString tmp = str.toLower();
    tmp[0] = str[0].toUpper();
    return tmp;
}

PollerPointer Transmission::acquire_poller(const QUrl& target, int timeout)
{
    if(!poller_map.contains(target))
    {
        PollerData pd;
        pd.poller = PollerPointer(new TransmissionPoller(target, timeout));
        poller_map[target] = pd;
    }

    PollerData& pd = poller_map[target];
    ++pd.reference_count;
    return pd.poller;
}

void Transmission::release_poller(const QUrl& target)
{
    if(!poller_map.contains(target))
        return;

    PollerData& pd = poller_map[target];
    if(--pd.reference_count == 0)
        poller_map.remove(target);
}
