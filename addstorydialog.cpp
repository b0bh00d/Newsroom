#include <QtWidgets/QPushButton>
#include <QtWidgets/QPlainTextEdit>

#include <QtGui/QRegExpValidator>

#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>

#include <iplugin.h>

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
    ui->edit_LimitContent->setValidator(new QRegExpValidator(rx, this));
    ui->edit_FadeTargetMilliseconds->setValidator(new QRegExpValidator(rx, this));
    ui->edit_TrainReduceOpacity->setValidator(new QIntValidator(10, 90, this));
    ui->edit_DashboardReduceOpacity->setValidator(new QIntValidator(10, 90, this));

    ui->radio_HeadlinesSizeTextScale->setChecked(true);
    ui->radio_TrainReduceOpacityFixed->setChecked(true);

    ui->check_DashboardReduceOpacityFixed->setChecked(false);

    ui->group_ReporterConfig->setHidden(true);
    ui->group_Train->setHidden(true);
    ui->group_Dashboard->setHidden(true);

    QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
    line_edit->setPlaceholderText("Default");

    connect(this, &QDialog::accepted, this, &AddStoryDialog::slot_accepted);
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
    connect(ui->radio_InterpretAsPixels, &QCheckBox::clicked, this, &AddStoryDialog::slot_interpret_size_clicked);
    connect(ui->radio_InterpretAsPercentage, &QCheckBox::clicked, this, &AddStoryDialog::slot_interpret_size_clicked);
    connect(ui->radio_TrainReduceOpacityFixed, &QCheckBox::clicked, this, &AddStoryDialog::slot_train_reduce_opacity_clicked);
    connect(ui->radio_TrainReduceOpacityByAge, &QCheckBox::clicked, this, &AddStoryDialog::slot_train_reduce_opacity_clicked);
    connect(ui->combo_LocalTrigger, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_trigger_changed);

    QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
    connect(line_edit, &QLineEdit::editingFinished, this, &AddStoryDialog::slot_update_groupid_list);
    connect(ui->combo_DashboardGroupId, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_set_group_id_text);

    QDialog::showEvent(event);
    activateWindow();
    raise();
}

void AddStoryDialog::save_defaults(QSettings* settings)
{
    settings->beginGroup("AddStoryDialog");

    settings->setValue("target", story);
    settings->setValue("current_tab", ui->tabWidget->currentIndex());
    settings->setValue("local_trigger", ui->combo_LocalTrigger->currentIndex());
    settings->setValue("ttl", ui->edit_TTL->text());
    settings->setValue("keep_on_top", ui->check_KeepOnTop->isChecked());
    settings->setValue("display", ui->radio_Monitor1->isChecked() ? 0 : (ui->radio_Monitor2->isChecked() ? 1 : (ui->radio_Monitor3->isChecked() ? 2 : 3)));
    settings->setValue("headlines_fixed_width", ui->edit_HeadlinesFixedWidth->text());
    settings->setValue("headlines_fixed_height", ui->edit_HeadlinesFixedHeight->text());
    settings->setValue("interpret_as_pixels", ui->radio_InterpretAsPixels->isChecked());
    settings->setValue("interpret_as_percent", ui->radio_InterpretAsPercentage->isChecked());
    settings->setValue("limit_content", ui->check_LimitContent->isChecked());
    settings->setValue("limit_content_to", ui->edit_LimitContent->text());
    settings->setValue("fixed_text", ui->radio_HeadlinesSizeTextScale->isChecked() ? 0 : 1);
    settings->setValue("entry_type", ui->combo_EntryType->currentIndex());
    settings->setValue("exit_type", ui->combo_ExitType->currentIndex());
    settings->setValue("exit_type_enabled", ui->combo_ExitType->isEnabled());
    settings->setValue("anim_motion_duration", ui->edit_AnimMotionMilliseconds->text());
    settings->setValue("fade_opacity_duration", ui->edit_FadeTargetMilliseconds->text());
    settings->setValue("group_age_effects", ui->group_AgeEffects->isChecked());
    settings->setValue("age_opacity_type", ui->radio_TrainReduceOpacityFixed->isChecked() ? 1 : 2);
    settings->setValue("age_opacity_fixed_percent", ui->edit_TrainReduceOpacity->text());
    settings->setValue("reporter_configuration", reporter_configuration);
    settings->setValue("dashboard_age_effects", ui->check_DashboardReduceOpacityFixed->isChecked());
    settings->setValue("dashboard_age_effects_percent", ui->edit_DashboardReduceOpacity->text());
    QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
    settings->setValue("dashboard_group_id", line_edit->text());
    QStringList id_list;
    for(int i = 0;i < ui->combo_DashboardGroupId->count() && i < 10;++i)
        id_list << ui->combo_DashboardGroupId->itemText(i);
    settings->setValue("dashboard_group_id_list", id_list);
    settings->endGroup();
}

void AddStoryDialog::load_defaults(QSettings* settings)
{
    QVector<QRadioButton*> display_buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };
    QVector<QRadioButton*> fixed_buttons { ui->radio_HeadlinesSizeTextScale, ui->radio_HeadlinesSizeTextClip };
    QVector<QRadioButton*> age_buttons { nullptr, ui->radio_TrainReduceOpacityFixed, ui->radio_TrainReduceOpacityByAge };

    settings->beginGroup("AddStoryDialog");

    ui->tabWidget->setCurrentIndex(settings->value("current_tab", 0).toInt());
    story = settings->value("target", QUrl()).toUrl();
    if(story.isLocalFile())
        ui->edit_Target->setText(story.toLocalFile());
    else
        ui->edit_Target->setText(story.toString());
    ui->combo_LocalTrigger->setCurrentIndex(settings->value("local_trigger", static_cast<int>(LocalTrigger::NewContent)).toInt());
    ui->edit_TTL->setText(settings->value("ttl", "").toString());
    ui->check_KeepOnTop->setChecked(settings->value("keep_on_top", true).toBool());
    display_buttons[settings->value("display", 0).toInt()]->setChecked(true);

    ui->radio_InterpretAsPixels->setChecked(settings->value("interpret_as_pixels", true).toBool());
    ui->radio_InterpretAsPercentage->setChecked(settings->value("interpret_as_percent", false).toBool());
    slot_interpret_size_clicked(true);

    ui->edit_HeadlinesFixedWidth->setText(settings->value("headlines_fixed_width", QString()).toString());
    ui->edit_HeadlinesFixedHeight->setText(settings->value("headlines_fixed_height", QString()).toString());

    ui->check_LimitContent->setChecked(settings->value("limit_content", false).toBool());
    ui->edit_LimitContent->setText(settings->value("limit_content_to", QString()).toString());

    fixed_buttons[settings->value("fixed_text", 0).toInt()]->setChecked(true);
    ui->combo_EntryType->setCurrentIndex(settings->value("entry_type", static_cast<int>(AnimEntryType::SlideDownLeftTop)).toInt());
    ui->combo_ExitType->setCurrentIndex(settings->value("exit_type", static_cast<int>(AnimExitType::SlideLeft)).toInt());
    ui->combo_ExitType->setEnabled(settings->value("exit_type_enabled", true).toBool());
    ui->edit_AnimMotionMilliseconds->setText(settings->value("anim_motion_duration", QString()).toString());
    ui->edit_FadeTargetMilliseconds->setText(settings->value("fade_opacity_duration", QString()).toString());
    ui->group_AgeEffects->setChecked(settings->value("group_age_effects", false).toBool());
    age_buttons[settings->value("age_opacity_type", 1).toInt()]->setChecked(true);
    if(ui->radio_TrainReduceOpacityFixed->isChecked())
        ui->edit_TrainReduceOpacity->setText(QString::number(settings->value("age_opacity_fixed_percent", 60).toInt()));
    reporter_configuration = settings->value("reporter_configuration", QStringList()).toStringList();

    ui->check_DashboardReduceOpacityFixed->setChecked(settings->value("dashboard_age_effects", false).toBool());
    int percent = settings->value("dashboard_age_effects_percent", 60).toInt();
    if(percent)
        ui->edit_DashboardReduceOpacity->setText(QString::number(percent));

    QStringList id_list = settings->value("dashboard_group_id_list", QStringList()).toStringList();
    ui->combo_DashboardGroupId->addItems(id_list);
    QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
    line_edit->setText(settings->value("dashboard_group_id", QString()).toString());

    AnimEntryType entry_type = static_cast<AnimEntryType>(ui->combo_EntryType->currentIndex());
    ui->group_Train->setHidden(!IS_TRAIN(entry_type));
    ui->group_Dashboard->setHidden(!IS_DASHBOARD(entry_type));

    settings->endGroup();
}

void AddStoryDialog::set_story(const QUrl& story)
{
    this->story = story;

    if(story.isLocalFile())
    {
        ui->edit_Target->setText(QDir::toNativeSeparators(story.toLocalFile()));
        ui->edit_Target->setEnabled(false);
    }
    else
        ui->edit_Target->setText(story.toString());

    configure_for_local(story.isLocalFile());
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

    slot_configure_reporter_configure();
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

void AddStoryDialog::set_headlines_size(int width, int height)
{
    ui->radio_InterpretAsPixels->setChecked(true);

    QRegExp rx("\\d+");
    ui->edit_HeadlinesFixedWidth->setValidator(new QRegExpValidator(rx, this));
    ui->edit_HeadlinesFixedHeight->setValidator(new QRegExpValidator(rx, this));

    if(width)
        ui->edit_HeadlinesFixedWidth->setText(QString::number(width));
    if(height)
        ui->edit_HeadlinesFixedHeight->setText(QString::number(height));
}

void AddStoryDialog::set_headlines_size(double width, double height)
{
    ui->radio_InterpretAsPercentage->setChecked(true);

    ui->edit_HeadlinesFixedWidth->setValidator(new QDoubleValidator(this));
    ui->edit_HeadlinesFixedHeight->setValidator(new QDoubleValidator(this));

    if(width)
        ui->edit_HeadlinesFixedWidth->setText(QString::number(width));
    if(height)
        ui->edit_HeadlinesFixedHeight->setText(QString::number(height));
}

void AddStoryDialog::set_limit_content(int lines)
{
    ui->check_LimitContent->setChecked(lines != 0);
    ui->edit_LimitContent->setEnabled(ui->check_LimitContent->isChecked());
    if(lines)
        ui->edit_LimitContent->setText(QString::number(lines));
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

void AddStoryDialog::set_fade_target_duration(int duration)
{
    if(duration)
        ui->edit_FadeTargetMilliseconds->setText(QString::number(duration));
}

void AddStoryDialog::set_anim_motion_duration(int duration)
{
    if(duration)
        ui->edit_AnimMotionMilliseconds->setText(QString::number(duration));
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

void AddStoryDialog::set_dashboard_age_effects(int percent)
{
    ui->check_DashboardReduceOpacityFixed->setChecked(percent != 0);
    if(!percent)
        ui->edit_DashboardReduceOpacity->setText("");
    else
        ui->edit_DashboardReduceOpacity->setText(QString::number(percent));
}

void AddStoryDialog::set_dashboard_group_id(const QString& group_id)
{
    if(!group_id.isEmpty())
        ui->combo_DashboardGroupId->setEditText(group_id);
}

void AddStoryDialog::configure_for_local(bool for_local)
{
    ui->group_ReporterConfig->setHidden(for_local);
    ui->label_Triggers->setHidden(!for_local);
    ui->combo_LocalTrigger->setHidden(!for_local);

    QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(for_local || reporter_configuration.count());
}

QString AddStoryDialog::get_story_identity()
{
    if(story.isLocalFile())
        return story.toLocalFile();

    // otherwise, we try to make something unique to this URL...
    Q_ASSERT(!plugin_reporter.isNull());

    QString identity = ui->edit_Target->text();
    identity += QString(":%1").arg(plugin_reporter->DisplayName()[0]);

    QStringList params = plugin_reporter->Requires();
    if(!params.isEmpty())
    {
        foreach(const QString& param, reporter_configuration)
        {
            if(!param.startsWith("multiline:"))
                identity += QString(":%1").arg(param);
        }
    }

    return identity;
}

QUrl AddStoryDialog::get_target()
{
    if(ui->group_ReporterConfig->isHidden())
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

bool AddStoryDialog::get_headlines_size(int& width, int& height)
{
    if(!ui->radio_InterpretAsPixels->isChecked())
        return false;

    width = 0;
    height = 0;

    if(ui->edit_HeadlinesFixedWidth->text().isEmpty())
        width = ui->edit_HeadlinesFixedWidth->placeholderText().toInt();
    else
        width = ui->edit_HeadlinesFixedWidth->text().toInt();
    if(ui->edit_HeadlinesFixedHeight->text().isEmpty())
        height = ui->edit_HeadlinesFixedHeight->placeholderText().toInt();
    else
        height = ui->edit_HeadlinesFixedHeight->text().toInt();

    return true;
}

bool AddStoryDialog::get_headlines_size(double& width, double& height)
{
    if(ui->radio_InterpretAsPixels->isChecked())
        return false;

    width = 0.0;
    height = 0.0;

    if(ui->edit_HeadlinesFixedWidth->text().isEmpty())
        width = ui->edit_HeadlinesFixedWidth->placeholderText().toDouble();
    else
        width = ui->edit_HeadlinesFixedWidth->text().toDouble();
    if(ui->edit_HeadlinesFixedHeight->text().isEmpty())
        height = ui->edit_HeadlinesFixedHeight->placeholderText().toDouble();
    else
        height = ui->edit_HeadlinesFixedHeight->text().toDouble();

    return true;
}

bool AddStoryDialog::get_limit_content(int& lines)
{
    lines = 0;
    if(!ui->edit_LimitContent->text().isEmpty())
        lines = ui->edit_LimitContent->text().toInt();
    return ui->check_LimitContent->isChecked();
}

FixedText AddStoryDialog::get_headlines_fixed_text()
{
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

int AddStoryDialog::get_fade_target_duration()
{
    if(ui->edit_FadeTargetMilliseconds->text().isEmpty())
        return ui->edit_FadeTargetMilliseconds->placeholderText().toInt();
    return ui->edit_FadeTargetMilliseconds->text().toInt();
}

int AddStoryDialog::get_anim_motion_duration()
{
    if(ui->edit_AnimMotionMilliseconds->text().isEmpty())
        return ui->edit_AnimMotionMilliseconds->placeholderText().toInt();
    return ui->edit_AnimMotionMilliseconds->text().toInt();
}

AgeEffects AddStoryDialog::get_train_age_effects(int& percent)
{
    if(!ui->group_AgeEffects->isEnabled())
        return AgeEffects::None;

    percent = ui->edit_TrainReduceOpacity->text().toInt();
    return (ui->radio_TrainReduceOpacityFixed->isChecked() ? AgeEffects::ReduceOpacityFixed : AgeEffects::ReduceOpacityByAge);
}

bool AddStoryDialog::get_dashboard_age_effects(int& percent)
{
    percent = ui->edit_DashboardReduceOpacity->text().toInt();
    return ui->check_DashboardReduceOpacityFixed->isChecked();
}

QString AddStoryDialog::get_dashboard_group_id()
{
    QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
    QString group_id = line_edit->text();
    if(group_id.isEmpty())
        return line_edit->placeholderText();
    return group_id;
}

void AddStoryDialog::slot_accepted()
{
    reporter_configuration.clear();
    QList<QLineEdit*> all_edits = ui->group_ReporterConfig->findChildren<QLineEdit*>();
    foreach(QLineEdit* edit, all_edits)
        reporter_configuration << edit->text();

    plugin_reporter->SetRequirements(reporter_configuration);
}

void AddStoryDialog::slot_entry_type_changed(int index)
{
    AnimEntryType entry_type = static_cast<AnimEntryType>(index);
    bool is_train = IS_TRAIN(entry_type);
    bool is_dashboard = IS_DASHBOARD(entry_type);
    ui->group_Train->setHidden(!is_train);
    ui->group_Dashboard->setHidden(!is_dashboard);

    ui->combo_ExitType->setEnabled(!is_train && !is_dashboard);
}

void AddStoryDialog::slot_interpret_size_clicked(bool /*checked*/)
{
    if(ui->radio_InterpretAsPixels->isChecked())
    {
        QRegExp rx("\\d+");
        ui->edit_HeadlinesFixedWidth->setValidator(new QRegExpValidator(rx, this));
        ui->edit_HeadlinesFixedHeight->setValidator(new QRegExpValidator(rx, this));

        if(ui->edit_HeadlinesFixedWidth->text().isEmpty())
            ui->edit_HeadlinesFixedWidth->setPlaceholderText("350");
        if(ui->edit_HeadlinesFixedHeight->text().isEmpty())
            ui->edit_HeadlinesFixedHeight->setPlaceholderText("100");
    }
    else
    {
        ui->edit_HeadlinesFixedWidth->setValidator(new QDoubleValidator(this));
        ui->edit_HeadlinesFixedHeight->setValidator(new QDoubleValidator(this));

        if(ui->edit_HeadlinesFixedWidth->text().isEmpty())
            ui->edit_HeadlinesFixedWidth->setPlaceholderText("18.2");
        if(ui->edit_HeadlinesFixedHeight->text().isEmpty())
            ui->edit_HeadlinesFixedHeight->setPlaceholderText("8.3");
    }
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

    slot_configure_reporter_configure();
}

void AddStoryDialog::slot_set_group_id_text(int index)
{
    QString str = ui->combo_DashboardGroupId->itemText(index);
    if(str.isEmpty())
        return;
    ui->combo_DashboardGroupId->setEditText(str);
}

void AddStoryDialog::slot_update_groupid_list()
{
    QString id = ui->combo_DashboardGroupId->currentText();
    if(id.isEmpty())
        return;

    for(int i = 0;i < ui->combo_DashboardGroupId->count();++i)
    {
        if(id.compare(ui->combo_DashboardGroupId->itemText(i)) == 0)
            return;
    }

    ui->combo_DashboardGroupId->addItem(id);
    ui->combo_DashboardGroupId->lineEdit()->setText(id);
}

void AddStoryDialog::slot_configure_reporter_configure()
{
    // we populate group_ReporterConfig with controls for entering
    // Reporter configuration parameters

    QObject* child;
    while((child = ui->group_ReporterConfig->findChild<QObject*>()) != nullptr)
        child->deleteLater();

    Q_ASSERT(!plugin_reporter.isNull());

    QStringList params = plugin_reporter->Requires();
    if(params.isEmpty())
    {
        ui->group_ReporterConfig->setHidden(true);
        return;
    }

    QVector<QWidget*> edit_fields;
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
        QLineEdit* edit = nullptr;
        QPlainTextEdit* multiline = nullptr;

        QString type(params[i+1]);
        QString def_value;

        if(!type.startsWith("multiline"))
        {
            edit = new QLineEdit();
            edit->setObjectName(QString("edit_%1").arg(j));
            if(required_fields.at(j))
                edit->setStyleSheet("background-color: rgb(255, 245, 245);");

            if(params[i+1].contains(':'))
            {
                QStringList items = params[i+1].split(':');
                type = items[0];
                def_value = items[1];
            }
        }
        else if(type.startsWith("multiline:"))
        {
            multiline = new QPlainTextEdit();
            multiline->setObjectName(QString("multiline_%1").arg(j));
            multiline->setWordWrapMode(QTextOption::NoWrap);
            if(required_fields.at(j))
                multiline->setStyleSheet("background-color: rgb(255, 245, 245);");

            def_value = type.right(type.length() - 10);
            type = "multiline";
        }

        if(!type.compare("multiline"))
        {
            if(reporter_configuration.count() > j)
                multiline->insertPlainText(reporter_configuration[j]);
            else
                multiline->insertPlainText(def_value);

            edit_fields.push_back(multiline);

            QHBoxLayout* hbox = new QHBoxLayout();
            hbox->addWidget(label);
            hbox->addWidget(multiline);

            vbox->addLayout(hbox);
        }
        else
        {
            if(!type.compare("password"))
                edit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
            else if(!type.compare("integer"))
                edit->setValidator(new QIntValidator());
            else if(!type.compare("double"))
                edit->setValidator(new QDoubleValidator());

            if(!def_value.isEmpty())
                edit->setPlaceholderText(def_value);

            if(reporter_configuration.count() > j)
                edit->setText(reporter_configuration[j]);

            edit_fields.push_back(edit);

            QHBoxLayout* hbox = new QHBoxLayout();
            hbox->addWidget(label);
            hbox->addWidget(edit);

            vbox->addLayout(hbox);
        }
    }

    if(required_fields.count(true))
    {
        for(int i = 0;i < edit_fields.count();++i)
        {
            if(required_fields.at(i))
            {
                QLineEdit* edit = qobject_cast<QLineEdit*>(edit_fields[i]);
                if(edit)
                    connect(edit, &QLineEdit::textChanged, this, &AddStoryDialog::slot_config_reporter_check_required);
                else
                {
                    QPlainTextEdit* multiline = qobject_cast<QPlainTextEdit*>(edit_fields[i]);
                    if(multiline)
                        connect(multiline, &QPlainTextEdit::textChanged, this, &AddStoryDialog::slot_config_reporter_check_required);
                }
            }
        }
    }

    ui->group_ReporterConfig->setLayout(vbox);

    slot_config_reporter_check_required();
}

void AddStoryDialog::slot_config_reporter_check_required()
{
    bool have_required_fields = true;

    reporter_configuration.clear();

    QList<QWidget*> all_edits = ui->group_ReporterConfig->findChildren<QWidget*>();
    foreach(QWidget* child, all_edits)
    {
        QPlainTextEdit* multiline = nullptr;
        QString name;

        QLineEdit* edit = qobject_cast<QLineEdit*>(child);
        if(edit)
            name = edit->objectName();
        else
        {
            QPlainTextEdit* multiline = qobject_cast<QPlainTextEdit*>(child);
            if(multiline)
                name = multiline->objectName();
        }

        if(name.isEmpty())
            continue;

        int index = name.split(QChar('_'))[1].toInt();

        if(required_fields.at(index))
        {
            if(edit)
            {
                if(edit->text().isEmpty())
                {
                    have_required_fields = false;
                    edit->setStyleSheet("background-color: rgb(255, 245, 245);");
                }
                else
                    edit->setStyleSheet("background-color: rgb(245, 255, 245);");

                reporter_configuration << edit->text();
            }
            else if(multiline)
            {
                if(multiline->toPlainText().isEmpty())
                {
                    have_required_fields = false;
                    multiline->setStyleSheet("background-color: rgb(255, 245, 245);");
                }
                else
                    multiline->setStyleSheet("background-color: rgb(245, 255, 245);");

                reporter_configuration << multiline->toPlainText();
            }
        }
    }

    QDialogButtonBox* buttonBox = findChild<QDialogButtonBox*>("buttonBox");
    QPushButton* button = buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(have_required_fields);
}
