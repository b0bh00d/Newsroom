#include <QIntValidator>

#include "editseriesdialog.h"
#include "ui_editseriesdialog.h"

EditSeriesDialog::EditSeriesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditSeriesDialog)
{
    ui->setupUi(this);

    connect(ui->check_CompactMode, &QCheckBox::clicked, this, &EditSeriesDialog::slot_compact_mode_clicked);
    ui->edit_ZoomOutPercent->setValidator(new QIntValidator(5, 30));
    slot_compact_mode_clicked(true);
}

EditSeriesDialog::~EditSeriesDialog()
{
    delete ui;
}

void EditSeriesDialog::set_compact_mode(bool compact_mode, int zoom_percent)
{
    ui->check_CompactMode->setChecked(compact_mode);
    if(zoom_percent != ui->edit_ZoomOutPercent->placeholderText().toInt())
    {
        if(zoom_percent >= 5 && zoom_percent <= 30)
            ui->edit_ZoomOutPercent->setText(QString::number(zoom_percent));
    }

    slot_compact_mode_clicked(compact_mode);
}

bool EditSeriesDialog::get_compact_mode(int& zoom_percent) const
{
    zoom_percent = ui->edit_ZoomOutPercent->placeholderText().toInt();
    if(ui->check_CompactMode->isChecked() && !ui->edit_ZoomOutPercent->text().isEmpty())
        zoom_percent = ui->edit_ZoomOutPercent->text().toInt();
    return ui->check_CompactMode->isChecked();
}

void EditSeriesDialog::slot_compact_mode_clicked(bool /*checked*/)
{
    ui->label_ZoomOut1->setEnabled(ui->check_CompactMode->isChecked());
    ui->edit_ZoomOutPercent->setEnabled(ui->check_CompactMode->isChecked());
    ui->label_ZoomOut2->setEnabled(ui->check_CompactMode->isChecked());
}
