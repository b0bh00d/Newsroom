#pragma once

#include <QtWidgets/QDialog>

#include <QtGui/QShowEvent>
#include <QtGui/QFont>

#include <QtCore/QBitArray>
#include <QtCore/QSettings>

#include "types.h"
#include "specialize.h"

#include "reporters/interfaces/iplugin"

namespace Ui {
class AddStoryDialog;
}

/// @class AddLocalDialog
/// @brief A dialog for configuration a local Reporter instance.
///
/// When a local story is submitted to the Newsroom, a local Reporter
/// instance is assigned to cover it.  This dialog gathers information
/// for the Reporter when it submits Headlines for display on the Chyron.

/*
*/

class AddStoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddStoryDialog(QWidget *parent = 0);
    ~AddStoryDialog();

    void            save_defaults(QSettings* settings);
    void            load_defaults(QSettings* settings);

    void            set_target(const QUrl& story);
                    // Notifications
    void            set_trigger(LocalTrigger trigger_type);
    void            set_reporters(PluginsInfoVector* reporters_info);
    void            set_ttl(uint ttl);
                    // Presentation
                    //   Display
    void            set_display(int primary_screen, int screen_count);
                    //   Headlines
    void            set_headlines_always_visible(bool visible);
                    //   Size
    void            set_headlines_lock_size(int width = 0, int height = 0);
    void            set_headlines_fixed_text(FixedText fixed_type = FixedText::None);
                    // Animation
    void            set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type);
                    //   Train
                    //     Age Effects
    void            set_train_age_effects(AgeEffects effect = AgeEffects::None, int percent = 0);

    void            configure_for_local(bool for_local = true);

    // The 'story identity' is something that will uniquely identify this
    // story.  In the case of a local file system entity, the full path to
    // that entity will suffice.  However, in the case of a REST URL, the
    // URL itself will not be unique enough as it will simply be the base
    // value for addition API selectors.  So, this function will return a
    // string value that most uniquely represents this particular story.
    QString         get_story_identity();

    QUrl            get_target();
    LocalTrigger    get_trigger();
    IPluginPointer  get_reporter() const { return plugin_reporter; }
    QStringList     get_reporter_parameters() const { return reporter_configuration; }
    uint            get_ttl();

    int             get_display();
    bool            get_headlines_always_visible();
    bool            get_headlines_lock_size(int& width, int& height);
    FixedText       get_headlines_fixed_text();

    AnimEntryType   get_animation_entry_type();
    AnimExitType    get_animation_exit_type();
    AgeEffects      get_train_age_effects(int& percent);

protected:
    void            showEvent(QShowEvent *);

protected slots:
    void            slot_entry_type_changed(int index);
    void            slot_headlines_fixed_size_clicked(bool checked);
    void            slot_train_reduce_opacity_clicked(bool checked);
    void            slot_trigger_changed(int index);
    void            slot_reporter_changed(int index);
    void            slot_configure_reporter(bool checked);
    void            slot_config_reporter_check_required();

private:
    Ui::AddStoryDialog *ui;

    QStringList         plugin_paths;
    QStringList         plugin_tooltips;

    QDialog*            params_dialog;
    QBitArray           required_fields;
    QStringList         reporter_configuration;

    PluginsInfoVector*  plugin_factories;
    IPluginPointer      plugin_reporter;

    QUrl                story;
};
