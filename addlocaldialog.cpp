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

    setWindowTitle(tr("Newsroom: Add Story"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));

    connect(ui->combo_EntryType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddLocalDialog::slot_entry_type_changed);
    connect(ui->check_TrainFixedWidth, &QCheckBox::clicked, this, &AddLocalDialog::slot_train_fixed_width_clicked);
    connect(ui->radio_TrainReduceOpacityFixed, &QCheckBox::clicked, this, &AddLocalDialog::slot_train_reduce_opacity_clicked);
    connect(ui->radio_TrainReduceOpacityByAge, &QCheckBox::clicked, this, &AddLocalDialog::slot_train_reduce_opacity_clicked);
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

    slot_entry_type_changed(ui->combo_EntryType->currentIndex());
}

void AddLocalDialog::set_train_fixed_width(int width)
{
    ui->check_TrainFixedWidth->setEnabled(width != 0);
    if(width)
        ui->edit_TrainFixedWidth->setText(QString::number(width));
}

void AddLocalDialog::set_train_age_effects(AgeEffects effect, int percent)
{
    ui->edit_TrainFixedWidth->clear();

    ui->group_AgeEffects->setEnabled(effect != AgeEffects::None);
    ui->radio_TrainReduceOpacityFixed->setEnabled(effect == AgeEffects::ReduceOpacityFixed);
    if(percent)
        ui->edit_TrainReduceOpacity->setText(QString::number(percent));
    ui->edit_TrainReduceOpacity->setEnabled(effect == AgeEffects::ReduceOpacityFixed);
    ui->radio_TrainReduceOpacityFixed->setEnabled(effect == AgeEffects::ReduceOpacityByAge);
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

int AddLocalDialog::get_train_fixed_width()
{
    if(!ui->check_TrainFixedWidth->isChecked() || ui->edit_TrainFixedWidth->text().isEmpty())
        return 0;
    return ui->edit_TrainFixedWidth->text().toInt();
}

AgeEffects AddLocalDialog::get_train_age_effects(int& percent)
{
    if(!ui->group_AgeEffects->isEnabled())
        return AgeEffects::None;

    percent = ui->edit_TrainReduceOpacity->text().toInt();
    return (ui->radio_TrainReduceOpacityFixed->isChecked() ? AgeEffects::ReduceOpacityFixed : AgeEffects::ReduceOpacityByAge);
}

void AddLocalDialog::slot_entry_type_changed(int index)
{
    bool is_train = ui->combo_EntryType->itemText(index).startsWith(tr("Train"));
    ui->combo_ExitType->setEnabled(!is_train);
    ui->group_Train->setEnabled(is_train);

//    if(is_train)
//    {
//        slot_train_fixed_width_clicked(ui->check_TrainFixedWidth->isChecked());
//        slot_train_reduce_opacity_clicked(ui->check_TrainReduceOpacity->isChecked());
//    }
}

void AddLocalDialog::slot_train_fixed_width_clicked(bool checked)
{
    ui->edit_TrainFixedWidth->setEnabled(checked);
}

void AddLocalDialog::slot_train_reduce_opacity_clicked(bool /*checked*/)
{
    ui->edit_TrainReduceOpacity->setEnabled(ui->radio_TrainReduceOpacityFixed->isChecked());
}
