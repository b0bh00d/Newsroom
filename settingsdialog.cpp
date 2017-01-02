#include <QtWidgets/QMessageBox>

#include "mainwindow.h"
#include "editheadlinedialog.h"
#include "addstorydialog.h"

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

extern MainWindow* mainwindow;

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->combo_FontFamily, &QFontComboBox::currentFontChanged, this, &SettingsDialog::slot_update_font);
    connect(ui->combo_FontSize, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::slot_update_font_size);
    connect(ui->button_AddStyle, &QPushButton::clicked, this, &SettingsDialog::slot_add_style);
    connect(ui->button_DeleteStyle, &QPushButton::clicked, this, &SettingsDialog::slot_delete_style);
    connect(ui->button_EditStyle, &QPushButton::clicked, this, &SettingsDialog::slot_edit_style);
    connect(ui->tree_Stories, &QTreeWidget::itemSelectionChanged, this, &SettingsDialog::slot_story_selection_changed);
    connect(ui->button_EditStory, &QPushButton::clicked, this, &SettingsDialog::slot_edit_story);
    connect(ui->button_StartCoverage, &QPushButton::clicked, this, &SettingsDialog::slot_start_coverage);
    connect(ui->button_StopCoverage, &QPushButton::clicked, this, &SettingsDialog::slot_stop_coverage);
    connect(ui->button_StartCoverageAll, &QPushButton::clicked, this, &SettingsDialog::slot_start_coverage_all);
    connect(ui->button_StopCoverageAll, &QPushButton::clicked, this, &SettingsDialog::slot_stop_coverage_all);
    connect(ui->button_RemoveStory, &QPushButton::clicked, this, &SettingsDialog::slot_remove_story);
    connect(ui->button_RemoveStoryAll, &QPushButton::clicked, this, &SettingsDialog::slot_remove_story_all);
    connect(ui->tree_Styles, &QTreeWidget::itemSelectionChanged, this, &SettingsDialog::slot_apply_stylesheet);

    ui->button_EditStory->setEnabled(false);

    connect(ui->check_CompactMode, &QCheckBox::clicked, this, &SettingsDialog::slot_compact_mode_clicked);
    ui->edit_ZoomOutPercent->setValidator(new QIntValidator(5, 30));
    slot_compact_mode_clicked(true);

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

void SettingsDialog::set_compact_mode(bool compact_mode, int zoom_percent)
{
    ui->check_CompactMode->setChecked(compact_mode);
    if(zoom_percent != ui->edit_ZoomOutPercent->placeholderText().toInt())
    {
        if(zoom_percent >= 5 && zoom_percent <= 30)
            ui->edit_ZoomOutPercent->setText(QString::number(zoom_percent));
    }

    slot_compact_mode_clicked(compact_mode);
}

void SettingsDialog::set_font(const QFont& font)
{
    ui->combo_FontFamily->setCurrentFont(font);
    slot_update_font(font);
}

void SettingsDialog::set_styles(const HeadlineStyleList& style_list)
{
    while(ui->tree_Styles->topLevelItemCount())
        delete ui->tree_Styles->takeTopLevelItem(0);

//    new QTreeWidgetItem(ui->tree_Styles, QStringList() << "Default" << "" << mainwindow->default_stylesheet);

    foreach(const HeadlineStyle& style, style_list)
    {
        QList<QTreeWidgetItem*> items = ui->tree_Styles->findItems(style.name, Qt::MatchExactly, 0);
        if(items.isEmpty())
            new QTreeWidgetItem(ui->tree_Styles, QStringList() << style.name << style.triggers.join(", ") << style.stylesheet);
        else
        {
            items[0]->setText(0, style.name);
            items[0]->setText(1, style.triggers.join(", "));
            items[0]->setText(2, style.stylesheet);
        }
    }

    for(int i = 0;i < ui->tree_Styles->columnCount();++i)
        ui->tree_Styles->resizeColumnToContents(i);

    ui->tree_Styles->topLevelItem(0)->setSelected(true);
}

void SettingsDialog::set_stories(const QList<QString>& stories, const QList<ProducerPointer> producers)
{
    while(ui->tree_Stories->topLevelItemCount())
        delete ui->tree_Stories->takeTopLevelItem(0);

    for(int i = 0;i < stories.length();++i)
    {
        QVariant v;
        v.setValue(producers[i]);

        QTreeWidgetItem* tree_item = new QTreeWidgetItem(ui->tree_Stories, QStringList() << "" << stories[i]);
        tree_item->setData(0, Qt::UserRole, v);

        if(producers[i]->is_covering_story())
            tree_item->setIcon(0, QIcon(":/images/Covering.png"));
        else
            tree_item->setIcon(0, QIcon(":/images/NotCovering.png"));
    }

    for(int i = 0;i < ui->tree_Stories->columnCount();++i)
        ui->tree_Stories->resizeColumnToContents(i);

    slot_story_selection_changed();
}

bool SettingsDialog::get_autostart()
{
    return ui->check_StartAutomatically->isChecked();
}

bool SettingsDialog::get_continue_coverage()
{
    return ui->check_ContinueCoverage->isChecked();
}

bool SettingsDialog::get_compact_mode(int &zoom_percent)
{
    zoom_percent = ui->edit_ZoomOutPercent->placeholderText().toInt();
    if(ui->check_CompactMode->isChecked() && !ui->edit_ZoomOutPercent->text().isEmpty())
        zoom_percent = ui->edit_ZoomOutPercent->text().toInt();
    return ui->check_CompactMode->isChecked();
}

QFont SettingsDialog::get_font()
{
    QFont f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    return f;
}

void SettingsDialog::get_styles(HeadlineStyleList& style_list)
{
    style_list.clear();
    for(int i = 0;i < ui->tree_Styles->topLevelItemCount();++i)
    {
        QTreeWidgetItem* item = ui->tree_Styles->topLevelItem(i);

        HeadlineStyle hs;
        hs.name = item->text(0);
        foreach(const QString& trigger, item->text(1).split(","))
            hs.triggers << trigger.trimmed();
        hs.stylesheet = item->text(2);

        style_list.append(hs);
    }
}

QList<QString> SettingsDialog::get_stories()
{
    QList<QString> remaining_stories;
    for(int i = 0;i < ui->tree_Stories->topLevelItemCount();++i)
    {
        QTreeWidgetItem* item = ui->tree_Stories->topLevelItem(i);
        remaining_stories.append(item->text(1));
    }

    return remaining_stories;
}

// font selection changed
void SettingsDialog::slot_update_font(const QFont& font)
{
    ui->combo_FontSize->clear();

    QFontDatabase font_db;
    foreach(int point, font_db.smoothSizes(font.family(), font.styleName()))
        ui->combo_FontSize->addItem(QString::number(point));
    ui->combo_FontSize->setCurrentIndex(ui->combo_FontSize->findText(QString::number(font.pointSize())));

    QFont f = font;
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    ui->label_HeadlineExample->setFont(f);
}

// font size changed
void SettingsDialog::slot_update_font_size(int index)
{
    QFont f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(index).toInt());
    ui->label_HeadlineExample->setFont(f);
}

void SettingsDialog::slot_add_style()
{
    QTreeWidgetItem* item = ui->tree_Styles->topLevelItem(0);       // "Default"

    EditHeadlineDialog dlg;
    mainwindow->restore_window_data(&dlg);

    QFont f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    dlg.set_style_font(f);
    dlg.set_style_stylesheet(item->text(2));
    QStringList triggers;
    foreach(const QString& trigger, item->text(1).split(","))
        triggers << trigger.trimmed();
    dlg.set_style_triggers(triggers);

    if(dlg.exec() == QDialog::Accepted)
    {
        QStringList elements;
        elements << dlg.get_style_name();
        elements << dlg.get_style_triggers().join(", ");
        elements << dlg.get_style_stylesheet();

        new QTreeWidgetItem(ui->tree_Styles, elements);

        for(int i = 0;i < ui->tree_Styles->columnCount();++i)
            ui->tree_Styles->resizeColumnToContents(i);
    }

    mainwindow->save_window_data(&dlg);
}

void SettingsDialog::slot_delete_style()
{
    QTreeWidgetItem* item = ui->tree_Styles->selectedItems()[0];
    delete ui->tree_Styles->takeTopLevelItem(ui->tree_Styles->indexOfTopLevelItem(item));
}

void SettingsDialog::slot_edit_style()
{
    QTreeWidgetItem* item = ui->tree_Styles->selectedItems()[0];

    EditHeadlineDialog dlg;
    mainwindow->restore_window_data(&dlg);

    dlg.set_style_name(item->text(0));
    QFont f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    dlg.set_style_font(f);
    dlg.set_style_stylesheet(item->text(2));
    QStringList triggers;
    foreach(const QString& trigger, item->text(1).split(","))
        triggers << trigger.trimmed();
    dlg.set_style_triggers(triggers);

    if(dlg.exec() == QDialog::Accepted)
    {
        item->setText(0, dlg.get_style_name());
        item->setText(1, dlg.get_style_triggers().join(", "));
        item->setText(2, dlg.get_style_stylesheet());

        for(int i = 0;i < ui->tree_Styles->columnCount();++i)
            ui->tree_Styles->resizeColumnToContents(i);
    }

    mainwindow->save_window_data(&dlg);
}

void SettingsDialog::slot_apply_stylesheet()
{
    QTreeWidgetItem* item = ui->tree_Styles->selectedItems()[0];
    ui->label_HeadlineExample->setStyleSheet(item->text(2));
}

void SettingsDialog::slot_story_selection_changed()
{
    ui->tab_Stories->setEnabled(ui->tree_Stories->topLevelItemCount() != 0);
    QList<QTreeWidgetItem *> selections = ui->tree_Stories->selectedItems();
    ui->button_RemoveStory->setEnabled(selections.length() != 0);

    ui->button_StartCoverage->setEnabled(false);
    ui->button_StopCoverage->setEnabled(false);

    if(selections.count())
    {
        ui->button_EditStory->setEnabled(true);

        ProducerPointer producer = selections[0]->data(0, Qt::UserRole).value<ProducerPointer>();
        ui->button_StartCoverage->setEnabled(!producer->is_covering_story());
        ui->button_StopCoverage->setEnabled(producer->is_covering_story());
    }
}

void SettingsDialog::slot_edit_story()
{
    QList<QTreeWidgetItem *> selections = ui->tree_Stories->selectedItems();
    // BUG: The story ID might change here, leaving the old
    // entry in the tree
    emit signal_edit_story(selections[0]->text(1));
}

void SettingsDialog::slot_start_coverage()
{
    QList<QTreeWidgetItem *> selections = ui->tree_Stories->selectedItems();
    if(selections.count())
    {
        ProducerPointer producer = selections[0]->data(0, Qt::UserRole).value<ProducerPointer>();
        if(!producer->start_covering_story())
        {
            QMessageBox::critical(this,
                                  "Newsroom: Error",
                                  "The Producer reports this Story cannot be covered!");
        }
        else
        {
            selections[0]->setIcon(0, QIcon(":/images/Covering.png"));

            for(int i = 0;i < ui->tree_Stories->columnCount();++i)
                ui->tree_Stories->resizeColumnToContents(i);

            slot_story_selection_changed();
        }
    }
}

void SettingsDialog::slot_stop_coverage()
{
    QList<QTreeWidgetItem *> selections = ui->tree_Stories->selectedItems();
    if(selections.count())
    {
        ProducerPointer producer = selections[0]->data(0, Qt::UserRole).value<ProducerPointer>();
        if(producer->is_covering_story())
        {
            if(producer->stop_covering_story())
            {
                selections[0]->setIcon(0, QIcon(":/images/NotCovering.png"));

                for(int i = 0;i < ui->tree_Stories->columnCount();++i)
                    ui->tree_Stories->resizeColumnToContents(i);

                slot_story_selection_changed();
            }
        }
    }
}

void SettingsDialog::slot_start_coverage_all()
{
    bool changed = false;

    for(int i = 0;i < ui->tree_Stories->topLevelItemCount();++i)
    {
        QTreeWidgetItem* item = ui->tree_Stories->topLevelItem(i);
        ProducerPointer producer = item->data(0, Qt::UserRole).value<ProducerPointer>();
        if(!producer->is_covering_story())
        {
            if(producer->start_covering_story())
                item->setIcon(0, QIcon(":/images/Covering.png"));
            changed = true;
        }
    }

    if(changed)
    {
        for(int i = 0;i < ui->tree_Stories->columnCount();++i)
            ui->tree_Stories->resizeColumnToContents(i);

        slot_story_selection_changed();
    }
}

void SettingsDialog::slot_stop_coverage_all()
{
    bool changed = false;

    for(int i = 0;i < ui->tree_Stories->topLevelItemCount();++i)
    {
        QTreeWidgetItem* item = ui->tree_Stories->topLevelItem(i);
        ProducerPointer producer = item->data(0, Qt::UserRole).value<ProducerPointer>();
        if(producer->is_covering_story())
        {
            if(producer->stop_covering_story())
                item->setIcon(0, QIcon(":/images/NotCovering.png"));
            changed = true;
        }
    }

    if(changed)
    {
        for(int i = 0;i < ui->tree_Stories->columnCount();++i)
            ui->tree_Stories->resizeColumnToContents(i);

        slot_story_selection_changed();
    }
}

void SettingsDialog::slot_remove_story()
{
    QList<QTreeWidgetItem *> selections = ui->tree_Stories->selectedItems();
    foreach(QTreeWidgetItem* item, selections)
        delete ui->tree_Stories->takeTopLevelItem(ui->tree_Stories->indexOfTopLevelItem(item));
}

void SettingsDialog::slot_remove_story_all()
{
    while(ui->tree_Stories->topLevelItemCount())
        delete ui->tree_Stories->takeTopLevelItem(0);
}

void SettingsDialog::slot_compact_mode_clicked(bool /*checked*/)
{
    ui->label_ZoomOut1->setEnabled(ui->check_CompactMode->isChecked());
    ui->edit_ZoomOutPercent->setEnabled(ui->check_CompactMode->isChecked());
    ui->label_ZoomOut2->setEnabled(ui->check_CompactMode->isChecked());
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
