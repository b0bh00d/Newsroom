#include "qvlabel.h"

#include <QPainter>

QVLabel::QVLabel(QWidget *parent)
    : QLabel(parent)
{
}

QVLabel::QVLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
}

void QVLabel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    QRect s = geometry();
    painter.translate(0, s.height());
    painter.rotate(-90);
    painter.drawText(QRect(0, 0, s.height(), s.width()), Qt::AlignHCenter|Qt::AlignVCenter, text());

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
