#include "qvlabel.h"

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

QVLabel::QVLabel(QWidget *parent)
    : NewsroomLabel(parent)
{
}

QVLabel::QVLabel(const QString &text, bool configure_for_left, QWidget *parent)
    : configure_for_left(configure_for_left),
      NewsroomLabel(text, parent)
{
}

void QVLabel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    QRect s = geometry();

    QTextDocument td;
    td.setHtml(text());
    QSizeF doc_size = td.documentLayout()->documentSize();

    if(shrink_text_to_fit)
    {
        QFont f = td.defaultFont();
        for(;;)
        {
            doc_size = td.documentLayout()->documentSize();
            if(doc_size.width() < s.height())
                break;
            f.setPointSizeF(f.pointSizeF() - .1);
            td.setDefaultFont(f);
        }
    }

    int y = 0;
    int x = 0;

    if(alignment() & Qt::AlignVCenter)
    {
        y = (s.width() - doc_size.height()) / 2;
        y = (y < 0) ? 0 : y;
    }
    else if(alignment() & Qt::AlignBottom)
    {
        y = s.width() - doc_size.height();
        y = (y < 0) ? 0 : y;
    }

    if(alignment() & Qt::AlignHCenter)
    {
        x = (s.height() - doc_size.width()) / 2;
        x = (x < 0) ? 0 : x;
    }
    else if(alignment() & Qt::AlignRight)
    {
        x = s.height() - doc_size.width();
        x = (x < 0) ? 0 : x;
    }

    if(configure_for_left)
    {
        painter.translate(y, s.height() - x);
        painter.rotate(270);
    }
    else
    {
        painter.translate(s.width() - y, x);
        painter.rotate(90);
    }

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, painter.pen().color());
    ctx.clip = QRect(0, 0, s.width(), s.height());
    td.documentLayout()->draw( &painter, ctx );
}

QSize QVLabel::minimumSizeHint() const
{
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.width(), s.height());
}

QSize QVLabel::sizeHint() const
{
    QSize s = QLabel::sizeHint();
    return QSize(s.width(), s.height());
}


QHLabel::QHLabel(QWidget *parent)
    : detect_progress(false),
      NewsroomLabel(parent)
{
    init();
}

QHLabel::QHLabel(const QString &text, QWidget *parent)
    : detect_progress(false),
      NewsroomLabel(text, parent)
{
    init();
}

void QHLabel::init()
{
    QPalette p = palette();
    progress_color = p.color(QPalette::Active, QPalette::Mid).lighter(75);
    progress_highlight = progress_color.lighter(135);
}

void QHLabel::set_progress_detection(bool detect, const QString& re, bool on_top)
{
    detect_progress = detect;
    progress_re = re;
    progress_on_top = on_top;

    progress_margin = 5;
    progress_x = 5;
    progress_y = 5;     // preset for top
    progress_w = 0;
    progress_h = 5;
}

void QHLabel::changeEvent(QEvent* event)
{
    if(event->type() == QEvent::StyleChange)
    {
        QString stylesheet = styleSheet();
        QRegExp bc("background\\-color\\s*:.*rgb[a]?\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)");
        if(bc.indexIn(stylesheet) != -1)
        {
            int r = bc.cap(1).toInt();
            int g = bc.cap(2).toInt();
            int b = bc.cap(3).toInt();
            progress_color = QColor(r, g, b).lighter(125);
            progress_highlight = progress_color.lighter(150);
        }
        else
        {
            QPalette p = palette();
            progress_color = p.color(QPalette::Active, QPalette::Mid).lighter(75);
            progress_highlight = progress_color.lighter(135);
        }
    }
}

void QHLabel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    QRect s = geometry();

    QTextDocument td;
    td.setHtml(text());
    QSizeF doc_size = td.documentLayout()->documentSize();

    if(shrink_text_to_fit)
    {
        QFont f = td.defaultFont();
        for(;;)
        {
            doc_size = td.documentLayout()->documentSize();
            if(doc_size.width() < s.width())
                break;
            f.setPointSizeF(f.pointSizeF() - .1);
            td.setDefaultFont(f);
        }
    }

    int y = 0;
    int x = 0;

    if(alignment() & Qt::AlignVCenter)
    {
        y = (s.height() - doc_size.height()) / 2;
        y = (y < 0) ? 0 : y;
    }
    else if(alignment() & Qt::AlignBottom)
    {
        y = s.height() - doc_size.height();
        y = (y < 0) ? 0 : y;
    }

    if(alignment() & Qt::AlignHCenter)
    {
        x = (s.width() - doc_size.width()) / 2;
        x = (x < 0) ? 0 : x;
    }
    else if(alignment() & Qt::AlignRight)
    {
        x = s.width() - doc_size.width();
        x = (x < 0) ? 0 : x;
    }

    painter.save();
    painter.translate(x, y);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, painter.pen().color());
    ctx.clip = QRect(0, 0, s.width(), s.height());
    td.documentLayout()->draw(&painter, ctx);

    painter.restore();

    if(detect_progress)
    {
        painter.save();

        // see if we can detect any progress indicator in the plain
        // text, and put a progress bar on the headline if so.

        QString plain_text = td.toPlainText();
        QRegExp re(progress_re);
        if(re.indexIn(plain_text) != -1)
        {
            float percent = re.cap(1).toFloat() / 100.0f;

            // draw a progress bar along the bottom
            progress_h = 5;
            progress_x = progress_margin;
            progress_y = s.height() - progress_h - progress_margin;
            progress_w = s.width() - progress_margin * 2;

            if(progress_on_top)
                progress_y = 5;

            painter.setPen(QPen(progress_color));
            painter.setBrush(QBrush(progress_color));
            painter.drawRect(progress_x, progress_y, progress_w, progress_h);

            painter.setPen(QPen(progress_highlight));
            painter.setBrush(QBrush(progress_highlight));
            painter.drawRect(progress_x, progress_y, (int)(progress_w * percent), progress_h);
        }

        painter.restore();
    }
}

QSize QHLabel::minimumSizeHint() const
{
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.width(), s.height());
}

QSize QHLabel::sizeHint() const
{
    QSize s = QLabel::sizeHint();
    return QSize(s.width(), s.height());
}
