#pragma once

#include <QDialog>

namespace Ui {
class EditSeriesDialog;
}

class EditSeriesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditSeriesDialog(QWidget *parent = nullptr);
    ~EditSeriesDialog();

    void            set_compact_mode(bool compact_mode, int zoom_percent = 25);
    bool            get_compact_mode(int& zoom_percent) const;

private slots:
    void            slot_compact_mode_clicked(bool);

private:
    Ui::EditSeriesDialog *ui{nullptr};
};
