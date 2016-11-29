#pragma once

#include <QDialog>

#include <QtCore/QUrl>
#include <QtCore/QList>

#include "types.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    void            set_autostart(bool autostart);
    void            set_font(const QFont& font);
    void            set_stacking(ReportStacking stack_type);
    void            set_stories(const QList<QUrl>& stories);

    bool            get_autostart();
    QFont           get_font();
    ReportStacking  get_stacking();
    QList<QUrl>     get_stories();

protected slots:
    void            slot_update_font(const QFont& font);
    void            slot_update_font_size(int index);
    void            slot_story_update();
    void            slot_remove_story();

private:
    Ui::SettingsDialog *ui;
};
