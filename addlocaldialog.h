#pragma once

#include <QDialog>
#include <QShowEvent>

#include "types.h"

namespace Ui {
class AddLocalDialog;
}

class AddLocalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddLocalDialog(QWidget *parent = 0);
    ~AddLocalDialog();

    void            set_target(const QString& name);
    void            set_trigger(LocalTrigger trigger_type);
    void            set_ttl(uint ttl);
    void            set_screens(int primary_screen, int screen_count);
    void            set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type);
    void            set_stacking(ReportStacking stack_type);

    int             get_screen();
    LocalTrigger    get_trigger();
    uint            get_ttl();
    AnimEntryType   get_animation_entry_type();
    AnimExitType    get_animation_exit_type();
    ReportStacking  get_stacking();

protected:
    void            showEvent(QShowEvent *);

private:
    Ui::AddLocalDialog *ui;
};
