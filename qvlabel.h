#pragma once

#include <QtWidgets/QLabel>

#include <QtGui/QPaintEvent>

class NewsroomLabel : public QLabel
{
    Q_OBJECT
public:
    explicit NewsroomLabel(QWidget *parent = 0)
        : shrink_text_to_fit(false), QLabel(parent) {}
    explicit NewsroomLabel(const QString &text, QWidget *parent=0)
        : shrink_text_to_fit(false), QLabel(text, parent) {}

    virtual void shrink_to_fit(bool shrink = true) = 0;

protected:
    bool    shrink_text_to_fit;
};

// https://stackoverflow.com/questions/9183050/vertical-qlabel-or-the-equivalent
class QVLabel : public NewsroomLabel
{
    Q_OBJECT
public:
    explicit QVLabel(QWidget *parent=0);
    explicit QVLabel(const QString &text, bool configure_for_left = true, QWidget *parent=0);

    void    shrink_to_fit(bool shrink = true) Q_DECL_OVERRIDE { shrink_text_to_fit = shrink; }

    void    set_for_left(bool for_left = true) { configure_for_left = for_left; }

protected:      // methods
    void    paintEvent(QPaintEvent*);
    QSize   sizeHint() const;
    QSize   minimumSizeHint() const;

protected:      // data members
    bool    configure_for_left;
};

class QHLabel : public NewsroomLabel
{
    Q_OBJECT
public:
    explicit QHLabel(QWidget *parent=0);
    explicit QHLabel(const QString &text, QWidget *parent=0);

    void    shrink_to_fit(bool shrink = true) Q_DECL_OVERRIDE { shrink_text_to_fit = shrink; }

    void    set_progress_detection(bool detect, const QString& re, bool on_top);

protected:      // methods
    void    init();

    void    changeEvent(QEvent* event) Q_DECL_OVERRIDE;
    void    paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    QSize   sizeHint() const  Q_DECL_OVERRIDE;
    QSize   minimumSizeHint() const  Q_DECL_OVERRIDE;

protected:      // data members
    bool    detect_progress;
    QString progress_re;
    bool    progress_on_top;

    int     progress_margin;
    int     progress_x;
    int     progress_y;
    int     progress_w;
    int     progress_h;

    QColor  progress_color, progress_highlight;
};