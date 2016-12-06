#pragma once

#include <QtWidgets/QDialog>
#include <QtGui/QShowEvent>
#include <QtGui/QFont>

#include "types.h"

namespace Ui {
class AddLocalDialog;
}

/// @class AddLocalDialog
/// @brief A dialog for configuration a local Reporter instance.
///
/// When a local story is submitted to the Newsroom, a local Reporter
/// instance is assigned to cover it.  This dialog gathers information
/// for the Reporter when it submits Headlines for display on the Chyron.

/*
*/

class AddLocalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddLocalDialog(QWidget *parent = 0);
    ~AddLocalDialog();

    void            set_target(const QString& name);
                    // Notifications
    void            set_trigger(LocalTrigger trigger_type);
    void            set_ttl(uint ttl);
                    // Presentation
                    //   Display
    void            set_display(int primary_screen, int screen_count);
                    //   Headlines
    void            set_headlines_always_visible(bool visible);
                    //   Size
    void            set_headlines_lock_width(int width = 0);
    void            set_headlines_lock_height(int height = 0);
    void            set_headlines_fixed_text(FixedText fixed_type = FixedText::None);
                    // Animation
    void            set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type);
                    //   Train
                    //     Age Effects
    void            set_train_age_effects(AgeEffects effect = AgeEffects::None, int percent = 0);

    LocalTrigger    get_trigger();
    uint            get_ttl();

    int             get_display();
    bool            get_headlines_always_visible();
    int             get_headlines_lock_width();
    int             get_headlines_lock_height();
    FixedText       get_headlines_fixed_text();

    AnimEntryType   get_animation_entry_type();
    AnimExitType    get_animation_exit_type();
    AgeEffects      get_train_age_effects(int& percent);

protected:
    void            showEvent(QShowEvent *);

protected slots:
    void            slot_entry_type_changed(int index);
    void            slot_headlines_fixed_width_clicked(bool checked);
    void            slot_headlines_fixed_height_clicked(bool checked);
    void            slot_train_reduce_opacity_clicked(bool checked);

private:
    Ui::AddLocalDialog *ui;
};
