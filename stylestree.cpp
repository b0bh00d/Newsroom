#include "stylestree.h"

StylesTree::StylesTree(QWidget *parent)
    : QTreeWidget(parent)
{
}

void StylesTree::dropEvent(QDropEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    if(!index.isValid())
    {
        // just in case
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    QTreeWidgetItem* destination_item = itemFromIndex(index);
    int index_of_destination = indexOfTopLevelItem(destination_item);
    if(index_of_destination <= 0)
    {
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    // get the list of the items that are about to be dragged
    QList<QTreeWidgetItem*> dragItems = selectedItems();
    if(indexOfTopLevelItem(dragItems[0]) == 0)
    {
        // the "Default" item cannot be moved
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    // the default implementation takes care of the actual move inside the tree
    QTreeWidget::dropEvent(event);

    if(indexOfTopLevelItem(dragItems[0]) == -1)
    {
        // Styles cannot be parented to other Styles
        QTreeWidgetItem* parent = dragItems[0]->parent();
        parent->takeChild(parent->indexOfChild(dragItems[0]));
        insertTopLevelItem(indexOfTopLevelItem(parent), dragItems[0]);
    }
}
