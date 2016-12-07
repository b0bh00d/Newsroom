#include <QPainter>
#include <QPaintEvent>

#include "highlightwidget.h"

#define OPACITY_MIN .2
#define OPACITY_MAX .6

HighlightWidget::HighlightWidget(QWidget *parent)
    : opacity(OPACITY_MIN),
      QWidget(parent)
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
    painter.drawRect(geometry());
}
