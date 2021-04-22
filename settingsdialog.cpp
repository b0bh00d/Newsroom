#include <QtWidgets/QMessageBox>

#include "mainwindow.h"
#include "editheadlinedialog.h"
#include "addstorydialog.h"
#include "editseriesdialog.h"

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

extern MainWindow* mainwindow;

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->tree_Series, &QTreeWidget::itemDoubleClicked, this, &SettingsDialog::slot_rename_series);
    connect(ui->tree_Series, &QTreeWidget::itemChanged, this, &SettingsDialog::slot_series_renamed);

    connect(ui->combo_FontFamily, &QFontComboBox::currentFontChanged, this, &SettingsDialog::slot_update_font);
    connect(ui->combo_FontSize, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::slot_update_font_size);
    connect(ui->button_AddStyle, &QPushButton::clicked, this, &SettingsDialog::slot_add_style);
    connect(ui->button_DeleteStyle, &QPushButton::clicked, this, &SettingsDialog::slot_delete_style);
    connect(ui->button_EditStyle, &QPushButton::clicked, this, &SettingsDialog::slot_edit_style);
    connect(ui->tree_Series, &QTreeWidget::itemSelectionChanged, this, &SettingsDialog::slot_story_selection_changed);
    connect(ui->button_EditStory, &QPushButton::clicked, this, &SettingsDialog::slot_edit_story);
    connect(ui->button_StartCoverage, &QPushButton::clicked, this, &SettingsDialog::slot_start_coverage);
    connect(ui->button_StopCoverage, &QPushButton::clicked, this, &SettingsDialog::slot_stop_coverage);
    connect(ui->button_StartCoverageAll, &QPushButton::clicked, this, &SettingsDialog::slot_start_coverage_all);
    connect(ui->button_StopCoverageAll, &QPushButton::clicked, this, &SettingsDialog::slot_stop_coverage_all);
    connect(ui->button_RemoveStory, &QPushButton::clicked, this, &SettingsDialog::slot_remove_story);
    connect(ui->button_RemoveStoryAll, &QPushButton::clicked, this, &SettingsDialog::slot_remove_story_all);
    connect(ui->button_EditSeries, &QPushButton::clicked, this, &SettingsDialog::slot_edit_series);
    connect(ui->tree_Styles, &QTreeWidget::itemSelectionChanged, this, &SettingsDialog::slot_apply_stylesheet);

    ui->button_EditStory->setEnabled(false);

    setWindowTitle(tr("Newsroom: Settings"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));

    slot_story_selection_changed();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::set_autostart(bool autostart)
{
    ui->check_StartAutomatically->setChecked(autostart);
}

void SettingsDialog::set_continue_coverage(bool continue_coverage)
{
    ui->check_ContinueCoverage->setChecked(continue_coverage);
}

void SettingsDialog::set_autostart_coverage(bool autostart)
{
    ui->check_AutoStartCoverage->setChecked(autostart);
}

void SettingsDialog::set_font(const QFont& font)
{
    QSignalBlocker blocker(ui->combo_FontFamily);

    ui->combo_FontFamily->setCurrentFont(font);
    slot_update_font(font);
}

void SettingsDialog::set_styles(const HeadlineStyleList& style_list)
{
    while(ui->tree_Styles->topLevelItemCount())
        delete ui->tree_Styles->takeTopLevelItem(0);

    ui->tree_Styles->setDragEnabled(true);
    ui->tree_Styles->viewport()->setAcceptDrops(true);
    ui->tree_Styles->setDropIndicatorShown(true);
    ui->tree_Styles->setDefaultDropAction(Qt::TargetMoveAction);
    ui->tree_Styles->setDragDropMode(QAbstractItemView::InternalMove);

    foreach(const auto& style, style_list)
    {
        auto items = ui->tree_Styles->findItems(style.name, Qt::MatchExactly, 0);
        if(items.isEmpty())
            new QTreeWidgetItem(ui->tree_Styles, QStringList() << style.name << style.triggers.join(", ") << style.stylesheet);
        else
        {
            items[0]->setText(0, style.name);
            items[0]->setText(1, style.triggers.join(", "));
            items[0]->setText(2, style.stylesheet);
        }
    }

    for(auto i = 0;i < ui->tree_Styles->columnCount();++i)
        ui->tree_Styles->resizeColumnToContents(i);

    ui->tree_Styles->topLevelItem(0)->setSelected(true);
}

void SettingsDialog::set_series(const SeriesInfoList& series_ordered)
{
    while(ui->tree_Series->topLevelItemCount())
        delete ui->tree_Series->takeTopLevelItem(0);

    ui->tree_Series->setDragEnabled(true);
    ui->tree_Series->viewport()->setAcceptDrops(true);
    ui->tree_Series->setDropIndicatorShown(true);
    ui->tree_Series->setDefaultDropAction(Qt::TargetMoveAction);
    ui->tree_Series->setDragDropMode(QAbstractItemView::InternalMove);

    foreach(auto series_info, series_ordered)
    {
        auto series_item = new QTreeWidgetItem(ui->tree_Series, QStringList() << series_info->name);

        QVariant v;
        v.setValue(series_info);
        series_item->setData(0, Qt::UserRole, v);

        series_item->setFlags(series_item->flags() | Qt::ItemIsEditable);

        foreach(auto producer, series_info->producers)
        {
            auto story_info = producer->get_story();

            auto story_item = new QTreeWidgetItem(series_item, QStringList() << "" << story_info->angle);
            story_item->setData(0, Qt::UserRole, story_info->identity);

            producers[story_info->identity] = producer;

            if(producer->is_covering_story())
                story_item->setIcon(0, QIcon(":/images/Covering.png"));
            else
                story_item->setIcon(0, QIcon(":/images/NotCovering.png"));
        }

        series_item->setExpanded(true);
    }

    for(auto i = 0;i < ui->tree_Series->columnCount();++i)
        ui->tree_Series->resizeColumnToContents(i);

    slot_story_selection_changed();
}

bool SettingsDialog::get_autostart() const
{
    return ui->check_StartAutomatically->isChecked();
}

bool SettingsDialog::get_continue_coverage() const
{
    return ui->check_ContinueCoverage->isChecked();
}

bool SettingsDialog::get_autostart_coverage() const
{
    return ui->check_AutoStartCoverage->isChecked();
}

QFont SettingsDialog::get_font()
{
    auto f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    return f;
}

void SettingsDialog::get_styles(HeadlineStyleList& style_list)
{
    style_list.clear();
    for(auto i = 0;i < ui->tree_Styles->topLevelItemCount();++i)
    {
        auto item = ui->tree_Styles->topLevelItem(i);

        HeadlineStyle hs;
        hs.name = item->text(0);
        foreach(const auto& trigger, item->text(1).split(","))
            hs.triggers << trigger.trimmed();
        hs.stylesheet = item->text(2);

        style_list.append(hs);
    }
}

SeriesInfoList SettingsDialog::get_series()
{
    SeriesInfoList sl;
    for(auto i = 0;i < ui->tree_Series->topLevelItemCount();++i)
    {
        auto series_item = ui->tree_Series->topLevelItem(i);

        if(series_item->childCount() || !series_item->text(0).compare("Default"))
        {
            auto si = series_item->data(0, Qt::UserRole).value<SeriesInfoPointer>();
            Q_ASSERT(!si.isNull());

            si->producers.clear();
            si->name = series_item->text(0);

            for(auto j = 0;j < series_item->childCount();++j)
            {
                auto story_item = series_item->child(j);
                auto identity = story_item->data(0, Qt::UserRole).toString();
                si->producers.append(producers[identity]);
            }

            sl.append(si);
        }
    }

    return sl;
}

// font selection changed
void SettingsDialog::slot_update_font(const QFont& font)
{
    QSignalBlocker blocker(ui->combo_FontSize);

    ui->combo_FontSize->clear();

    QFontDatabase font_db;
    foreach(auto point, font_db.smoothSizes(font.family(), font.styleName()))
        ui->combo_FontSize->addItem(QString::number(point));
    ui->combo_FontSize->setCurrentIndex(ui->combo_FontSize->findText(QString::number(font.pointSize())));

    auto f = font;
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    ui->label_HeadlineExample->setFont(f);
}

// font size changed
void SettingsDialog::slot_update_font_size(int index)
{
    auto f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(index).toInt());
    ui->label_HeadlineExample->setFont(f);
}

void SettingsDialog::slot_add_style()
{
    auto item = ui->tree_Styles->topLevelItem(0);       // "Default"

    EditHeadlineDialog dlg;
    mainwindow->restore_window_data(&dlg);

    auto f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    dlg.set_style_font(f);
    dlg.set_style_stylesheet(item->text(2));
    QStringList triggers;
    foreach(const auto& trigger, item->text(1).split(","))
        triggers << trigger.trimmed();
    dlg.set_style_triggers(triggers);

    if(dlg.exec() == QDialog::Accepted)
    {
        QStringList elements;
        elements << dlg.get_style_name();
        elements << dlg.get_style_triggers().join(", ");
        elements << dlg.get_style_stylesheet();

        new QTreeWidgetItem(ui->tree_Styles, elements);

        for(auto i = 0;i < ui->tree_Styles->columnCount();++i)
            ui->tree_Styles->resizeColumnToContents(i);
    }

    mainwindow->save_window_data(&dlg);
}

void SettingsDialog::slot_delete_style()
{
    auto item = ui->tree_Styles->selectedItems()[0];
    delete ui->tree_Styles->takeTopLevelItem(ui->tree_Styles->indexOfTopLevelItem(item));
}

void SettingsDialog::slot_edit_style()
{
    auto item = ui->tree_Styles->selectedItems()[0];

    EditHeadlineDialog dlg;
    mainwindow->restore_window_data(&dlg);

    dlg.set_style_name(item->text(0));
    auto f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    dlg.set_style_font(f);
    dlg.set_style_stylesheet(item->text(2));
    QStringList triggers;
    foreach(const auto& trigger, item->text(1).split(","))
        triggers << trigger.trimmed();
    dlg.set_style_triggers(triggers);

    if(dlg.exec() == QDialog::Accepted)
    {
        item->setText(0, dlg.get_style_name());
        item->setText(1, dlg.get_style_triggers().join(", "));
        item->setText(2, dlg.get_style_stylesheet());

        for(auto i = 0;i < ui->tree_Styles->columnCount();++i)
            ui->tree_Styles->resizeColumnToContents(i);
    }

    mainwindow->save_window_data(&dlg);
}

void SettingsDialog::slot_apply_stylesheet()
{
    auto item = ui->tree_Styles->selectedItems()[0];
    ui->label_HeadlineExample->setStyleSheet(item->text(2));
}

void SettingsDialog::slot_story_selection_changed()
{
    ui->tab_Series->setEnabled(ui->tree_Series->topLevelItemCount() != 0);
    auto selections = ui->tree_Series->selectedItems();

    auto series_selected{0};
    auto stories_selected{0};
    foreach(const auto item, selections)
    {
        if(ui->tree_Series->indexOfTopLevelItem(const_cast<QTreeWidgetItem*>(item)) == -1)
            ++stories_selected;
        else
            ++series_selected;
    }

    ui->button_RemoveStory->setEnabled(selections.length() != 0 && !series_selected);

    ui->button_EditStory->setEnabled(false);
    ui->button_EditSeries->setEnabled(false);
    ui->button_StartCoverage->setEnabled(false);
    ui->button_StopCoverage->setEnabled(false);

    if(series_selected && !stories_selected)
    {
        auto covering{0};
        auto not_covering{0};
        for(auto i = 0;i < selections[0]->childCount();++i)
        {
            auto item = selections[0]->child(i);
            auto identity = item->data(0, Qt::UserRole).toString();
            auto producer = producers[identity];
            if(producer->is_covering_story())
                ++covering;
            else
                ++not_covering;
        }

        ui->button_StartCoverage->setEnabled(not_covering != 0);
        ui->button_StopCoverage->setEnabled(covering != 0);

        ui->button_EditSeries->setEnabled(true);
    }
    else if(stories_selected && !series_selected)
    {
        ui->button_EditStory->setEnabled(true);

        auto identity = selections[0]->data(0, Qt::UserRole).toString();
        auto producer = producers[identity];
        ui->button_StartCoverage->setEnabled(!producer->is_covering_story());
        ui->button_StopCoverage->setEnabled(producer->is_covering_story());
    }
}

void SettingsDialog::slot_edit_story()
{
    auto selections = ui->tree_Series->selectedItems();
    emit signal_edit_story(selections[0]->data(0, Qt::UserRole).toString());
}

void SettingsDialog::slot_edit_series()
{
    auto selections = ui->tree_Series->selectedItems();
    auto si = selections[0]->data(0, Qt::UserRole).value<SeriesInfoPointer>();

    // stop this series before we edit its settings
    QBitArray active(selections[0]->childCount());

    for(auto j = 0;j < selections[0]->childCount();++j)
    {
        auto story_item = selections[0]->child(j);
        auto identity = story_item->data(0, Qt::UserRole).toString();
        auto producer = producers[identity];
        active.setBit(j, producer->is_covering_story());
        if(active[j])
        {
            if(producer->stop_covering_story())
                story_item->setIcon(0, QIcon(":/images/NotCovering.png"));
        }
    }

    EditSeriesDialog dlg(this);
    dlg.set_compact_mode(si->compact_mode, si->compact_compression);

    if(dlg.exec() == QDialog::Accepted)
    {
        si->compact_mode = dlg.get_compact_mode(si->compact_compression);

        // update each Story in the series with the new setting

        for(auto j = 0;j < selections[0]->childCount();++j)
        {
            auto story_item = selections[0]->child(j);
            auto identity = story_item->data(0, Qt::UserRole).toString();
            auto producer = producers[identity];
            auto story_info = producer->get_story();
            story_info->dashboard_compact_mode = si->compact_mode;
            story_info->dashboard_compression = si->compact_compression;
        }
    }

    // restart, as indicated
    for(auto j = 0;j < selections[0]->childCount();++j)
    {
        if(!active[j])
            continue;

        auto story_item = selections[0]->child(j);
        auto identity = story_item->data(0, Qt::UserRole).toString();
        auto producer = producers[identity];
        if(producer->start_covering_story())
            story_item->setIcon(0, QIcon(":/images/Covering.png"));
    }
}

void SettingsDialog::start_coverage(QTreeWidgetItem* item)
{
    auto identity = item->data(0, Qt::UserRole).toString();
    auto producer = producers[identity];
    if(!producer->start_covering_story())
    {
        QMessageBox::critical(this,
                              "Newsroom: Error",
                              "The Producer reports this Story cannot be covered!");
    }
    else
        item->setIcon(0, QIcon(":/images/Covering.png"));
}

void SettingsDialog::slot_start_coverage()
{
    auto selections = ui->tree_Series->selectedItems();
    if(selections.count())
    {
        if(ui->tree_Series->indexOfTopLevelItem(selections[0]) != -1)
        {
            for(auto i = 0;i < selections[0]->childCount();++i)
                start_coverage(selections[0]->child(i));
        }
        else
            start_coverage(selections[0]);

        for(auto i = 0;i < ui->tree_Series->columnCount();++i)
            ui->tree_Series->resizeColumnToContents(i);

        slot_story_selection_changed();
    }
}

void SettingsDialog::stop_coverage(QTreeWidgetItem* item)
{
    auto identity = item->data(0, Qt::UserRole).toString();
    auto producer = producers[identity];
    if(producer->is_covering_story())
    {
        if(producer->stop_covering_story())
            item->setIcon(0, QIcon(":/images/NotCovering.png"));
    }
}

void SettingsDialog::slot_stop_coverage()
{
    auto selections = ui->tree_Series->selectedItems();
    if(selections.count())
    {
        if(ui->tree_Series->indexOfTopLevelItem(selections[0]) != -1)
        {
            for(auto i = 0;i < selections[0]->childCount();++i)
                stop_coverage(selections[0]->child(i));
        }
        else
            stop_coverage(selections[0]);

        for(auto i = 0;i < ui->tree_Series->columnCount();++i)
            ui->tree_Series->resizeColumnToContents(i);

        slot_story_selection_changed();
    }
}

void SettingsDialog::slot_start_coverage_all()
{
    auto changed{false};

    for(auto i = 0;i < ui->tree_Series->topLevelItemCount();++i)
    {
        auto series_item = ui->tree_Series->topLevelItem(i);
        for(auto j = 0;j < series_item->childCount();++j)
        {
            auto story_item = series_item->child(j);
            auto identity = story_item->data(0, Qt::UserRole).toString();
            auto producer = producers[identity];
            if(!producer->is_covering_story())
            {
                if(producer->start_covering_story())
                    story_item->setIcon(0, QIcon(":/images/Covering.png"));
                changed = true;
            }
        }
    }

    if(changed)
    {
        for(auto i = 0;i < ui->tree_Series->columnCount();++i)
            ui->tree_Series->resizeColumnToContents(i);

        slot_story_selection_changed();
    }
}

void SettingsDialog::slot_stop_coverage_all()
{
    auto changed{false};

    for(auto i = 0;i < ui->tree_Series->topLevelItemCount();++i)
    {
        auto series_item = ui->tree_Series->topLevelItem(i);
        for(auto j = 0;j < series_item->childCount();++j)
        {
            auto story_item = series_item->child(j);
            auto identity = story_item->data(0, Qt::UserRole).toString();
            auto producer = producers[identity];
            if(producer->is_covering_story())
            {
                if(producer->stop_covering_story())
                    story_item->setIcon(0, QIcon(":/images/NotCovering.png"));
                changed = true;
            }
        }
    }

    if(changed)
    {
        for(auto i = 0;i < ui->tree_Series->columnCount();++i)
            ui->tree_Series->resizeColumnToContents(i);

        slot_story_selection_changed();
    }
}

void SettingsDialog::slot_remove_story()
{
    auto selections = ui->tree_Series->selectedItems();
    QStringList story_angles;
    for(auto iter = selections.begin();iter != selections.end();++iter)
    {
        story_angles << (*iter)->text(1);
        removed_story_identities << (*iter)->data(0, Qt::UserRole).toString();

        auto series = (*iter)->parent();
        delete series->takeChild(series->indexOfChild(*iter));

        if(series->childCount() == 0 && series->text(0).compare("Default"))
            delete ui->tree_Series->takeTopLevelItem(ui->tree_Series->indexOfTopLevelItem(series));
    }

    foreach(const auto& identity, removed_story_identities)
        producers.remove(identity);
}

void SettingsDialog::slot_remove_story_all()
{
    while(ui->tree_Series->topLevelItemCount())
    {
        auto series = ui->tree_Series->takeTopLevelItem(0);
        while(series->childCount())
        {
            auto child = series->child(0);
            removed_story_identities << child->data(0, Qt::UserRole).toString();

            delete series->takeChild(0);
        }
        delete ui->tree_Series->takeTopLevelItem(0);
    }

    auto new_default = new QTreeWidgetItem(ui->tree_Series, QStringList() << "Default");
    new_default->setFlags(new_default->flags() | Qt::ItemIsEditable);

    producers.clear();
}

void SettingsDialog::slot_rename_series(QTreeWidgetItem* item, int col)
{
    if(col == 0 && ui->tree_Series->indexOfTopLevelItem(item) != -1)
    {
        editing = true;
        original_series_name = item->text(0);
        ui->tree_Series->editItem(item, col);
    }
}

void SettingsDialog::slot_series_renamed(QTreeWidgetItem* item, int col)
{
    if(!editing || col != 0)
        return;
    editing = false;

    auto new_series_name = item->text(0);
    if(!original_series_name.compare(new_series_name))
        return;     // no change

    if(new_series_name.isEmpty())
    {
        // blank names not allowed
        item->setText(0, original_series_name);
        return;
    }

    // make sure it's not a duplicate
    for(auto i = 0;i < ui->tree_Series->topLevelItemCount();++i)
    {
        auto top_item = ui->tree_Series->topLevelItem(i);
        if(top_item == item)
            continue;
        if(!top_item->text(0).compare(new_series_name))
        {
            // dupe!
            item->setText(0, original_series_name);
            return;
        }
    }

    // was this "Default"
    if(!original_series_name.compare("Default"))
    {
        // add a new "Default" entry
        auto new_default = new QTreeWidgetItem(QStringList() << "Default");
        new_default->setFlags(new_default->flags() | Qt::ItemIsEditable);

        auto si = SeriesInfoPointer(new SeriesInfo());
        QVariant v;
        v.setValue(si);
        new_default->setData(0, Qt::UserRole, v);

        ui->tree_Series->insertTopLevelItem(0, new_default);
    }

    for(auto i = 0;i < ui->tree_Series->columnCount();++i)
        ui->tree_Series->resizeColumnToContents(i);
}

// this is from an early iteration of the application; I keep it for the gradient stylesheet refrence
//void SettingsDialog::slot_apply_predefined_style(int index)
//{
//    if(index == 0)
//    {
//        ui->edit_HeadlineStylesheetNormal->setText("color: rgb(255, 255, 255); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 50, 255), stop:1 rgba(0, 0, 255, 255)); border: 1px solid black; border-radius: 10px;");
//        ui->edit_HeadlineStylesheetAlert->setText("color: rgb(255, 255, 255); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(50, 0, 0, 255), stop:1 rgba(255, 0, 0, 255)); border: 1px solid black; border-radius: 10px;");
//    }
//    else if(index == 1)
//    {
//        ui->edit_HeadlineStylesheetNormal->setText("color: rgb(255, 255, 255); background-color: rgb(75, 75, 75); border: 1px solid black;");
//        ui->edit_HeadlineStylesheetAlert->setText("color: rgb(255, 200, 200); background-color: rgb(75, 0, 0); border: 1px solid black;");
//    }

//    slot_apply_normal_stylesheet();
//    slot_apply_alert_stylesheet();
//}
