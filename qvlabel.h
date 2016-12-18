#pragma once

#include <QtWidgets/QLabel>

#include <QtGui/QPaintEvent>

// https://stackoverflow.com/questions/9183050/vertical-qlabel-or-the-equivalent
class QVLabel : public QLabel
{
    Q_OBJECT
public:
    explicit QVLabel(QWidget *parent=0);
    explicit QVLabel(const QString &text, bool configure_for_left = true, QWidget *parent=0);

    void    set_for_left(bool for_left = true) { configure_for_left = for_left; }

protected:      // methods
    void paintEvent(QPaintEvent*);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

protected:      // data members
    bool    configure_for_left;
};
