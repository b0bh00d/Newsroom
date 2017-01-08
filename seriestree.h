#pragma once

#include <QtWidgets/QTreeWidget>

#include <QtGui/QDropEvent>

class SeriesTree : public QTreeWidget
{
public:     // methods
    SeriesTree(QWidget* parent = nullptr);

protected:  // methods
    void    dropEvent(QDropEvent* event);
};
