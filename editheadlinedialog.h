#pragma once

#include <QtWidgets/QDialog>

#include <QtGui/QFont>

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Ui {
class EditHeadlineDialog;
}

class EditHeadlineDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditHeadlineDialog(QWidget *parent = nullptr);
    ~EditHeadlineDialog();

    void            set_style_name(const QString& name);
    void            set_style_font(const QFont& font);
    void            set_style_stylesheet(const QString& stylesheet);
    void            set_style_triggers(const QStringList& triggers);

    QString         get_style_name() const;
    QString         get_style_stylesheet() const;
    QStringList     get_style_triggers() const;

private slots:
//    void            slot_update_font(const QFont& font);
//    void            slot_update_font_size(int index);
    void            slot_apply_stylesheet();
    void            slot_update_ok();

private:
    Ui::EditHeadlineDialog *ui{nullptr};
};
