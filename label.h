#pragma once

#include <QtWidgets/QLabel>

#include <QtGui/QPaintEvent>

class ILabel : public QLabel
{
    Q_OBJECT
public:
    explicit ILabel(QWidget *parent = 0)
        : margin(5), shrink_text_to_fit(false), compact_mode(false), QLabel(parent) {}
    explicit ILabel(const QString &text, QWidget *parent=0)
        : margin(5), shrink_text_to_fit(false), compact_mode(false), QLabel(text, parent) {}

    void    set_shrink_to_fit(bool shrink = true) { shrink_text_to_fit = shrink; }
    void    set_compact_mode(bool compact = true) { compact_mode = compact; }

protected:
    int     margin;
    bool    shrink_text_to_fit;
    bool    compact_mode;
};

// https://stackoverflow.com/questions/9183050/vertical-qlabel-or-the-equivalent
class VLabel : public ILabel
{
    Q_OBJECT
public:
    explicit VLabel(QWidget *parent=0);
    explicit VLabel(const QString &text, bool configure_for_left = true, QWidget *parent=0);

    void    set_for_left(bool for_left = true) { configure_for_left = for_left; }

protected:      // methods
    void    paintEvent(QPaintEvent*);
    QSize   sizeHint() const;
    QSize   minimumSizeHint() const;

protected:      // data members
    bool    configure_for_left;
};

class HLabel : public ILabel
{
    Q_OBJECT
public:
    explicit HLabel(QWidget *parent=0);
    explicit HLabel(const QString &text, QWidget *parent=0);

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

    int     progress_x;
    int     progress_y;
    int     progress_w;
    int     progress_h;

    QColor  progress_color, progress_highlight;
};
