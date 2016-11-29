#include <QtCore/QDir>
#include <QtCore/QStringList>

#include "addlocaldialog.h"
#include "ui_addlocaldialog.h"

// X() macro trick
// https://stackoverflow.com/questions/201593/is-there-a-simple-way-to-convert-c-enum-to-string#201792

static const QStringList animentrytype_str
{
#define X(a) QObject::tr( #a ),
#include "animentrytype.def"
#undef X
};

static const QStringList animexittype_str
{
#define X(a) QObject::tr( #a ),
#include "animexittype.def"
#undef X
};

AddLocalDialog::AddLocalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddLocalDialog)
{
    ui->setupUi(this);

    ui->combo_EntryType->clear();
    ui->combo_EntryType->addItems(animentrytype_str);
    ui->combo_ExitType->clear();
    ui->combo_ExitType->addItems(animexittype_str);

    connect(ui->combo_FontFamily, &QFontComboBox::currentFontChanged, this, &AddLocalDialog::slot_update_font);
    connect(ui->combo_FontSize, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddLocalDialog::slot_update_font_size);
}

AddLocalDialog::~AddLocalDialog()
{
    delete ui;
}

void AddLocalDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    activateWindow();
    raise();
}

void AddLocalDialog::set_target(const QString& name)
{
    ui->label_Entity->setText(QDir::toNativeSeparators(name));
}

void AddLocalDialog::set_trigger(LocalTrigger trigger_type)
{
    ui->combo_LocalTrigger->setCurrentIndex(static_cast<int>(trigger_type));
}

void AddLocalDialog::set_ttl(uint ttl)
{
    ui->edit_TTL->setText(QString::number(ttl));
}

void AddLocalDialog::set_font(const QFont& font)
{
    ui->combo_FontFamily->setCurrentFont(font);
    slot_update_font(font);
}

void AddLocalDialog::set_always_visible(bool visible)
{
    ui->check_KeepOnTop->setChecked(visible);
}

void AddLocalDialog::set_screens(int primary_screen, int screen_count)
{
    QVector<QRadioButton*> buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };
    for(int i = 0; i < 4;++i)
    {
        if((i + 1) > screen_count)
            buttons[i]->setDisabled(true);
       if(i == primary_screen)
           buttons[i]->setChecked(true);
    }

    if(screen_count == 1)
        ui->radio_Monitor1->setDisabled(true);
}

void AddLocalDialog::set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type)
{
    ui->combo_EntryType->setCurrentIndex(static_cast<int>(entry_type));
    ui->combo_ExitType->setCurrentIndex(static_cast<int>(exit_type));
}

void AddLocalDialog::set_stacking(ReportStacking stack_type)
{
    ui->radio_Stacked->setChecked(stack_type == ReportStacking::Stacked);
    ui->radio_Intermixed->setChecked(stack_type == ReportStacking::Intermixed);
}

LocalTrigger AddLocalDialog::get_trigger()
{
    return static_cast<LocalTrigger>(ui->combo_LocalTrigger->currentIndex());
}

uint AddLocalDialog::get_ttl()
{
    QString value = ui->edit_TTL->text();
    if(value.isEmpty())
        return ui->edit_TTL->placeholderText().toInt();
    return value.toInt();
}

QFont AddLocalDialog::get_font()
{
    QFont f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    return f;
}

bool AddLocalDialog::get_always_visible()
{
    return ui->check_KeepOnTop->isChecked();
}

int AddLocalDialog::get_display()
{
    QVector<QRadioButton*> buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };
    for(int i = 0; i < 4;++i)
    {
        if(buttons[i]->isChecked())
            return i;
    }

    return 0;
}

AnimEntryType AddLocalDialog::get_animation_entry_type()
{
    return static_cast<AnimEntryType>(ui->combo_EntryType->currentIndex());
}

AnimExitType AddLocalDialog::get_animation_exit_type()
{
    return static_cast<AnimExitType>(ui->combo_ExitType->currentIndex());
}

ReportStacking AddLocalDialog::get_stacking()
{
    return ui->radio_Stacked->isChecked() ? ReportStacking::Stacked : ReportStacking::Intermixed;
}

// font selection changed
void AddLocalDialog::slot_update_font(const QFont& font)
{
    ui->combo_FontSize->clear();

    QFontDatabase font_db;
    foreach(int point, font_db.smoothSizes(font.family(), font.styleName()))
        ui->combo_FontSize->addItem(QString::number(point));
    ui->combo_FontSize->setCurrentIndex(ui->combo_FontSize->findText(QString::number(font.pointSize())));

    QFont f = font;
    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    ui->label_FontExample->setFont(f);
}

// font size changed
void AddLocalDialog::slot_update_font_size(int index)
{
    QFont f = ui->combo_FontFamily->currentFont();
    f.setPointSize(ui->combo_FontSize->itemText(index).toInt());
    ui->label_FontExample->setFont(f);
}
