#pragma once

#include <QDialog>

#include <QtCore/QUrl>
#include <QtCore/QList>

#include "types.h"
#include "specialize.h"
#include "seriesinfo.h"    // -> staffinfo.h -> producer.h

namespace Ui {
class SettingsDialog;
}

class QTreeWidgetItem;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:     // typdefs and enums
    SPECIALIZE_PAIR(QString, QStringList, Series)       // "SeriesPair"
    SPECIALIZE_LIST(SeriesPair, Series)                 // "SeriesList"

public:     // methods
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    void            set_autostart(bool autostart);
    void            set_continue_coverage(bool continue_coverage);
    void            set_compact_mode(bool compact_mode, int zoom_percent = 25);
    void            set_font(const QFont& font);
    void            set_styles(const HeadlineStyleList& style_list);
    void            set_stories(const QList<QString>& stories, const QList<ProducerPointer> producers);
    void            set_series(const SeriesMap& series);

    bool            get_autostart();
    bool            get_continue_coverage();
    bool            get_compact_mode(int& zoom_percent);
    QFont           get_font();
    void            get_styles(HeadlineStyleList& style_list);
    SeriesList      get_series();

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
    void            slot_remove_story_all();
    void            slot_compact_mode_clicked(bool);
    void            slot_rename_series(QTreeWidgetItem*, int);
    void            slot_series_renamed(QTreeWidgetItem*, int);

private:    // typedefs and enums
    SPECIALIZE_MAP(QString, ProducerPointer, Producer)     // "ProducerMap"

private:    // data members
    Ui::SettingsDialog *ui;

    ProducerMap     producers;

    QString         original_series_name;
    bool            editing;
};
