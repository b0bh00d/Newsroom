#pragma once

#include <QtWidgets/QLabel>

#include <QtGui/QPaintEvent>

// https://stackoverflow.com/questions/9183050/vertical-qlabel-or-the-equivalent
class QVLabel : public QLabel
{
    Q_OBJECT
public:
    explicit QVLabel(QWidget *parent=0);
    explicit QVLabel(const QString &text, QWidget *parent=0);

protected:
    void paintEvent(QPaintEvent*);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
};
