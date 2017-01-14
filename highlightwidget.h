#pragma once

#include <QWidget>
#include <QPaintEvent>

/// @class HighlightWidget
/// @brief Visual display of a Chyron lane
///
/// This class is used for debugging purposes to visually indicate the lane
/// boundaries of a Chyron.

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
