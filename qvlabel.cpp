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
    : NewsroomLabel(parent)
{
}

QHLabel::QHLabel(const QString &text, QWidget *parent)
    : NewsroomLabel(text, parent)
{
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
    painter.save();

    // see if we can detect any progress indicator in the plain
    // text, and put a progress bar on the headline if so.

    QString plain_text = td.toPlainText();
    QRegExp re("\\s(\\d+)%");
    if(re.indexIn(plain_text) != -1)
    {
        float percent = re.cap(1).toFloat() / 100.0f;

        // draw a progress bar along the bottom
        x = 5;
        y = s.height() - 5 - 5;
        int w = s.width() - x * 2;
        int h = 5;

        QPalette p = palette();
        QColor base_color = p.color(QPalette::Window).lighter(50);
        painter.setPen(QPen(base_color));
        painter.setBrush(QBrush(base_color));
        painter.drawRect(x, y, w, h);

        QColor highlight_color = base_color.lighter(100);

        painter.setPen(QPen(highlight_color));
        painter.setBrush(QBrush(highlight_color));
        painter.drawRect(x, y, (int)(w * percent), h);
    }

    painter.restore();
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
