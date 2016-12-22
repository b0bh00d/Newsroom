#pragma once

#include <QtWidgets/QDialog>

#include <QtGui/QShowEvent>
#include <QtGui/QFont>

#include <QtCore/QBitArray>
#include <QtCore/QSettings>

#include "types.h"
#include "specialize.h"
#include "storyinfo.h"

#include "reporters/interfaces/ireporter.h"

namespace Ui {
class AddStoryDialog;
}

/// @class AddLocalDialog
/// @brief A dialog for configurating a Story instance.
///
/// When a Story is submitted to the Newsroom, a Producer and a Reporter
/// needs to be assigned to cover it.  This dialog gathers information
/// from the user for customizing how the Reporter and the Producer will
/// display Headlines on the Chyron, and how and where the Chyron will be
/// displayed on the screen.

class AddStoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddStoryDialog(PluginsInfoVector* reporters_info,
                            StoryInfoPointer story_info,
                            SettingsPointer settings,
                            QWidget *parent = 0);
    ~AddStoryDialog();

//    void            set_story(const QUrl& story);
                    // Notifications
//    void            set_trigger(LocalTrigger trigger_type);
//    void            set_reporters(PluginsInfoVector* reporters_info);
//    void            set_ttl(uint ttl);
                    // Presentation
                    //   Display
//    void            set_display();
                    //   Headlines
//    void            set_headlines_always_visible(bool visible);
                    //   Size
//    void            set_headlines_size(int width = 0, int height = 0);
//    void            set_headlines_size(double width = 0.0, double height = 0.0);
                    //   Visibility
//    void            set_limit_content(int lines = 0);
//    void            set_headlines_fixed_text(FixedText fixed_type = FixedText::None);
                    // Animation
//    void            set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type);
//    void            set_anim_motion_duration(int duration = 0);
//    void            set_fade_target_duration(int duration = 0);
                    //   Train
                    //     Age Effects
//    void            set_train_age_effects(AgeEffects effect = AgeEffects::None, int percent = 0);
                    //   Dashboard
                    //     Age Effects
//    void            set_dashboard_age_effects(int percent = 0);
                    //     Group ID
//    void            set_dashboard_group_id(const QString& group_id);

    // The 'story identity' is something that will uniquely identify this
    // story.  In the case of a local file system entity, the full path to
    // that entity will suffice.  However, in the case of a REST URL, the
    // URL itself will not be unique enough as it will simply be the base
    // value for addition API selectors.  So, this function will return a
    // string value that most uniquely represents this particular story.
//    QString         get_story_identity();

//    QUrl            get_target();
//    LocalTrigger    get_trigger();
//    IReporterPointer  get_reporter() const { return plugin_reporter; }
//    QStringList     get_reporter_parameters() const { return reporter_configuration; }
//    uint            get_ttl();

//    int             get_display();
//    bool            get_headlines_always_visible();
//    bool            get_headlines_size(int& width, int& height);
//    bool            get_headlines_size(double& width, double& height);
//    bool            get_limit_content(int& lines);
//    FixedText       get_headlines_fixed_text();

//    AnimEntryType   get_animation_entry_type();
//    AnimExitType    get_animation_exit_type();
//    int             get_anim_motion_duration();
//    int             get_fade_target_duration();
//    AgeEffects      get_train_age_effects(int& percent);

//    bool            get_dashboard_age_effects(int& percent);
//    QString         get_dashboard_group_id();

protected:
    void            showEvent(QShowEvent *);

protected slots:
    void            slot_accepted();
    void            slot_entry_type_changed(int index);
    void            slot_interpret_size_clicked(bool checked);
    void            slot_train_reduce_opacity_clicked(bool checked);
    void            slot_trigger_changed(int index);
    void            slot_reporter_changed(int index);
    void            slot_config_reporter_check_required();
    void            slot_set_group_id_text(int index);
    void            slot_update_groupid_list();
    void            slot_configure_reporter_configure();

private:        // methods
    void            save_settings();
    void            load_settings();

    void            configure_for_local(bool for_local = true);
    void            set_display();
    void            set_angle();

private:
    Ui::AddStoryDialog *ui;

    StoryInfoPointer    story_info;
    SettingsPointer     settings;

    QStringList         plugin_paths;
    QStringList         plugin_tooltips;

    QBitArray           required_fields;
    QStringList         reporter_configuration;

    PluginsInfoVector*  plugin_factories;
    IReporterPointer      plugin_reporter;

    QUrl                story;
};
