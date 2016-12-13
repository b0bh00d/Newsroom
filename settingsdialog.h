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
    void            set_normal_stylesheet(const QString& stylesheet);
    void            set_alert_stylesheet(const QString& stylesheet);
    void            set_alert_keywords(const QStringList& alert_words);
    void            set_stacking(ReportStacking stack_type);
    void            set_stories(const QList<QString>& stories);

    bool            get_autostart();
    QFont           get_font();
    QString         get_normal_stylesheet();
    QString         get_alert_stylesheet();
    QStringList     get_alert_keywords();
    ReportStacking  get_stacking();
    QList<QString>  get_stories();

protected slots:
    void            slot_update_font(const QFont& font);
    void            slot_update_font_size(int index);
    void            slot_story_update();
    void            slot_remove_story();
    void            slot_apply_normal_stylesheet();
    void            slot_apply_alert_stylesheet();
    void            slot_apply_predefined_style(int index);

private:
    Ui::SettingsDialog *ui;
};
