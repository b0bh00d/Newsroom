#include "label.h"

#include <QtCore/QRegExp>

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

VLabel::VLabel(QWidget *parent)
    : ILabel(parent)
{
}

VLabel::VLabel(const QString &text, bool configure_for_left, QWidget *parent)
    : configure_for_left(configure_for_left),
      ILabel(text, parent)
{
}

void VLabel::paintEvent(QPaintEvent* /*event*/)
{
    if(compact_mode)
        return;

    QPainter painter(this);

    QRect s = geometry();

    QTextDocument td;
    td.setDocumentMargin(margin);

    QRegExp html_tags("<[^>]*>");
    if(html_tags.indexIn(text()) != -1)
        td.setHtml(text());
    else
        td.setPlainText(text());
    QSizeF doc_size = td.documentLayout()->documentSize();

    if(shrink_text_to_fit)
    {
        QFont f = td.defaultFont();
        for(;;)
        {
            doc_size = td.documentLayout()->documentSize();
            if(doc_size.width() < s.height())
                break;

            if((f.pointSizeF() - .1) < 6.0f)
            {
                // let it just clip the remaining text
                //f.setPointSizeF(original_point_size);
                //td.setDefaultFont(f);
                break;
            }

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

    painter.save();

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

    if(!shrink_text_to_fit)
        painter.setClipRect(margin, margin, s.width() - margin * 2, s.height() - margin * 2);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, painter.pen().color());
    td.documentLayout()->draw(&painter, ctx);

    painter.restore();
}

QSize VLabel::minimumSizeHint() const
{
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.width(), s.height());
}

QSize VLabel::sizeHint() const
{
    QSize s = QLabel::sizeHint();
    return QSize(s.width(), s.height());
}


HLabel::HLabel(QWidget *parent)
    : detect_progress(false),
      ILabel(parent)
{
    init();
}

HLabel::HLabel(const QString &text, QWidget *parent)
    : detect_progress(false),
      ILabel(text, parent)
{
    init();
}

void HLabel::init()
{
    QPalette p = palette();
    progress_color = p.color(QPalette::Active, QPalette::Mid).lighter(75);
    progress_highlight = progress_color.lighter(135);
}

void HLabel::set_progress_detection(bool detect, const QString& re, bool on_top)
{
    detect_progress = detect;
    progress_re = re;
    progress_on_top = on_top;

    progress_x = margin;
    progress_y = margin;     // preset for top
    progress_w = 0;
    progress_h = 5;
}

void HLabel::changeEvent(QEvent* event)
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

void HLabel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    QRect s = geometry();

    QTextDocument td;
    td.setDocumentMargin(margin);

    QRegExp html_tags("<[^>]*>");
    if(html_tags.indexIn(text()) != -1)
        td.setHtml(text());
    else
        td.setPlainText(text());

    if(!compact_mode)
    {
        QSizeF doc_size = td.documentLayout()->documentSize();

        if(shrink_text_to_fit)
        {
            QFont f = td.defaultFont();
            for(;;)
            {
                doc_size = td.documentLayout()->documentSize();
                if(doc_size.width() < s.width())
                    break;

                if((f.pointSizeF() - .1) < 6.0f)
                {
                    // let it just clip the remaining text
                    //f.setPointSizeF(original_point_size);
                    //td.setDefaultFont(f);
                    break;
                }

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
        painter.setClipRect(margin, margin, s.width() - margin * 2, s.height() - margin * 2);

        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.palette.setColor(QPalette::Text, painter.pen().color());
        td.documentLayout()->draw(&painter, ctx);

        painter.restore();
    }

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
            if(compact_mode)
            {
                // progress bar consumes the whole window
                progress_h = s.height() - margin * 2 - 1;
                progress_x = margin;
                progress_y = margin;
                progress_w = s.width() - margin * 2 - 1;
            }
            else
            {
                progress_h = 5;
                progress_x = margin;
                progress_y = s.height() - progress_h - margin;
                progress_w = s.width() - margin * 2 - 1;

                if(progress_on_top)
                    progress_y = 5;
            }

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

QSize HLabel::minimumSizeHint() const
{
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.width(), s.height());
}

QSize HLabel::sizeHint() const
{
    QSize s = QLabel::sizeHint();
    return QSize(s.width(), s.height());
}
