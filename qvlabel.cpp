#include "qvlabel.h"

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QAbstractTextDocumentLayout>

QVLabel::QVLabel(QWidget *parent)
    : QLabel(parent)
{
}

QVLabel::QVLabel(const QString &text, bool configure_for_left, QWidget *parent)
    : configure_for_left(configure_for_left),
      QLabel(text, parent)
{
}

void QVLabel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    QRect s = geometry();
    if(configure_for_left)
    {
        painter.translate(0, s.height());
        painter.rotate(270);
    }
    else
    {
        painter.translate(s.width(), 0);
        painter.rotate(90);
    }

    QTextDocument td;
    td.setHtml(text());

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, painter.pen().color());
    ctx.clip = QRect(0, 0, s.width(), s.height());
    td.documentLayout()->draw( &painter, ctx );

///    painter.drawText(QRect(0, 0, s.height(), s.width()), Qt::AlignHCenter|Qt::AlignVCenter, text());

//    painter.save();
//    painter.translate(sizeHint().width(),0);
//    painter.rotate(90);
//    painter.drawText(QRect(QPoint(0,0), QLabel::sizeHint()), Qt::AlignLeft, text());
//    painter.restore();

//    painter.translate(sizeHint().width(),0);
//    painter.rotate(90);

//    painter.translate(0, sizeHint().height());
//    painter.rotate(270);

//    QRect bounding = QRect(QPoint(0,0), QLabel::sizeHint());
//    painter.drawText(bounding, Qt::AlignCenter, text(), &bounding);
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
