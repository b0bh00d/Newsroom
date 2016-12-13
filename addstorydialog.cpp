#include <QtGui/QRegExpValidator>

#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>

#include <iplugin>

#include "addstorydialog.h"
#include "ui_addstorydialog.h"

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

AddStoryDialog::AddStoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddStoryDialog)
{
    ui->setupUi(this);

    ui->combo_EntryType->clear();
    ui->combo_EntryType->addItems(animentrytype_str);
    ui->combo_ExitType->clear();
    ui->combo_ExitType->addItems(animexittype_str);

    setWindowTitle(tr("Newsroom: Add Story"));
    setWindowIcon(QIcon(":/images/Newsroom.png"));

    QRegExp rx("\\d+");
    ui->edit_TTL->setValidator(new QRegExpValidator(rx, this));
    ui->edit_HeadlinesFixedWidth->setValidator(new QRegExpValidator(rx, this));
    ui->edit_HeadlinesFixedHeight->setValidator(new QRegExpValidator(rx, this));
    ui->edit_TrainReduceOpacity->setValidator(new QIntValidator(10, 90, this));

    ui->radio_HeadlinesSizeTextScale->setChecked(true);
    ui->radio_TrainReduceOpacityFixed->setChecked(true);

//    connect(this, &QDialog::accepted, this, &AddStoryDialog::slot_cleanup);
//    connect(this, &QDialog::rejected, this, &AddStoryDialog::slot_cleanup);
//    connect(this, &QDialog::finished, this, &AddStoryDialog::slot_cleanup);
}

AddStoryDialog::~AddStoryDialog()
{
    delete ui;
}

void AddStoryDialog::showEvent(QShowEvent *event)
{
    connect(ui->combo_EntryType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_entry_type_changed);
    connect(ui->combo_AvailableReporters, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_reporter_changed);
    connect(ui->check_HeadlinesFixedSize, &QCheckBox::clicked, this, &AddStoryDialog::slot_headlines_fixed_size_clicked);
    connect(ui->radio_TrainReduceOpacityFixed, &QCheckBox::clicked, this, &AddStoryDialog::slot_train_reduce_opacity_clicked);
    connect(ui->radio_TrainReduceOpacityByAge, &QCheckBox::clicked, this, &AddStoryDialog::slot_train_reduce_opacity_clicked);
    connect(ui->combo_LocalTrigger, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_trigger_changed);
    connect(ui->button_ConfigureReporter, &QPushButton::clicked, this, &AddStoryDialog::slot_configure_reporter);

    QDialog::showEvent(event);
    activateWindow();
    raise();
}

void AddStoryDialog::save_defaults(QSettings* settings)
{
    settings->beginGroup("AddStoryDialog");

    settings->setValue("local_trigger", ui->combo_LocalTrigger->currentIndex());
    settings->setValue("ttl", ui->edit_TTL->text());
    settings->setValue("keep_on_top", ui->check_KeepOnTop->isChecked());
    settings->setValue("display", ui->radio_Monitor1->isChecked() ? 0 : (ui->radio_Monitor2->isChecked() ? 1 : (ui->radio_Monitor3->isChecked() ? 2 : 3)));
    settings->setValue("headlines_fixed_size", ui->check_HeadlinesFixedSize->isChecked());
    settings->setValue("headlines_fixed_width", ui->edit_HeadlinesFixedWidth->text());
    settings->setValue("headlines_fixed_height", ui->edit_HeadlinesFixedHeight->text());
    settings->setValue("fixed_text", ui->radio_HeadlinesSizeTextScale->isChecked() ? 0 : 1);
    settings->setValue("entry_type", ui->combo_EntryType->currentIndex());
    settings->setValue("exit_type", ui->combo_ExitType->currentIndex());
    settings->setValue("exit_type_enabled", ui->combo_ExitType->isEnabled());
    settings->setValue("group_age_effects", ui->group_AgeEffects->isChecked());
    settings->setValue("age_opacity_type", ui->radio_TrainReduceOpacityFixed->isChecked() ? 1 : 2);
    settings->setValue("age_opacity_fixed_percent", ui->edit_TrainReduceOpacity->text());
    settings->setValue("reporter_configuration", reporter_configuration);

    settings->endGroup();
}

void AddStoryDialog::load_defaults(QSettings* settings)
{
    QVector<QRadioButton*> display_buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };
    QVector<QRadioButton*> fixed_buttons { ui->radio_HeadlinesSizeTextScale, ui->radio_HeadlinesSizeTextClip };
    QVector<QRadioButton*> age_buttons { nullptr, ui->radio_TrainReduceOpacityFixed, ui->radio_TrainReduceOpacityByAge };

    settings->beginGroup("AddStoryDialog");

    ui->combo_LocalTrigger->setCurrentIndex(settings->value("local_trigger", static_cast<int>(LocalTrigger::NewContent)).toInt());
    ui->edit_TTL->setText(settings->value("ttl", "").toString());
    ui->check_KeepOnTop->setChecked(settings->value("keep_on_top", true).toBool());
    display_buttons[settings->value("display", 0).toInt()]->setChecked(true);
    ui->check_HeadlinesFixedSize->setChecked(settings->value("headlines_fixed_size", true).toBool());
    if(ui->check_HeadlinesFixedSize->isChecked())
    {
        ui->edit_HeadlinesFixedWidth->setText(QString::number(settings->value("headlines_fixed_width", 350).toInt()));
        ui->edit_HeadlinesFixedHeight->setText(QString::number(settings->value("headlines_fixed_height", 100).toInt()));
    }
    fixed_buttons[settings->value("fixed_text", 0).toInt()]->setChecked(true);
    ui->combo_EntryType->setCurrentIndex(settings->value("entry_type", static_cast<int>(AnimEntryType::SlideDownLeftTop)).toInt());
    ui->combo_ExitType->setCurrentIndex(settings->value("exit_type", static_cast<int>(AnimExitType::SlideLeft)).toInt());
    ui->combo_ExitType->setEnabled(settings->value("exit_type_enabled", true).toBool());
    ui->group_AgeEffects->setChecked(settings->value("group_age_effects", false).toBool());
    age_buttons[settings->value("age_opacity_type", 1).toInt()]->setChecked(true);
    if(ui->radio_TrainReduceOpacityFixed->isChecked())
        ui->edit_TrainReduceOpacity->setText(QString::number(settings->value("age_opacity_fixed_percent", 60).toInt()));
    reporter_configuration = settings->value("reporter_configuration", QStringList()).toStringList();

    settings->endGroup();
}

void AddStoryDialog::set_target(const QUrl& story)
{
    this->story = story;

    if(story.isLocalFile())
    {
        ui->edit_Target->setText(QDir::toNativeSeparators(story.toLocalFile()));
        ui->edit_Target->setEnabled(false);
    }
    else
        ui->edit_Target->setText(story.toString());
}

void AddStoryDialog::set_trigger(LocalTrigger trigger_type)
{
    ui->combo_LocalTrigger->setCurrentIndex(static_cast<int>(trigger_type));
}

void AddStoryDialog::set_reporters(PluginsInfoVector* reporters_info)
{
    plugin_factories = reporters_info;
    PluginsInfoVector& info = *plugin_factories;

    ui->combo_AvailableReporters->clear();
    foreach(const PluginInfo& pi_info, info)
        ui->combo_AvailableReporters->addItem(pi_info.name);

    ui->combo_AvailableReporters->setEnabled(info.count() > 1);

    QObject* instance = info[ui->combo_AvailableReporters->currentIndex()].factory->instance();
    IPluginFactory* ipluginfactory = reinterpret_cast<IPluginFactory*>(instance);
    Q_ASSERT(ipluginfactory);
    plugin_reporter = ipluginfactory->newInstance();
}

void AddStoryDialog::set_ttl(uint ttl)
{
    ui->edit_TTL->setText(QString::number(ttl));
}

void AddStoryDialog::set_headlines_always_visible(bool visible)
{
    ui->check_KeepOnTop->setChecked(visible);
}

void AddStoryDialog::set_display(int primary_screen, int screen_count)
{
    QVector<QRadioButton*> display_buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };
    for(int i = 0; i < 4;++i)
    {
        if((i + 1) > screen_count)
            display_buttons[i]->setDisabled(true);
       if(i == primary_screen)
           display_buttons[i]->setChecked(true);
    }

    if(screen_count == 1)
        ui->radio_Monitor1->setDisabled(true);
}

void AddStoryDialog::set_headlines_lock_size(int width, int height)
{
    ui->check_HeadlinesFixedSize->setChecked(width != 0 && height != 0);
    ui->edit_HeadlinesFixedWidth->setEnabled(ui->check_HeadlinesFixedSize->isChecked());
    ui->edit_HeadlinesFixedHeight->setEnabled(ui->check_HeadlinesFixedSize->isChecked());
    if(width)
        ui->edit_HeadlinesFixedWidth->setText(QString::number(width));
    if(height)
        ui->edit_HeadlinesFixedHeight->setText(QString::number(height));

    ui->group_HeadlinesSizeText->setEnabled(ui->check_HeadlinesFixedSize->isChecked());
}

void AddStoryDialog::set_headlines_fixed_text(FixedText fixed_type)
{
    ui->radio_HeadlinesSizeTextScale->setChecked(fixed_type == FixedText::None || fixed_type == FixedText::ScaleToFit);
    ui->radio_HeadlinesSizeTextClip->setChecked(fixed_type == FixedText::ClipToFit);
}

void AddStoryDialog::set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type)
{
    ui->combo_EntryType->setCurrentIndex(static_cast<int>(entry_type));
    ui->combo_ExitType->setCurrentIndex(static_cast<int>(exit_type));

    slot_entry_type_changed(ui->combo_EntryType->currentIndex());
}

void AddStoryDialog::set_train_age_effects(AgeEffects effect, int percent)
{
    ui->group_AgeEffects->setEnabled(effect != AgeEffects::None);
    ui->radio_TrainReduceOpacityFixed->setEnabled(effect == AgeEffects::ReduceOpacityFixed);
    if(percent)
        ui->edit_TrainReduceOpacity->setText(QString::number(percent));
    ui->edit_TrainReduceOpacity->setEnabled(effect == AgeEffects::ReduceOpacityFixed);
    ui->radio_TrainReduceOpacityByAge->setEnabled(effect == AgeEffects::ReduceOpacityByAge);
}

void AddStoryDialog::configure_for_local(bool for_local)
{
    ui->button_ConfigureReporter->setHidden(for_local);
    ui->label_Triggers->setHidden(!for_local);
    ui->combo_LocalTrigger->setHidden(!for_local);

    QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(for_local || reporter_configuration.count());
}

QString AddStoryDialog::get_story_identity()
{
    if(story.isLocalFile())
        return story.toLocalFile();

    // otherwise, we have to make something unique to this URL.
    Q_ASSERT(!plugin_reporter.isNull());
    IPluginURL* ipluginurl = reinterpret_cast<IPluginURL*>(plugin_reporter.data());
    Q_ASSERT(ipluginurl);
    QStringList params = ipluginurl->Requires();

    QString identity = ui->edit_Target->text();
    identity += QString(":%1").arg(ipluginurl->DisplayName()[0]);
    foreach(const QString& param, reporter_configuration)
        identity += QString(":%1").arg(param);

    return identity;
}

QUrl AddStoryDialog::get_target()
{
    if(ui->button_ConfigureReporter->isHidden())
        return QUrl::fromLocalFile(ui->edit_Target->text());

    QString text = ui->edit_Target->text();
    if(text.endsWith('/'))
        text.chop(1);
    return QUrl(text);
}

LocalTrigger AddStoryDialog::get_trigger()
{
    return static_cast<LocalTrigger>(ui->combo_LocalTrigger->currentIndex());
}

uint AddStoryDialog::get_ttl()
{
    QString value = ui->edit_TTL->text();
    if(value.isEmpty())
        return ui->edit_TTL->placeholderText().toInt();
    return value.toInt();
}

bool AddStoryDialog::get_headlines_always_visible()
{
    return ui->check_KeepOnTop->isChecked();
}

int AddStoryDialog::get_display()
{
    QVector<QRadioButton*> buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };
    for(int i = 0; i < 4;++i)
    {
        if(buttons[i]->isChecked())
            return i;
    }

    return 0;
}

bool AddStoryDialog::get_headlines_lock_size(int& width, int& height)
{
    width = 0;
    height = 0;
    if(!ui->check_HeadlinesFixedSize->isChecked())
        return false;
    width = ui->edit_HeadlinesFixedWidth->text().toInt();
    height = ui->edit_HeadlinesFixedHeight->text().toInt();
    return true;
}

FixedText AddStoryDialog::get_headlines_fixed_text()
{
    if(!ui->check_HeadlinesFixedSize->isChecked())
        return FixedText::None;
    return ui->radio_HeadlinesSizeTextScale->isChecked() ? FixedText::ScaleToFit : FixedText::ClipToFit;
}

AnimEntryType AddStoryDialog::get_animation_entry_type()
{
    return static_cast<AnimEntryType>(ui->combo_EntryType->currentIndex());
}

AnimExitType AddStoryDialog::get_animation_exit_type()
{
    return static_cast<AnimExitType>(ui->combo_ExitType->currentIndex());
}

AgeEffects AddStoryDialog::get_train_age_effects(int& percent)
{
    if(!ui->group_AgeEffects->isEnabled())
        return AgeEffects::None;

    percent = ui->edit_TrainReduceOpacity->text().toInt();
    return (ui->radio_TrainReduceOpacityFixed->isChecked() ? AgeEffects::ReduceOpacityFixed : AgeEffects::ReduceOpacityByAge);
}

void AddStoryDialog::slot_entry_type_changed(int index)
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

void AddStoryDialog::slot_headlines_fixed_size_clicked(bool checked)
{
    ui->edit_HeadlinesFixedWidth->setEnabled(checked);
    ui->edit_HeadlinesFixedHeight->setEnabled(checked);
    ui->group_HeadlinesSizeText->setEnabled(checked);
}

void AddStoryDialog::slot_train_reduce_opacity_clicked(bool /*checked*/)
{
    ui->edit_TrainReduceOpacity->setEnabled(ui->radio_TrainReduceOpacityFixed->isChecked());
}

void AddStoryDialog::slot_trigger_changed(int /*index*/)
{
    ui->combo_AvailableReporters->setEnabled(
       static_cast<LocalTrigger>(ui->combo_LocalTrigger->currentIndex()) == LocalTrigger::NewContent &&
       ui->combo_AvailableReporters->count() > 1);
}

void AddStoryDialog::slot_reporter_changed(int index)
{
    PluginsInfoVector& info = *plugin_factories;

    ui->combo_AvailableReporters->setToolTip(info[index].tooltip);

    QObject* instance = info[ui->combo_AvailableReporters->currentIndex()].factory->instance();
    IPluginFactory* ipluginfactory = reinterpret_cast<IPluginFactory*>(instance);
    Q_ASSERT(ipluginfactory);
    plugin_reporter = ipluginfactory->newInstance();
    reporter_configuration.clear();

    QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(ui->button_ConfigureReporter->isVisible() && reporter_configuration.count());
}

void AddStoryDialog::slot_configure_reporter(bool /*checked*/)
{
    // We construct a QDialog on the fly to get the parameters needed by the
    // selected URL Reporter.

    QPushButton* button;

    Q_ASSERT(!plugin_reporter.isNull());
    IPluginURL* ipluginurl = reinterpret_cast<IPluginURL*>(plugin_reporter.data());
    Q_ASSERT(ipluginurl);

    QStringList params = ipluginurl->Requires();

    params_dialog = new QDialog(this);
    params_dialog->setWindowTitle(tr("Newsroom: Configure Reporter"));
    params_dialog->setSizeGripEnabled(false);

    QVector<QLineEdit*> edit_fields;
    required_fields.resize(params.length() / 2);

    QVBoxLayout* vbox = new QVBoxLayout();
    for(int i = 0, j = 0;i < params.length();i += 2, ++j)
    {
        QString param_name(params[i]);
        if(param_name.endsWith(QChar('*')))
        {
            param_name.chop(1);
            required_fields.setBit(j);
        }
        else
            required_fields.clearBit(j);
        QLabel* label = new QLabel(param_name);
        QLineEdit* edit = new QLineEdit();
        edit->setObjectName(QString("edit_%1").arg(j));
        if(required_fields.at(j))
            edit->setStyleSheet("background-color: rgb(255, 245, 245);");

        if(!params[i+1].compare("password"))
            edit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        else if(!params[i+1].compare("integer"))
            edit->setValidator(new QIntValidator());
        else if(!params[i+1].compare("double"))
            edit->setValidator(new QDoubleValidator());

        if(reporter_configuration.count() > j)
            edit->setText(reporter_configuration[j]);

        edit_fields.push_back(edit);

        QHBoxLayout* hbox = new QHBoxLayout();
        hbox->addWidget(label);
        hbox->addWidget(edit);

        vbox->addLayout(hbox);
    }

    QDialogButtonBox* buttonBox = new QDialogButtonBox;
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, params_dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, params_dialog, &QDialog::reject);

    if(required_fields.count(true))
    {
        for(int i = 0;i < edit_fields.count();++i)
        {
            if(required_fields.at(i))
                connect(edit_fields[i], &QLineEdit::textChanged, this, &AddStoryDialog::slot_config_reporter_check_required);
        }
    }

    vbox->addWidget(buttonBox);

    params_dialog->setLayout(vbox);

    slot_config_reporter_check_required();

    if(params_dialog->exec() == QDialog::Accepted)
    {
        reporter_configuration.clear();
        foreach(QLineEdit* edit, edit_fields)
            reporter_configuration << edit->text();

        if(!ipluginurl->SetRequirements(reporter_configuration))
            reporter_configuration.clear();
    }

    params_dialog->deleteLater();
    params_dialog = nullptr;

    button = ui->buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(ui->button_ConfigureReporter->isVisible() && reporter_configuration.count());
}

void AddStoryDialog::slot_config_reporter_check_required()
{
    bool have_required_fields = true;

    QList<QLineEdit*> all_edits = params_dialog->findChildren<QLineEdit*>();
    foreach(QLineEdit* edit, all_edits)
    {
        QString name = edit->objectName();
        int index = name.split(QChar('_'))[1].toInt();
        if(required_fields.at(index))
        {
            if(edit->text().isEmpty())
            {
                have_required_fields = false;
                edit->setStyleSheet("background-color: rgb(255, 245, 245);");
            }
            else
                edit->setStyleSheet("background-color: rgb(245, 255, 245);");
        }
    }

    QDialogButtonBox* buttonBox = params_dialog->findChild<QDialogButtonBox*>("buttonBox");
    QPushButton* button = buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(have_required_fields);
}
