#pragma once

#include <QtWidgets/QDialog>
#include <QtGui/QShowEvent>
#include <QtGui/QFont>

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
    void            set_font(const QFont& font);
    void            set_always_visible(bool visible);
    void            set_screens(int primary_screen, int screen_count);
    void            set_animation_entry_and_exit(AnimEntryType entry_type, AnimExitType exit_type);
    void            set_stacking(ReportStacking stack_type);

    int             get_display();
    LocalTrigger    get_trigger();
    uint            get_ttl();
    QFont           get_font();
    bool            get_always_visible();
    AnimEntryType   get_animation_entry_type();
    AnimExitType    get_animation_exit_type();
    ReportStacking  get_stacking();

protected:
    void            showEvent(QShowEvent *);

protected slots:
    void            slot_update_font(const QFont& font);
    void            slot_update_font_size(int index);

private:
    Ui::AddLocalDialog *ui;
};
