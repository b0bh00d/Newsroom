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

class AddLocalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddLocalDialog(QWidget *parent = 0);
    ~AddLocalDialog();

    void            set_target(const QString& name);
    void            set_trigger(LocalTrigger trigger_type);
    void            set_ttl(uint ttl);
    void            set_always_visible(bool visible);
    void            set_screens(int primary_screen, int screen_count);
    void            set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type);
    void            set_train_fixed_width(int width = 0);
    void            set_train_age_effects(AgeEffects effect, int percent = 0);

    int             get_display();
    LocalTrigger    get_trigger();
    uint            get_ttl();
    bool            get_always_visible();
    AnimEntryType   get_animation_entry_type();
    AnimExitType    get_animation_exit_type();
    int             get_train_fixed_width();
    AgeEffects      get_train_age_effects(int& percent);

protected:
    void            showEvent(QShowEvent *);

protected slots:
    void            slot_entry_type_changed(int index);
    void            slot_train_fixed_width_clicked(bool checked);
    void            slot_train_reduce_opacity_clicked(bool checked);

private:
    Ui::AddLocalDialog *ui;
};
