#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QPlainTextEdit>

#include <QtGui/QRegExpValidator>

#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>

#include <ireporter.h>

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

static const QMap<AnimEntryType, QString> animentrytype_map
{
#define X(a) { AnimEntryType::a, QObject::tr( #a ) },
#include "animentrytype.def"
#undef X
};

static const QStringList animexittype_str
{
#define X(a) QObject::tr( #a ),
#include "animexittype.def"
#undef X
};

static const QMap<ReportStacking, QString> reportstacking_map
{
#define X(a) { ReportStacking::a, QObject::tr( #a ) },
#include "reportstacking.def"
#undef X
};

AddStoryDialog::AddStoryDialog(PluginsInfoVector *reporters_info,
                               StoryInfoPointer story_info,
                               SettingsPointer settings,
                               QWidget *parent)
    : plugin_factories(reporters_info),
      story_info(story_info),
      settings(settings),
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
//    ui->edit_HeadlinesFixedWidth->setValidator(new QRegExpValidator(rx, this));
//    ui->edit_HeadlinesFixedHeight->setValidator(new QRegExpValidator(rx, this));
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

    load_settings();
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

    connect(ui->radio_Monitor1, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);
    connect(ui->radio_Monitor2, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);
    connect(ui->radio_Monitor3, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);
    connect(ui->radio_Monitor4, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);

    QDialog::showEvent(event);
    activateWindow();
    raise();
}

void AddStoryDialog::save_settings()
{
    settings->beginGroup("AddStoryDialog");
    settings->setValue("current_tab", ui->tabWidget->currentIndex());
    QStringList id_list;
    for(int i = 0;i < ui->combo_DashboardGroupId->count() && i < 10;++i)
        id_list << ui->combo_DashboardGroupId->itemText(i);
    settings->setValue("dashboard_group_id_list", id_list);
    settings->endGroup();

    QString story = ui->edit_Source->text();
    if(QFile::exists(story))
        story_info->story = QUrl::fromLocalFile(story);
    else
    {
        if(story.endsWith("/"))
            story.chop(1);
        story_info->story.setUrl(story);
    }

    story_info->identity = ui->edit_Angle->text();
    if(story_info->identity.isEmpty())
        story_info->identity = ui->edit_Angle->placeholderText();

    story_info->reporter_id = plugin_reporter->PluginID();

    story_info->reporter_parameters.clear();
    QList<QWidget*> all_edits = ui->group_ReporterConfig->findChildren<QWidget*>();
    foreach(QWidget* child, all_edits)
    {
        QString value;
        QLineEdit* edit = qobject_cast<QLineEdit*>(child);
        if(edit)
            story_info->reporter_parameters << edit->text();
        else
        {
            QPlainTextEdit* multiline = qobject_cast<QPlainTextEdit*>(child);
            if(multiline)
                story_info->reporter_parameters << multiline->toPlainText();
        }
    }

    story_info->trigger_type = static_cast<LocalTrigger>(ui->combo_LocalTrigger->currentIndex());
    story_info->ttl = ui->edit_TTL->text().toInt();
    story_info->headlines_always_visible = ui->check_KeepOnTop->isChecked();
    story_info->primary_screen = ui->radio_Monitor1->isChecked() ? 0 : (ui->radio_Monitor2->isChecked() ? 1 : (ui->radio_Monitor3->isChecked() ? 2 : 3));
    if(ui->radio_InterpretAsPixels->isChecked())
    {
        if(ui->edit_HeadlinesFixedWidth->text().isEmpty())
            story_info->headlines_pixel_width = ui->edit_HeadlinesFixedWidth->placeholderText().toInt();
        else
            story_info->headlines_pixel_width = ui->edit_HeadlinesFixedWidth->text().toInt();

        if(ui->edit_HeadlinesFixedHeight->text().isEmpty())
            story_info->headlines_pixel_height = ui->edit_HeadlinesFixedHeight->placeholderText().toInt();
        else
            story_info->headlines_pixel_height = ui->edit_HeadlinesFixedHeight->text().toInt();

        story_info->headlines_percent_width = 0.0;
        story_info->headlines_percent_height = 0.0;
    }
    else
    {
        if(ui->edit_HeadlinesFixedWidth->text().isEmpty())
            story_info->headlines_percent_width = ui->edit_HeadlinesFixedWidth->placeholderText().toDouble();
        else
            story_info->headlines_percent_width = ui->edit_HeadlinesFixedWidth->text().toDouble();

        if(ui->edit_HeadlinesFixedHeight->text().isEmpty())
            story_info->headlines_percent_height = ui->edit_HeadlinesFixedHeight->placeholderText().toDouble();
        else
            story_info->headlines_percent_height = ui->edit_HeadlinesFixedHeight->text().toDouble();

        story_info->headlines_pixel_width = 0;
        story_info->headlines_pixel_height = 0;
    }
    story_info->limit_content = ui->check_LimitContent->isChecked();
    if(!ui->edit_LimitContent->text().isEmpty())
        story_info->limit_content_to = ui->edit_LimitContent->text().toInt();
    story_info->headlines_fixed_type = ui->radio_HeadlinesSizeTextScale->isChecked() ? FixedText::ScaleToFit : FixedText::ClipToFit;
    story_info->entry_type = static_cast<AnimEntryType>(ui->combo_EntryType->currentIndex());
    story_info->exit_type = static_cast<AnimExitType>(ui->combo_ExitType->currentIndex());
    if(!ui->edit_AnimMotionMilliseconds->text().isEmpty())
        story_info->anim_motion_duration = ui->edit_AnimMotionMilliseconds->text().toInt();
    else
        story_info->anim_motion_duration = ui->edit_AnimMotionMilliseconds->placeholderText().toInt();
    if(!ui->edit_FadeTargetMilliseconds->text().isEmpty())
        story_info->fade_target_duration = ui->edit_FadeTargetMilliseconds->text().toInt();
    else
        story_info->fade_target_duration = ui->edit_FadeTargetMilliseconds->placeholderText().toInt();
    story_info->train_use_age_effect = ui->group_TrainAgeEffects->isChecked();
    story_info->train_age_effect = static_cast<AgeEffects>(ui->radio_TrainReduceOpacityFixed->isChecked() ? 1 : 2);
    if(!ui->edit_TrainReduceOpacity->text().isEmpty())
        story_info->train_age_percent = ui->edit_TrainReduceOpacity->text().toInt();
    story_info->dashboard_use_age_effect = ui->check_DashboardReduceOpacityFixed->isChecked();
    if(!ui->edit_DashboardReduceOpacity->text().isEmpty())
        story_info->dashboard_age_percent = ui->edit_DashboardReduceOpacity->text().toInt();
    QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
    if(line_edit->text().isEmpty())
        story_info->dashboard_group_id = line_edit->placeholderText();
    else
        story_info->dashboard_group_id = line_edit->text();
}

void AddStoryDialog::load_settings()
{
    QVector<QRadioButton*> fixed_buttons { ui->radio_HeadlinesSizeTextScale, ui->radio_HeadlinesSizeTextClip };
    QVector<QRadioButton*> age_buttons { nullptr, ui->radio_TrainReduceOpacityFixed, ui->radio_TrainReduceOpacityByAge };

    settings->beginGroup("AddStoryDialog");

    ui->tabWidget->setCurrentIndex(settings->value("current_tab", 0).toInt());

    QStringList id_list = settings->value("dashboard_group_id_list", QStringList()).toStringList();
    ui->combo_DashboardGroupId->addItems(id_list);

    settings->endGroup();

    if(story_info->story.isLocalFile())
    {
        ui->edit_Source->setText(QDir::toNativeSeparators(story_info->story.toLocalFile()));
        ui->edit_Source->setEnabled(false);
    }
    else
        ui->edit_Source->setText(story_info->story.toString());

    if(story_info->identity.isEmpty())
        ui->edit_Angle->setPlaceholderText(ui->edit_Source->text());
    else
        ui->edit_Angle->setText(story_info->identity);

    configure_for_local(story_info->story.isLocalFile());

    PluginsInfoVector& info = *plugin_factories;

    ui->combo_AvailableReporters->clear();
    foreach(const PluginInfo& pi_info, info)
        ui->combo_AvailableReporters->addItem(pi_info.name);

    if(info.count() > 1)
    {
        // find the Reporter
        if(!story_info->reporter_id.isEmpty())
        {
            int index = 0;
            for(index = 0;index < info.count();++index)
            {
                QObject* instance = info[index].factory->instance();
                IReporterFactory* ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
                Q_ASSERT(ireporterfactory);
                plugin_reporter = ireporterfactory->newInstance();
                if(!story_info->reporter_id.compare(plugin_reporter->PluginID()))
                    break;
            }

            ui->combo_AvailableReporters->setCurrentIndex(index);
        }
    }
    else
    {
        ui->combo_AvailableReporters->setEnabled(false);

        QObject* instance = info[ui->combo_AvailableReporters->currentIndex()].factory->instance();
        IReporterFactory* ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
        Q_ASSERT(ireporterfactory);
        plugin_reporter = ireporterfactory->newInstance();
    }

    reporter_configuration = story_info->reporter_parameters;

    slot_configure_reporter_configure();

    ui->combo_LocalTrigger->setCurrentIndex(static_cast<int>(story_info->trigger_type));
    ui->edit_TTL->setText(QString::number(story_info->ttl));
    ui->check_KeepOnTop->setChecked(story_info->headlines_always_visible);

    set_display();

    ui->radio_InterpretAsPixels->setChecked(story_info->interpret_as_pixels);
    ui->radio_InterpretAsPercentage->setChecked(!story_info->interpret_as_pixels);

    slot_interpret_size_clicked(true);

    if(story_info->interpret_as_pixels)
    {
        if(story_info->headlines_pixel_width != 0 && ui->edit_HeadlinesFixedWidth->placeholderText().toInt() != story_info->headlines_pixel_width)
            ui->edit_HeadlinesFixedWidth->setText(QString::number(story_info->headlines_pixel_width));
        if(story_info->headlines_pixel_height != 0 && ui->edit_HeadlinesFixedHeight->placeholderText().toInt() != story_info->headlines_pixel_height)
            ui->edit_HeadlinesFixedHeight->setText(QString::number(story_info->headlines_pixel_height));
    }
    else
    {
        if(story_info->headlines_percent_width != 0.0 && ui->edit_HeadlinesFixedWidth->placeholderText().toDouble() != story_info->headlines_percent_width)
            ui->edit_HeadlinesFixedWidth->setText(QString::number(story_info->headlines_percent_width));
        if(story_info->headlines_percent_height != 0.0 && ui->edit_HeadlinesFixedHeight->placeholderText().toDouble() != story_info->headlines_percent_height)
            ui->edit_HeadlinesFixedHeight->setText(QString::number(story_info->headlines_percent_height));
    }
    slot_interpret_size_clicked(true);

    ui->check_LimitContent->setChecked(story_info->limit_content);
    ui->edit_LimitContent->setEnabled(ui->check_LimitContent->isChecked());
    if(story_info->limit_content_to)
        ui->edit_LimitContent->setText(QString::number(story_info->limit_content_to));

    fixed_buttons[(story_info->headlines_fixed_type == FixedText::None || story_info->headlines_fixed_type == FixedText::ScaleToFit) ? 0 : 1]->setChecked(true);

    ui->combo_EntryType->setCurrentIndex(static_cast<int>(story_info->entry_type));
    ui->combo_ExitType->setCurrentIndex(static_cast<int>(story_info->exit_type));

    slot_entry_type_changed(ui->combo_EntryType->currentIndex());

    if(story_info->anim_motion_duration)
    {
        if(ui->edit_AnimMotionMilliseconds->placeholderText().toInt() != story_info->anim_motion_duration)
            ui->edit_AnimMotionMilliseconds->setText(QString::number(story_info->anim_motion_duration));
    }
    if(story_info->fade_target_duration)
    {
        if(ui->edit_FadeTargetMilliseconds->placeholderText().toInt() != story_info->fade_target_duration)
            ui->edit_FadeTargetMilliseconds->setText(QString::number(story_info->fade_target_duration));
    }

    ui->group_TrainAgeEffects->setChecked(story_info->train_use_age_effect);
    age_buttons[(story_info->train_age_effect == AgeEffects::None || story_info->train_age_effect == AgeEffects::ReduceOpacityFixed) ? 1 : 2]->setChecked(true);
    ui->edit_TrainReduceOpacity->setEnabled(ui->radio_TrainReduceOpacityFixed->isChecked());
    if(ui->radio_TrainReduceOpacityFixed->isChecked())
    {
        if(ui->edit_TrainReduceOpacity->placeholderText().toInt() != story_info->train_age_percent)
            ui->edit_TrainReduceOpacity->setText(QString::number(story_info->train_age_percent));
    }

//    ui->group_TrainAgeEffects->setEnabled(story_info->train_age_effect != AgeEffects::None);
//    ui->radio_TrainReduceOpacityFixed->setEnabled(story_info->train_age_effect == AgeEffects::ReduceOpacityFixed);
//    if(percent)
//        ui->edit_TrainReduceOpacity->setText(QString::number(percent));
//    ui->radio_TrainReduceOpacityByAge->setEnabled(story_info->train_age_effect == AgeEffects::ReduceOpacityByAge);

    ui->check_DashboardReduceOpacityFixed->setChecked(story_info->dashboard_use_age_effect);
    if(ui->check_DashboardReduceOpacityFixed->isChecked())
    {
        if(ui->edit_DashboardReduceOpacity->placeholderText().toInt() != story_info->dashboard_age_percent)
            ui->edit_DashboardReduceOpacity->setText(QString::number(story_info->dashboard_age_percent));
    }

    QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
    line_edit->setText(story_info->dashboard_group_id);

    ui->group_Train->setHidden(!IS_TRAIN(story_info->entry_type));
    ui->group_Dashboard->setHidden(!IS_DASHBOARD(story_info->entry_type));

    set_angle();

    if(!ui->edit_Angle->placeholderText().compare(story_info->identity))
        ui->edit_Angle->setText(QString());
}

void AddStoryDialog::set_display()
{
    QVector<QRadioButton*> display_buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };

    QDesktopWidget* desktop = QApplication::desktop();
    int screen_count = desktop->screenCount();

    for(int i = 0; i < 4;++i)
    {
        if((i + 1) > screen_count)
            display_buttons[i]->setDisabled(true);
       if(i == story_info->primary_screen)
           display_buttons[i]->setChecked(true);
    }

    if(screen_count == 1)
        ui->radio_Monitor1->setDisabled(true);
}

void AddStoryDialog::configure_for_local(bool for_local)
{
    ui->group_ReporterConfig->setHidden(for_local);
    ui->label_Triggers->setHidden(!for_local);
    ui->combo_LocalTrigger->setHidden(!for_local);

    QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(for_local || reporter_configuration.count());
}

void AddStoryDialog::set_angle()
{
    if(!ui->edit_Angle->text().isEmpty())
        return;

    AnimEntryType entry_type = static_cast<AnimEntryType>(ui->combo_EntryType->currentIndex());

    QString target;
    if(story_info->story.isLocalFile())
    {
        QFileInfo info(story_info->story.toLocalFile());
        target = info.fileName();
    }
    else
    {
        QString story_str = story_info->story.toString();
        if(story_str.endsWith("/"))
            story_str.chop(1);
        target = story_str.split("/").back();
    }

    QStringList requirements = plugin_reporter->Requires();
    QString plugin_params;
    QList<QLineEdit*> all_edits = ui->group_ReporterConfig->findChildren<QLineEdit*>();
    int index = 0;
    foreach(QLineEdit* child, all_edits)
    {
        QString value = child->text();
        if(!value.isEmpty() && !requirements[(index*2)+1].compare("string"))
        {
            if(plugin_params.length())
                plugin_params += "?";
            plugin_params += value.replace(" ", "_");
        }
        ++index;
    }
    QString plugin_id = QString("%1%2").arg(plugin_reporter->DisplayName()[0]).arg(plugin_params.isEmpty() ? "" : QString("(%1)").arg(plugin_params));

    QString display = QString("Display %1").arg(ui->radio_Monitor1->isChecked() ? 1 : (ui->radio_Monitor2->isChecked() ? 2 : (ui->radio_Monitor3->isChecked() ? 3 : 4)));

    if(IS_DASHBOARD(entry_type))
    {
        QLineEdit* line_edit = ui->combo_DashboardGroupId->lineEdit();
        if(line_edit->text().isEmpty())
            ui->edit_Angle->setPlaceholderText(QString("%1::%2::%3::%4::%5")
                            .arg(display)
                            .arg(animentrytype_map[entry_type])
                            .arg(plugin_id)
                            .arg(line_edit->placeholderText())
                            .arg(target));
        else
            ui->edit_Angle->setPlaceholderText(QString("%1::%2::%3::%4::%5")
                            .arg(display)
                            .arg(animentrytype_map[entry_type])
                            .arg(plugin_id)
                            .arg(line_edit->text())
                            .arg(target));
    }
    else
        ui->edit_Angle->setPlaceholderText(QString("%1::%2::%3::%4")
                        .arg(display)
                        .arg(animentrytype_map[entry_type])
                        .arg(plugin_id)
                        .arg(target));
}

void AddStoryDialog::slot_accepted()
{
    save_settings();
}

void AddStoryDialog::slot_entry_type_changed(int index)
{
    AnimEntryType entry_type = static_cast<AnimEntryType>(index);
    bool is_train = IS_TRAIN(entry_type);
    bool is_dashboard = IS_DASHBOARD(entry_type);
    ui->group_Train->setHidden(!is_train);
    ui->group_Dashboard->setHidden(!is_dashboard);

    ui->combo_ExitType->setEnabled(!is_train && !is_dashboard);

    set_angle();
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
    IReporterFactory* ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
    Q_ASSERT(ireporterfactory);
    plugin_reporter = ireporterfactory->newInstance();
    reporter_configuration.clear();

    slot_configure_reporter_configure();
}

void AddStoryDialog::slot_set_group_id_text(int index)
{
    QString str = ui->combo_DashboardGroupId->itemText(index);
    if(str.isEmpty())
        return;
    ui->combo_DashboardGroupId->setEditText(str);

    set_angle();
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

    set_angle();
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
            QLineEdit* edit = qobject_cast<QLineEdit*>(edit_fields[i]);
            if(edit)
            {
                if(required_fields.at(i))
                    connect(edit, &QLineEdit::textChanged, this, &AddStoryDialog::slot_config_reporter_check_required);
                else
                    connect(edit, &QLineEdit::textChanged, this, &AddStoryDialog::set_angle);
            }
            else
            {
                QPlainTextEdit* multiline = qobject_cast<QPlainTextEdit*>(edit_fields[i]);
                if(multiline && required_fields.at(i))
                    connect(multiline, &QPlainTextEdit::textChanged, this, &AddStoryDialog::slot_config_reporter_check_required);
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

    set_angle();
}
