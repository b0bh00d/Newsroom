#include "seriestree.h"

SeriesTree::SeriesTree(QWidget *parent)
    : QTreeWidget(parent)
{
}

// https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&ved=0ahUKEwib3YPAyrHRAhXqslQKHeo1AAoQFggaMAA&url=http%3A%2F%2Fwww.qtcentre.org%2Fthreads%2F56247-Moving-QTreeWidgetItem-Up-and-Down-in-a-QTreeWidget&usg=AFQjCNHKKC_phZ52t_YmnO-dtZRKqP6WpQ&bvm=bv.142059868,d.cGw&cad=rja
// https://stackoverflow.com/questions/17709221/how-can-i-get-notified-of-internal-item-moves-inside-a-qtreewidget
// https://stackoverflow.com/questions/30291628/internalmove-in-qlistwidget-makes-item-disappear
void SeriesTree::dropEvent(QDropEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    if(!index.isValid())
    {
        // just in case
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    QTreeWidgetItem* destination_item = itemFromIndex(index);
    if(indexOfTopLevelItem(destination_item) == -1)
    {
        // not a Series item
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    // get the list of the items that are about to be dragged
    QList<QTreeWidgetItem*> dragItems = selectedItems();

    // are they moving a Series or a Story?
    bool was_top_level_item = (indexOfTopLevelItem(dragItems[0]) != -1);
    // are they moving a Story to the top of the order in the same parent?
    bool moving_to_top_of_order = (dragItems[0]->parent() == destination_item);

    // the default implementation takes care of the actual move inside the tree
    QTreeWidget::dropEvent(event);

    if(!was_top_level_item)
    {
        if(indexOfTopLevelItem(dragItems[0]) != -1)
        {
            // Stories cannot be top-level items
            takeTopLevelItem(indexOfTopLevelItem(dragItems[0]));
            destination_item->insertChild(0, dragItems[0]);
        }
        else if(moving_to_top_of_order)
        {
            // shift the Story to the top of the order in the same parent
            destination_item->takeChild(destination_item->indexOfChild(dragItems[0]));
            destination_item->insertChild(0, dragItems[0]);
        }
    }
    else
    {
        if(indexOfTopLevelItem(dragItems[0]) == -1)
        {
            // Series cannot be parented to other Series
            QTreeWidgetItem* parent = dragItems[0]->parent();
            parent->takeChild(parent->indexOfChild(dragItems[0]));
            insertTopLevelItem(indexOfTopLevelItem(parent), dragItems[0]);
            dragItems[0]->setExpanded(true);
        }
    }
}
