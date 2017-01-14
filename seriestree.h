#pragma once

#include <QtWidgets/QTreeWidget>

#include <QtGui/QDropEvent>

/// @class SeriesTree
/// @brief Movement policy enforcement for the Series tree
///
/// The Series tree in the SettingsDialog uses this subclass to
/// enforce drag-and-drop movement policies specific to
/// Series/Story relationships.

class SeriesTree : public QTreeWidget
{
public:     // methods
    SeriesTree(QWidget* parent = nullptr);

protected:  // methods
    void    dropEvent(QDropEvent* event);
};
