#include "mainwindow.h"
#include "editheadlinedialog.h"

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
    connect(ui->list_Stories, &QListWidget::itemSelectionChanged, this, &SettingsDialog::slot_story_update);
    connect(ui->button_RemoveStory, &QPushButton::clicked, this, &SettingsDialog::slot_remove_story);
    connect(ui->tree_Styles, &QTreeWidget::itemSelectionChanged, this, &SettingsDialog::slot_apply_stylesheet);

    setWindowTitle(tr("Newsroom: Settings"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));

    slot_story_update();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::set_autostart(bool autostart)
{
    ui->check_StartAutomatically->setChecked(autostart);
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

void SettingsDialog::set_stacking(ReportStacking stack_type)
{
    ui->radio_Stacked->setChecked(stack_type == ReportStacking::Stacked);
    ui->radio_Intermixed->setChecked(stack_type == ReportStacking::Intermixed);
}

void SettingsDialog::set_stories(const QList<QString>& stories)
{
    while(ui->list_Stories->count())
        delete ui->list_Stories->takeItem(0);

    foreach(const QString& story, stories)
    {
        QListWidgetItem* item = new QListWidgetItem(story, ui->list_Stories);
        item->setData(Qt::UserRole, story);
    }

    slot_story_update();
}

bool SettingsDialog::get_autostart()
{
    return ui->check_StartAutomatically->isChecked();
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

ReportStacking SettingsDialog::get_stacking()
{
    return ui->radio_Stacked->isChecked() ? ReportStacking::Stacked : ReportStacking::Intermixed;
}

QList<QString> SettingsDialog::get_stories()
{
    QList<QString> remaining_stories;
    for(int i = 0;i < ui->list_Stories->count();++i)
    {
        QListWidgetItem* item = ui->list_Stories->item(i);
        remaining_stories.append(item->text());
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

void SettingsDialog::slot_story_update()
{
    ui->group_Stories->setEnabled(ui->list_Stories->count() != 0);
    QList<QListWidgetItem *> selections = ui->list_Stories->selectedItems();
    ui->button_RemoveStory->setEnabled(selections.length() != 0);
}

void SettingsDialog::slot_remove_story()
{
    QList<QListWidgetItem *> selections = ui->list_Stories->selectedItems();
    foreach(QListWidgetItem* item, selections)
        delete ui->list_Stories->takeItem(ui->list_Stories->row(item));
}

//void SettingsDialog::slot_apply_alert_stylesheet()
//{
//    ui->label_HeadlineAlert->setStyleSheet(ui->edit_HeadlineStylesheetAlert->text());
//}

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
