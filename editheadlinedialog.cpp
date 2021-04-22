#include "editheadlinedialog.h"
#include "ui_editheadlinedialog.h"

EditHeadlineDialog::EditHeadlineDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditHeadlineDialog)
{
    ui->setupUi(this);

    connect(ui->edit_StyleName, &QLineEdit::textChanged, this, &EditHeadlineDialog::slot_update_ok);
    connect(ui->plain_Stylesheet, &QPlainTextEdit::textChanged, this, &EditHeadlineDialog::slot_update_ok);
    connect(ui->edit_StyleTriggers, &QLineEdit::textChanged, this, &EditHeadlineDialog::slot_update_ok);

    connect(ui->button_ApplyStylesheet, &QPushButton::clicked, this, &EditHeadlineDialog::slot_apply_stylesheet);

    auto buttonBox = findChild<QDialogButtonBox*>("buttonBox");
    auto button = buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(false);
}

EditHeadlineDialog::~EditHeadlineDialog()
{
    delete ui;
}

void EditHeadlineDialog::set_style_name(const QString& name)
{
    ui->edit_StyleName->setText(name);

    ui->edit_StyleName->setEnabled(name.compare("Default"));
    ui->edit_StyleTriggers->setEnabled(name.compare("Default"));

    slot_update_ok();
}

void EditHeadlineDialog::set_style_font(const QFont& font)
{
//    QFont f = font;
//    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
    ui->label_Style->setFont(font);

//    ui->combo_FontFamily->setCurrentFont(font);
//    slot_update_font(font);
}

void EditHeadlineDialog::set_style_stylesheet(const QString& stylesheet)
{
    ui->plain_Stylesheet->setPlainText(stylesheet);
    slot_apply_stylesheet();
}

void EditHeadlineDialog::set_style_triggers(const QStringList& keywords)
{
    ui->edit_StyleTriggers->setText(keywords.join(", "));

    slot_update_ok();
}

QString EditHeadlineDialog::get_style_name() const
{
    return ui->edit_StyleName->text();
}

//QFont EditHeadlineDialog::get_font()
//{
//    QFont f = ui->combo_FontFamily->currentFont();
//    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
//    return f;
//}

QString EditHeadlineDialog::get_style_stylesheet() const
{
    return ui->plain_Stylesheet->toPlainText();
}

QStringList EditHeadlineDialog::get_style_triggers() const
{
    QStringList triggers;
    foreach(const auto& trigger, ui->edit_StyleTriggers->text().split(","))
        triggers << trigger.trimmed();
    return triggers;
}

//// font selection changed
//void EditHeadlineDialog::slot_update_font(const QFont& font)
//{
//    ui->combo_FontSize->clear();

//    QFontDatabase font_db;
//    foreach(int point, font_db.smoothSizes(font.family(), font.styleName()))
//        ui->combo_FontSize->addItem(QString::number(point));
//    ui->combo_FontSize->setCurrentIndex(ui->combo_FontSize->findText(QString::number(font.pointSize())));

//    QFont f = font;
//    f.setPointSize(ui->combo_FontSize->itemText(ui->combo_FontSize->currentIndex()).toInt());
//    ui->label_Style->setFont(f);
//}

//// font size changed
//void EditHeadlineDialog::slot_update_font_size(int index)
//{
//    QFont f = ui->combo_FontFamily->currentFont();
//    f.setPointSize(ui->combo_FontSize->itemText(index).toInt());
//    ui->label_Style->setFont(f);
//}

void EditHeadlineDialog::slot_apply_stylesheet()
{
    ui->label_Style->setStyleSheet(ui->plain_Stylesheet->toPlainText());
}

void EditHeadlineDialog::slot_update_ok()
{
    auto buttonBox = findChild<QDialogButtonBox*>("buttonBox");
    auto button = buttonBox->button(QDialogButtonBox::Ok);

    auto name = ui->edit_StyleName->text();
    auto have_triggers = (!ui->edit_StyleTriggers->text().isEmpty() && !name.isEmpty() && name.compare("Default"));

    button->setEnabled(have_triggers && !ui->plain_Stylesheet->toPlainText().isEmpty());
}
