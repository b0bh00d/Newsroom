#pragma once

#include <QDialog>

#include <QtCore/QUrl>
#include <QtCore/QList>

#include "types.h"
#include "producer.h"

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
    void            set_continue_coverage(bool continue_coverage);
    void            set_compact_mode(bool compact_mode);
    void            set_font(const QFont& font);
    void            set_styles(const HeadlineStyleList& style_list);
    void            set_stories(const QList<QString>& stories, const QList<ProducerPointer> producers);

    bool            get_autostart();
    bool            get_continue_coverage();
    bool            get_compact_mode();
    QFont           get_font();
    void            get_styles(HeadlineStyleList& style_list);
    QList<QString>  get_stories();

signals:
    void            signal_edit_story(const QString& story_id);

protected slots:
    void            slot_update_font(const QFont& font);
    void            slot_update_font_size(int index);
    void            slot_add_style();
    void            slot_delete_style();
    void            slot_edit_style();
    void            slot_apply_stylesheet();
    void            slot_story_selection_changed();
    void            slot_edit_story();
    void            slot_start_coverage();
    void            slot_stop_coverage();
    void            slot_start_coverage_all();
    void            slot_stop_coverage_all();
    void            slot_remove_story();

private:
    Ui::SettingsDialog *ui;
};
