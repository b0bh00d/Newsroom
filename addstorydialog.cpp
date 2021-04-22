#include <QtWidgets/QPushButton>
#include <QtWidgets/QPlainTextEdit>

#include <QtGui/QRegExpValidator>

#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStandardPaths>

#include <ireporter.h>

#include "mainwindow.h"
#include "addstorydialog.h"
#include "ui_addstorydialog.h"

extern MainWindow* mainwindow;

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

AddStoryDialog::AddStoryDialog(BeatsMap &beats_map,
                               StoryInfoPointer story_info,
                               SettingsPointer settings,
                               const QString &defaults_folder,
                               AddStoryDialog::Mode edit_mode,
                               QWidget *parent)
    : QDialog(parent),
      ui(new Ui::AddStoryDialog),
      story_info(story_info),
      settings(settings),
      plugin_beats(beats_map),
      edit_mode(edit_mode),
      defaults_folder(defaults_folder)
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

    ui->group_Train->setHidden(true);
    ui->group_Dashboard->setHidden(true);

    auto line_edit = ui->combo_DashboardGroupId->lineEdit();
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
    connect(ui->combo_ReporterBeats, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_beat_changed);
    connect(ui->combo_AvailableReporters, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_reporter_changed);
    connect(ui->radio_InterpretAsPixels, &QCheckBox::clicked, this, &AddStoryDialog::slot_interpret_size_clicked);
    connect(ui->radio_InterpretAsPercentage, &QCheckBox::clicked, this, &AddStoryDialog::slot_interpret_size_clicked);
    connect(ui->radio_TrainReduceOpacityFixed, &QCheckBox::clicked, this, &AddStoryDialog::slot_train_reduce_opacity_clicked);
    connect(ui->radio_TrainReduceOpacityByAge, &QCheckBox::clicked, this, &AddStoryDialog::slot_train_reduce_opacity_clicked);
//    connect(ui->combo_LocalTrigger, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//            this, &AddStoryDialog::slot_trigger_changed);

    auto line_edit = ui->combo_DashboardGroupId->lineEdit();
    connect(line_edit, &QLineEdit::editingFinished, this, &AddStoryDialog::slot_update_groupid_list);
    connect(ui->combo_DashboardGroupId, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AddStoryDialog::slot_set_group_id_text);

    connect(ui->radio_Monitor1, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);
    connect(ui->radio_Monitor2, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);
    connect(ui->radio_Monitor3, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);
    connect(ui->radio_Monitor4, &QCheckBox::clicked, this, &AddStoryDialog::set_angle);

    connect(ui->check_ProgressText, &QCheckBox::clicked, this, &AddStoryDialog::slot_progress_clicked);

    QDialog::showEvent(event);
    activateWindow();
    raise();
}

void AddStoryDialog::set_edit_mode()
{
    ui->edit_Angle->setEnabled(false);
    ui->combo_AvailableReporters->setEnabled(false);
    ui->combo_ReporterBeats->setEnabled(false);
}

void AddStoryDialog::save_settings()
{
    settings->begin_section("/AddStoryDialog");

    settings->set_item("current_tab", ui->tabWidget->currentIndex());
    QStringList id_list;
    for(int i = 0;i < ui->combo_DashboardGroupId->count() && i < 10;++i)
        id_list << ui->combo_DashboardGroupId->itemText(i);
    settings->set_item("dashboard_group_id_list", id_list);

    settings->end_section();

    if(edit_mode == Mode::Add)
    {
        auto story = ui->edit_Source->text();
        if(QFile::exists(story))
            story_info->story = QUrl::fromLocalFile(story);
        else
        {
            if(story.endsWith("/"))
                story.chop(1);
            story_info->story.setUrl(story);
        }

        story_info->angle = ui->edit_Angle->text();
        if(story_info->angle.isEmpty())
            story_info->angle = ui->edit_Angle->placeholderText();

        story_info->reporter_id = plugin_reporter->PluginID();

        store_reporter_parameters();
    }
    else
        get_reporter_parameters();

    if(!ui->edit_TTL->text().isEmpty())
        story_info->ttl = static_cast<uint>(ui->edit_TTL->text().toInt());
    else
        story_info->ttl = static_cast<uint>(ui->edit_TTL->placeholderText().toInt());
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

    story_info->include_progress_bar = ui->check_ProgressText->isChecked();
    if(ui->edit_ProgressText->text().isEmpty())
        story_info->progress_text_re = ui->edit_ProgressText->placeholderText();
    else
        story_info->progress_text_re = ui->edit_ProgressText->text();
    story_info->progress_on_top = ui->radio_ProgressOnTop->isChecked();

    story_info->entry_type = static_cast<AnimEntryType>(ui->combo_EntryType->currentIndex());
    story_info->exit_type = static_cast<AnimExitType>(ui->combo_ExitType->currentIndex());
    if(ui->edit_AnimMotionMilliseconds->text().isEmpty())
        story_info->anim_motion_duration = ui->edit_AnimMotionMilliseconds->placeholderText().toInt();
    else
        story_info->anim_motion_duration = ui->edit_AnimMotionMilliseconds->text().toInt();
    if(ui->edit_FadeTargetMilliseconds->text().isEmpty())
        story_info->fade_target_duration = ui->edit_FadeTargetMilliseconds->placeholderText().toInt();
    else
        story_info->fade_target_duration = ui->edit_FadeTargetMilliseconds->text().toInt();
    story_info->train_use_age_effect = ui->group_TrainAgeEffects->isChecked();
    story_info->train_age_effect = static_cast<AgeEffects>(ui->radio_TrainReduceOpacityFixed->isChecked() ? 1 : 2);
    if(ui->edit_TrainReduceOpacity->text().isEmpty())
        story_info->train_age_percent = ui->edit_TrainReduceOpacity->placeholderText().toInt();
    else
        story_info->train_age_percent = ui->edit_TrainReduceOpacity->text().toInt();
    story_info->dashboard_use_age_effect = ui->check_DashboardReduceOpacityFixed->isChecked();
    if(ui->edit_DashboardReduceOpacity->text().isEmpty())
        story_info->dashboard_age_percent = ui->edit_DashboardReduceOpacity->placeholderText().toInt();
    else
        story_info->dashboard_age_percent = ui->edit_DashboardReduceOpacity->text().toInt();
    auto line_edit = ui->combo_DashboardGroupId->lineEdit();
    if(line_edit->text().isEmpty())
        story_info->dashboard_group_id = line_edit->placeholderText();
    else
        story_info->dashboard_group_id = line_edit->text();
}

void AddStoryDialog::load_settings()
{
    QVector<QRadioButton*> fixed_buttons { ui->radio_HeadlinesSizeTextScale, ui->radio_HeadlinesSizeTextClip };
    QVector<QRadioButton*> age_buttons { nullptr, ui->radio_TrainReduceOpacityFixed, ui->radio_TrainReduceOpacityByAge };

    settings->begin_section("/AddStoryDialog");

    ui->tabWidget->setCurrentIndex(settings->get_item("current_tab", 0).toInt());

    auto id_list = settings->get_item("dashboard_group_id_list", QStringList()).toStringList();
    ui->combo_DashboardGroupId->addItems(id_list);

    for(auto i = 0;i < id_list.length();++i)
    {
        if(!id_list[i].compare(story_info->dashboard_group_id))
        {
            ui->combo_DashboardGroupId->setCurrentIndex(i);
            break;
        }
    }

    settings->end_section();

    if(story_info->story.isLocalFile())
    {
        ui->edit_Source->setText(QDir::toNativeSeparators(story_info->story.toLocalFile()));
        ui->edit_Source->setEnabled(false);
    }
    else
        ui->edit_Source->setText(story_info->story.toString());

    if(story_info->angle.isEmpty())
        ui->edit_Angle->setPlaceholderText(ui->edit_Source->text());
    else
        ui->edit_Angle->setText(story_info->angle);

    ui->combo_AvailableReporters->clear();

    if(!plugin_beats.contains(story_info->reporter_beat))
        story_info->reporter_beat = plugin_beats.keys()[0];

    foreach(const auto& pi_info, plugin_beats[story_info->reporter_beat])
        ui->combo_AvailableReporters->addItem(pi_info.name);

    ui->combo_ReporterBeats->clear();
    auto beats = plugin_beats.keys();
    ui->combo_ReporterBeats->addItems(beats);
    auto index_of_beat = beats.indexOf(story_info->reporter_beat);

    // this needs to be here because it will call begin_section("/AddStoryDialog")
    ui->combo_ReporterBeats->setCurrentIndex(index_of_beat);
    slot_beat_changed(index_of_beat);

    if(ui->edit_TTL->placeholderText().toUInt() == story_info->ttl)
        ui->edit_TTL->setText(QString());
    else
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

    ui->check_ProgressText->setChecked(story_info->include_progress_bar);
    if(!ui->edit_ProgressText->placeholderText().compare(story_info->progress_text_re))
        ui->edit_ProgressText->setText(QString());
    else
        ui->edit_ProgressText->setText(story_info->progress_text_re);
    ui->radio_ProgressOnTop->setChecked(story_info->progress_on_top);
    ui->radio_ProgressOnBottom->setChecked(!story_info->progress_on_top);

    slot_progress_clicked(story_info->include_progress_bar);

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

    ui->group_Train->setHidden(!IS_TRAIN(story_info->entry_type));
    ui->group_Dashboard->setHidden(!IS_DASHBOARD(story_info->entry_type));

    set_angle();

    if(!ui->edit_Angle->placeholderText().compare(story_info->angle))
        ui->edit_Angle->setText(QString());

    if(edit_mode == Mode::Edit)
        set_edit_mode();
}

void AddStoryDialog::set_display()
{
    QVector<QRadioButton*> display_buttons { ui->radio_Monitor1, ui->radio_Monitor2, ui->radio_Monitor3, ui->radio_Monitor4 };

    auto desktop = QApplication::desktop();
    auto screen_count = desktop->screenCount();

    for(auto i = 0; i < 4;++i)
    {
        if((i + 1) > screen_count)
            display_buttons[i]->setDisabled(true);
       if(i == story_info->primary_screen)
           display_buttons[i]->setChecked(true);
    }

    if(screen_count == 1)
        ui->radio_Monitor1->setDisabled(true);
}

void AddStoryDialog::set_angle()
{
    if(edit_mode != Mode::Add || !ui->edit_Angle->text().isEmpty())
        return;

    auto entry_type = static_cast<AnimEntryType>(ui->combo_EntryType->currentIndex());

    QString target;
    if(story_info->story.isLocalFile())
    {
        QFileInfo info(story_info->story.toLocalFile());
        target = info.fileName();
    }
    else
    {
        auto story_str = story_info->story.toString();
        if(story_str.endsWith("/"))
            story_str.chop(1);
        target = story_str.split("/").back();
    }

    auto requirements = plugin_reporter->Requires();

    QString plugin_params;
    auto all_edits = ui->group_ReporterConfig->findChildren<QLineEdit*>();
    foreach(auto child, all_edits)
    {
        auto name = child->objectName();
        if(name.isEmpty() || !name.startsWith(story_info->reporter_id))
            continue;

        auto index = name.split(QChar('_')).back().toInt();

        auto  value = child->text();
        if(!value.isEmpty() && !requirements[(index*2)+1].compare("string"))
        {
            if(plugin_params.length())
                plugin_params += "?";
            plugin_params += value.replace(" ", "_");
        }
    }

    auto plugin_id = QString("%1%2").arg(plugin_reporter->DisplayName()[0]).arg(plugin_params.isEmpty() ? "" : QString("(%1)").arg(plugin_params));
    auto display = QString("Display %1").arg(ui->radio_Monitor1->isChecked() ? 1 : (ui->radio_Monitor2->isChecked() ? 2 : (ui->radio_Monitor3->isChecked() ? 3 : 4)));

    if(IS_DASHBOARD(entry_type))
    {
        auto line_edit = ui->combo_DashboardGroupId->lineEdit();
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

void AddStoryDialog::get_reporter_parameters()
{
    story_info->reporter_parameters.clear();

    if(plugin_reporter.isNull())
        return;

    story_info->reporter_parameters_version = plugin_reporter->RequiresVersion();
    auto params = plugin_reporter->Requires();

    for(auto i = 0;i < params.length() / 2;++i)
        story_info->reporter_parameters << "";

    auto all_controls = ui->group_ReporterConfig->findChildren<QWidget*>();
    foreach(auto child, all_controls)
    {
        auto name = child->objectName();
        if(name.isEmpty() || !name.startsWith(story_info->reporter_id))
            continue;

        QString value;

        auto edit = qobject_cast<QLineEdit*>(child);
        if(edit)
            value = edit->text();
        else
        {
            auto multiline = qobject_cast<QPlainTextEdit*>(child);
            if(multiline)
                value = multiline->toPlainText();
            else
            {
                auto combo = qobject_cast<QComboBox*>(child);
                if(combo)
                    value = QString::number(combo->currentIndex());
                else
                {
                    auto checkbox = qobject_cast<QCheckBox*>(child);
                    if(checkbox)
                        value = checkbox->isChecked() ? "true" : "false";
                }
            }
        }

        auto index = name.split(QChar('_')).back().toInt();
        story_info->reporter_parameters[index] = value;
    }

    plugin_reporter->Secure(story_info->reporter_parameters);
}

void AddStoryDialog::store_reporter_parameters()
{
    if(plugin_reporter.isNull())
        return;

    get_reporter_parameters();

    auto parameters_filename = QDir::toNativeSeparators(QString("%1/%2")
                                                .arg(defaults_folder)
                                                .arg(mainwindow->encode_for_filesystem(story_info->reporter_id)));

    auto requires_params = plugin_reporter->Requires();

    auto params_count = requires_params.count() / 2;
    if(params_count && (params_count == story_info->reporter_parameters.count()))
    {
        auto reporter_settings = SettingsPointer(new SettingsXML("ReporterData", parameters_filename));
        reporter_settings->init(true);
        reporter_settings->set_version(story_info->reporter_parameters_version);

        reporter_settings->begin_section("/ReporterData");
          for(auto i = 0, j = 0;i < requires_params.length();i += 2, ++j)
            reporter_settings->set_item(requires_params[i], story_info->reporter_parameters[j]);
        reporter_settings->end_section();

        reporter_settings->flush();
    }
}

void AddStoryDialog::recall_reporter_parameters()
{
    if(plugin_reporter.isNull())
        return;

    story_info->reporter_parameters.clear();

    auto parameters_filename = QDir::toNativeSeparators(QString("%1/%2")
                                                .arg(defaults_folder)
                                                .arg(mainwindow->encode_for_filesystem(story_info->reporter_id)));

    auto reporter_settings = SettingsPointer(new SettingsXML("ReporterData", parameters_filename));
    if(QFile::exists(reporter_settings->get_filename()))
    {
        if(!reporter_settings->init())
        {
            // TODO: handle this error
        }
        else
        {
            auto requires_params = plugin_reporter->Requires();

            reporter_settings->begin_section("/ReporterData");
              for(auto i = 0, j = 0;i < requires_params.length();i += 2, ++j)
              {
                  story_info->reporter_parameters.append(QString());
                  story_info->reporter_parameters[j] = reporter_settings->get_item(requires_params[i], QString()).toString();
              }
            reporter_settings->end_section();
        }
    }
}

void AddStoryDialog::slot_accepted()
{
    save_settings();
}

void AddStoryDialog::slot_entry_type_changed(int index)
{
    auto entry_type = static_cast<AnimEntryType>(index);
    auto is_train = IS_TRAIN(entry_type);
    auto is_dashboard = IS_DASHBOARD(entry_type);
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

void AddStoryDialog::slot_progress_clicked(bool checked)
{
    ui->edit_ProgressText->setEnabled(checked);
    ui->radio_ProgressOnTop->setEnabled(checked);
    ui->radio_ProgressOnBottom->setEnabled(checked);
}

void AddStoryDialog::slot_train_reduce_opacity_clicked(bool /*checked*/)
{
    ui->edit_TrainReduceOpacity->setEnabled(ui->radio_TrainReduceOpacityFixed->isChecked());
}

void AddStoryDialog::slot_beat_changed(int index)
{
    // save the current reporter configuration under the current 'reporter_id'

    if(edit_mode == Mode::Add)
        store_reporter_parameters();

    story_info->reporter_beat = ui->combo_ReporterBeats->itemText(index);

    // keep signals from being fired while we're manipulating things
    QSignalBlocker blocker(ui->combo_AvailableReporters);

    ui->combo_AvailableReporters->clear();

    if(!plugin_beats.contains(story_info->reporter_beat))
        story_info->reporter_beat = plugin_beats.keys()[0];

    foreach(const auto& pi_info, plugin_beats[story_info->reporter_beat])
        ui->combo_AvailableReporters->addItem(pi_info.name);

    if(plugin_beats[story_info->reporter_beat].count() > 1)
    {
        // find the Reporter
        if(!story_info->reporter_id.isEmpty())
        {
            auto index{0};
            for(;index < plugin_beats[story_info->reporter_beat].count();++index)
            {
                auto instance = plugin_beats[story_info->reporter_beat][index].factory->instance();
                auto ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
                Q_ASSERT(ireporterfactory);
                plugin_reporter = ireporterfactory->newInstance();
                if(!story_info->reporter_id.compare(plugin_reporter->PluginID()))
                    break;
            }

            ui->combo_AvailableReporters->setCurrentIndex(index);
            ui->combo_AvailableReporters->setToolTip(plugin_beats[story_info->reporter_beat][index].tooltip);

            current_reporter_index = index;
       }
    }

    if(plugin_reporter.isNull())
    {
        ui->combo_AvailableReporters->setToolTip(plugin_beats[story_info->reporter_beat][0].tooltip);

        auto instance = plugin_beats[story_info->reporter_beat][ui->combo_AvailableReporters->currentIndex()].factory->instance();
        auto ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
        Q_ASSERT(ireporterfactory);
        plugin_reporter = ireporterfactory->newInstance();
    }

    story_info->reporter_id = plugin_reporter->PluginID();

    auto best_guess = plugin_reporter->Supports(story_info->story);
    ui->label_Success->setText(tr("Success: %1%").arg(static_cast<int>((best_guess * 100.0f))));

    if(edit_mode == Mode::Add)
        recall_reporter_parameters();

    slot_configure_reporter_configure();
}

void AddStoryDialog::slot_reporter_changed(int index)
{
    if(index == current_reporter_index)
        return;

    if(edit_mode == Mode::Add)
        store_reporter_parameters();

    auto& info = plugin_beats[story_info->reporter_beat];

    ui->combo_AvailableReporters->setToolTip(info[index].tooltip);

    auto instance = info[ui->combo_AvailableReporters->currentIndex()].factory->instance();
    auto ireporterfactory = reinterpret_cast<IReporterFactory*>(instance);
    Q_ASSERT(ireporterfactory);
    plugin_reporter = ireporterfactory->newInstance();

    story_info->reporter_id = plugin_reporter->PluginID();
    auto best_guess = plugin_reporter->Supports(story_info->story);
    ui->label_Success->setText(tr("Success: %1%").arg(static_cast<int>((best_guess * 100.0f))));

    if(edit_mode == Mode::Add)
        recall_reporter_parameters();

    slot_configure_reporter_configure();
}

void AddStoryDialog::slot_set_group_id_text(int index)
{
    auto str = ui->combo_DashboardGroupId->itemText(index);
    if(str.isEmpty())
        return;
    ui->combo_DashboardGroupId->setEditText(str);

    set_angle();
}

void AddStoryDialog::slot_update_groupid_list()
{
    auto id = ui->combo_DashboardGroupId->currentText();
    if(id.isEmpty())
        return;

    for(auto i = 0;i < ui->combo_DashboardGroupId->count();++i)
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
    // we populate group control "ReporterConfig" with controls for entering
    // Reporter configuration parameters

    auto children = ui->group_ReporterConfig->findChildren<QObject*>();
    for(auto iter = children.begin();iter != children.end();++iter)
        (*iter)->deleteLater();

    Q_ASSERT(!plugin_reporter.isNull());

    plugin_reporter->Unsecure(story_info->reporter_parameters);

    auto params = plugin_reporter->Requires();
    if(params.isEmpty())
    {
        ui->group_ReporterConfig->setHidden(true);
        return;
    }

    QVector<QWidget*> edit_fields;
    required_fields.resize(params.length() / 2);

    auto vbox = new QVBoxLayout();
    for(auto i = 0, j = 0;i < params.length();i += 2, ++j)
    {
        auto param_name(params[i]);
        if(param_name.endsWith(QChar('*')))
        {
            param_name.chop(1);
            required_fields.setBit(j);
        }
        else
            required_fields.clearBit(j);

        auto label = new QLabel(param_name);
        QLineEdit* edit{nullptr};
        QPlainTextEdit* multiline{nullptr};
        QComboBox* combo{nullptr};
        QCheckBox* checkbox{nullptr};

        QString type(params[i+1]);
        QString def_value;

        if(type.startsWith("multiline:"))
        {
            multiline = new QPlainTextEdit();
            multiline->setObjectName(QString("%1_%2_%3")
                                        .arg(story_info->reporter_id)
                                        .arg(QString(QUrl::toPercentEncoding(param_name, "", "_")))
                                        .arg(j));
            multiline->setWordWrapMode(QTextOption::NoWrap);
            if(required_fields.at(j))
                multiline->setStyleSheet("background-color: rgb(255, 245, 245);");

            def_value = type.right(type.length() - 10);
            type = "multiline";
        }
        else if(type.startsWith("combo:"))
        {
            combo = new QComboBox();
            combo->setObjectName(QString("%1_%2_%3")
                                        .arg(story_info->reporter_id)
                                        .arg(QString(QUrl::toPercentEncoding(param_name, "", "_")))
                                        .arg(j));
            def_value = type.right(type.length() - 6);
            type = "combo";
        }
        else if(type.startsWith("check:"))
        {
            checkbox = new QCheckBox();
            checkbox->setObjectName(QString("%1_%2_%3")
                                        .arg(story_info->reporter_id)
                                        .arg(QString(QUrl::toPercentEncoding(param_name, "", "_")))
                                        .arg(j));
            def_value = type.right(type.length() - 6);
            type = "check";
        }
        else    // default: QTextEdit
        {
            edit = new QLineEdit();
            edit->setObjectName(QString("%1_%2_%3")
                                        .arg(story_info->reporter_id)
                                        .arg(QString(QUrl::toPercentEncoding(param_name, "", "_")))
                                        .arg(j));
            if(required_fields.at(j))
                edit->setStyleSheet("background-color: rgb(255, 245, 245);");

            if(params[i+1].contains(':'))
            {
                auto items = params[i+1].split(':');
                type = items[0];
                def_value = items[1];
            }
        }

        if(!type.compare("multiline"))
        {
            if(story_info->reporter_parameters.count() > j && !story_info->reporter_parameters[j].isEmpty())
                multiline->insertPlainText(story_info->reporter_parameters[j]);
            else
                multiline->insertPlainText(def_value);

            edit_fields.push_back(multiline);

            auto hbox = new QHBoxLayout();
            hbox->addWidget(label);
            hbox->addWidget(multiline);

            vbox->addLayout(hbox);
        }
        else if(!type.compare("combo"))
        {
            foreach(const auto& item, def_value.split(","))
                combo->addItem(item.trimmed());
            if(story_info->reporter_parameters.count() > j && !story_info->reporter_parameters[j].isEmpty())
            {
                if(story_info->reporter_parameters[j].toInt() < combo->count())
                    combo->setCurrentIndex(story_info->reporter_parameters[j].toInt());
            }
            else if(!def_value.isEmpty())
                combo->setCurrentIndex(def_value.toInt());

            auto hbox = new QHBoxLayout();
            hbox->addWidget(label);
            hbox->addWidget(combo);

            vbox->addLayout(hbox);
        }
        else if(!type.compare("check"))
        {
            if(story_info->reporter_parameters.count() > j && !story_info->reporter_parameters[j].isEmpty())
                checkbox->setChecked(!story_info->reporter_parameters[j].toLower().compare("true"));
            else if(!def_value.isEmpty())
                checkbox->setChecked(!def_value.toLower().compare("true"));

            checkbox->setText(param_name);
            label->deleteLater();

            vbox->addWidget(checkbox);
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

            if(story_info->reporter_parameters.count() > j && !story_info->reporter_parameters[j].isEmpty())
            {
                if(!story_info->reporter_parameters[j].compare(def_value))
                    edit->setPlaceholderText(def_value);
                else
                    edit->setText(story_info->reporter_parameters[j]);
            }
            else
                edit->setPlaceholderText(def_value);

            edit_fields.push_back(edit);

            auto hbox = new QHBoxLayout();
            hbox->addWidget(label);
            hbox->addWidget(edit);

            vbox->addLayout(hbox);
        }
    }

    if(required_fields.count(true))
    {
        for(auto i = 0;i < edit_fields.count();++i)
        {
            auto edit = qobject_cast<QLineEdit*>(edit_fields[i]);
            if(edit)
            {
                if(required_fields.at(i))
                    connect(edit, &QLineEdit::textChanged, this, &AddStoryDialog::slot_config_reporter_check_required);
                else
                    connect(edit, &QLineEdit::textChanged, this, &AddStoryDialog::set_angle);
            }
            else
            {
                auto multiline = qobject_cast<QPlainTextEdit*>(edit_fields[i]);
                if(multiline && required_fields.at(i))
                    connect(multiline, &QPlainTextEdit::textChanged, this, &AddStoryDialog::slot_config_reporter_check_required);
            }
        }
    }

    vbox->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    auto layout = ui->group_ReporterConfig->layout();
    if(layout)
        delete layout;

    ui->group_ReporterConfig->setLayout(vbox);

    slot_config_reporter_check_required();
}

void AddStoryDialog::slot_config_reporter_check_required()
{
    auto have_required_fields{true};

    auto all_edits = ui->group_ReporterConfig->findChildren<QWidget*>();
    foreach(auto child, all_edits)
    {
        QPlainTextEdit* multiline{nullptr};
        QString name;

        auto edit = qobject_cast<QLineEdit*>(child);
        if(edit)
            name = edit->objectName();
        else
        {
            multiline = qobject_cast<QPlainTextEdit*>(child);
            if(multiline)
                name = multiline->objectName();
        }

        if(name.isEmpty() || !name.startsWith(story_info->reporter_id))
            continue;

        auto index = name.split(QChar('_')).back().toInt();

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
            }
        }
    }

    auto buttonBox = findChild<QDialogButtonBox*>("buttonBox");
    auto button = buttonBox->button(QDialogButtonBox::Ok);
    button->setEnabled(have_required_fields);

    set_angle();
}
