#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->combo_FontFamily, &QFontComboBox::currentFontChanged, this, &SettingsDialog::slot_update_font);
    connect(ui->combo_FontSize, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::slot_update_font_size);
    connect(ui->list_Stories, &QListWidget::itemSelectionChanged, this, &SettingsDialog::slot_story_update);
    connect(ui->button_RemoveStory, &QPushButton::clicked, this, &SettingsDialog::slot_remove_story);
    connect(ui->edit_HeadlineStylesheetNormal, &QLineEdit::editingFinished, this, &SettingsDialog::slot_apply_normal_stylesheet);
    connect(ui->edit_HeadlineStylesheetAlert, &QLineEdit::editingFinished, this, &SettingsDialog::slot_apply_alert_stylesheet);

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

void SettingsDialog::set_normal_stylesheet(const QString& stylesheet)
{
    ui->edit_HeadlineStylesheetNormal->setText(stylesheet);
    ui->label_HeadlineNormal->setStyleSheet(stylesheet);
}

void SettingsDialog::set_alert_stylesheet(const QString& stylesheet)
{
    ui->edit_HeadlineStylesheetAlert->setText(stylesheet);
    ui->label_HeadlineAlert->setStyleSheet(stylesheet);
}

void SettingsDialog::set_alert_keywords(const QStringList& alert_words)
{
    ui->edit_HeadlineAlertKeywords->setText(alert_words.join(", "));
}

void SettingsDialog::set_stacking(ReportStacking stack_type)
{
    ui->radio_Stacked->setChecked(stack_type == ReportStacking::Stacked);
    ui->radio_Intermixed->setChecked(stack_type == ReportStacking::Intermixed);
}

void SettingsDialog::set_stories(const QList<QUrl>& stories)
{
    while(ui->list_Stories->count())
        delete ui->list_Stories->takeItem(0);

    foreach(const QUrl& story, stories)
    {
        QListWidgetItem* item;
        if(story.isLocalFile())
            item = new QListWidgetItem(story.toLocalFile(), ui->list_Stories);
        else
            item = new QListWidgetItem(story.toString(), ui->list_Stories);
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

QString SettingsDialog::get_normal_stylesheet()
{
    return ui->edit_HeadlineStylesheetNormal->text();
}

QString SettingsDialog::get_alert_stylesheet()
{
    return ui->edit_HeadlineStylesheetAlert->text();
}

QStringList SettingsDialog::get_alert_keywords()
{
    QStringList keywords;
    foreach(const QString& kw, ui->edit_HeadlineAlertKeywords->text().split(QChar(',')))
        keywords << kw.trimmed();
    return keywords;
}

ReportStacking SettingsDialog::get_stacking()
{
    return ui->radio_Stacked->isChecked() ? ReportStacking::Stacked : ReportStacking::Intermixed;
}

QList<QUrl> SettingsDialog::get_stories()
{
    QList<QUrl> remaining_stories;
    for(int i = 0;i < ui->list_Stories->count();++i)
    {
        QListWidgetItem* item = ui->list_Stories->item(i);
        remaining_stories.append(item->data(Qt::UserRole).toUrl());
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
    ui->label_HeadlineNormal->setFont(f);
    ui->label_HeadlineAlert->setFont(f);
}

// font size changed
void SettingsDialog::slot_update_font_size(int index)
{
    QFont f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(index).toInt());
    ui->label_HeadlineNormal->setFont(f);
    ui->label_HeadlineAlert->setFont(f);
}

void SettingsDialog::slot_story_update()
{
    if(ui->list_Stories->count() == 0)
        ui->group_Stories->setEnabled(false);
    else
    {
        QList<QListWidgetItem *> selections = ui->list_Stories->selectedItems();
        ui->button_RemoveStory->setEnabled(selections.length() != 0);
    }
}

void SettingsDialog::slot_remove_story()
{
    QList<QListWidgetItem *> selections = ui->list_Stories->selectedItems();
    foreach(QListWidgetItem* item, selections)
        delete ui->list_Stories->takeItem(ui->list_Stories->row(item));
}

void SettingsDialog::slot_apply_normal_stylesheet()
{
    ui->label_HeadlineNormal->setStyleSheet(ui->edit_HeadlineStylesheetNormal->text());
}

void SettingsDialog::slot_apply_alert_stylesheet()
{
    ui->label_HeadlineAlert->setStyleSheet(ui->edit_HeadlineStylesheetAlert->text());
}
