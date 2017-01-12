#pragma once

#include <QtWidgets/QDialog>

#include <QtGui/QShowEvent>
#include <QtGui/QFont>

#include <QtCore/QBitArray>
#include <QtCore/QSettings>

#include "types.h"
#include "specialize.h"
#include "storyinfo.h"
#include "settings.h"

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
    explicit AddStoryDialog(BeatsMap &beats_map,
                            StoryInfoPointer story_info,
                            SettingsPointer settings,
                            QWidget *parent = 0);
    ~AddStoryDialog();

    /*!
      When an existing Story is edited, its original "angle" (i.e., unique
      identity), generated at the time it was created, must remain unmodified.
      This method signals to the AddStoryDialog that this should be the case
      (i.e., this is an edit, not a creation, of a Story) so it will prevent
      that "angle" from being altered.
     */
    void            lock_angle();

protected:
    void            showEvent(QShowEvent *);

protected slots:
    void            slot_accepted();
    void            slot_entry_type_changed(int index);
    void            slot_interpret_size_clicked(bool checked);
    void            slot_progress_clicked(bool checked);
    void            slot_train_reduce_opacity_clicked(bool checked);
    void            slot_beat_changed(int index);
    void            slot_reporter_changed(int index);
    void            slot_config_reporter_check_required();
    void            slot_set_group_id_text(int index);
    void            slot_update_groupid_list();
    void            slot_configure_reporter_configure();

private:        // methods
    void            save_settings();
    void            load_settings();

    void            set_display();
    void            set_angle();

    void            get_reporter_parameters();
    void            store_reporter_parameters();
    void            recall_reporter_parameters();

private:
    Ui::AddStoryDialog *ui;

    StoryInfoPointer    story_info;
    SettingsPointer     settings;

    QStringList         plugin_paths;
    QStringList         plugin_tooltips;

    QBitArray           required_fields;
//    QStringList         reporter_configuration;

    BeatsMap&           plugin_beats;
    IReporterPointer    plugin_reporter;

    QUrl                story;

    bool                angle_is_locked;

    int                 current_reporter_index;
};
