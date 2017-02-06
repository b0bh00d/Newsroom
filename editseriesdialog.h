#ifndef EDITSERIESDIALOG_H
#define EDITSERIESDIALOG_H

#include <QDialog>

namespace Ui {
class EditSeriesDialog;
}

class EditSeriesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditSeriesDialog(QWidget *parent = 0);
    ~EditSeriesDialog();

    void            set_compact_mode(bool compact_mode, int zoom_percent = 25);
    bool            get_compact_mode(int& zoom_percent);

private slots:
    void            slot_compact_mode_clicked(bool);

private:
    Ui::EditSeriesDialog *ui;
};

#endif // EDITSERIESDIALOG_H
