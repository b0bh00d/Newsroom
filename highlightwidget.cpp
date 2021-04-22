#include <QPainter>
#include <QPaintEvent>

#include "highlightwidget.h"

#define OPACITY_MIN .2
#define OPACITY_MAX .6

HighlightWidget::HighlightWidget(QWidget *parent)
    : QWidget(parent),
      opacity(OPACITY_MIN)
{
    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowOpacity(opacity);
}

HighlightWidget::~HighlightWidget()
{
}

void HighlightWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    painter.setPen(QColor(0xff, 0xff, 0xff));
    auto g = geometry();
    painter.drawRect(g);

//    QVector<QPoint> points;
//    painter.setBrush(QBrush(QColor(0x0f, 0x0f, 0x0f)));
////    painter.setPen(QColor(0x0f, 0x0f, 0x0f));
//    points.push_back(QPoint(g.left(), g.top()));
//    points.push_back(QPoint(g.right(), g.top()));

//    points.push_back(QPoint(g.right(), g.top()));
//    points.push_back(QPoint(g.right(), g.bottom()));

//    points.push_back(QPoint(g.right(), g.bottom()));
//    points.push_back(QPoint(g.left(), g.bottom()));

//    points.push_back(QPoint(g.left(), g.bottom()));
//    points.push_back(QPoint(g.left(), g.top()));

//    painter.drawLines(points);
}
