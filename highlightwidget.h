#pragma once

#include <QWidget>
#include <QPaintEvent>

class HighlightWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HighlightWidget(QWidget *parent = 0);
    ~HighlightWidget();

protected:
    void    paintEvent(QPaintEvent* event);

private:
    double              opacity;
};
