#pragma once

#include <QtWidgets/QTreeWidget>

#include <QtGui/QDropEvent>

/// @class StylesTree
/// @brief Movement policy enforcement for the Styles tree
///
/// The Styles tree in the SettingsDialog uses this subclass to
/// enforce drag-and-drop movement policies specific to
/// Style relationships.

class StylesTree : public QTreeWidget
{
public:     // methods
    StylesTree(QWidget* parent = nullptr);

protected:  // methods
    void    dropEvent(QDropEvent* event);
};
