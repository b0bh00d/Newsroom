#ifdef QT_WIN
#include <windows.h>
#endif

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGraphicsOpacityEffect>

#include <QtGui/QFontMetrics>
#include <QtGui/QTextDocument>

#include <QtCore/QUrl>
#include <QtCore/QPropertyAnimation>

#include "headline.h"
#include "label.h"

Headline::Headline(const QUrl& story,
                 const QString& headline,
                 AnimEntryType entry_type,
                 Qt::Alignment alignment,
                 QWidget* parent)
    : stay_visible(false),
      mouse_in_widget(false),
      is_zoomed(false),
      compact_mode(false),
      margin(5),
      ignore(false),
      story(story),
      headline(headline),
      old_opacity(1.0),
      animation(nullptr),
      entry_type(entry_type),
      alignment(alignment),
      include_progress_bar(false),
      progress_on_top(false),
      hover_timer(nullptr),
      bottom_window(nullptr),
      QWidget(parent)
{
    hover_timer = new QTimer(this);
    if(hover_timer)
        connect(hover_timer, &QTimer::timeout, this, &Headline::slot_zoom_in);
}

Headline::~Headline()
{
    if(hover_timer)
    {
        hover_timer->stop();
        hover_timer->deleteLater();
        hover_timer = nullptr;
    }
}

bool Headline::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    *result = 0L;

    if(!stay_visible)
    {
#ifdef QT_WIN
        if(!QString(eventType).compare("windows_generic_MSG"))
        {
            MSG* msg = (MSG*)message;

            if(msg->message == WM_WINDOWPOSCHANGING)
            {
                RECT    r;
                HWND    bottom = nullptr;

                // assume the first window in the Z order that is consuming
                // the entire virtual desktop size IS the desktop (e.g.,
                // "Program Manager"), and glue ourselves right on top of it
                int desktop_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                int desktop_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                HWND w = GetTopWindow(NULL);
                while(w)
                {
                    if(!bottom_window)
                    {
                        GetWindowRect(w, &r);
                        if((r.right - r.left) == desktop_width && (r.bottom - r.top) == desktop_height)
                            break;
                    }
                    else if((HWND)bottom_window->winId() == w)
                        break;

                    if(IsWindowVisible(w))
                        bottom = w;

                    w = GetNextWindow(w, GW_HWNDNEXT);
                }

                auto pwpos = (WINDOWPOS*)msg->lParam;
                pwpos->hwndInsertAfter = bottom;
                pwpos->flags &= (~SWP_NOZORDER);
                // fall through to the return
            }
        }
#endif
    }

    return false;
}

void Headline::enterEvent(QEvent *event)
{
    mouse_in_widget = true;
    if(!compact_mode)
        emit signal_mouse_enter();
    else
    {
        hover_timer->setInterval(1000);
        hover_timer->start();
    }
    event->accept();
}

void Headline::moveEvent(QMoveEvent* /*event*/)
{
    if(!compact_mode)
        return;
}

void Headline::leaveEvent(QEvent *event)
{
    mouse_in_widget = false;
    if(!compact_mode)
        emit signal_mouse_exit();
    else
    {
        if(hover_timer)
            hover_timer->stop();
        zoom_out();
    }
    event->accept();
}

void Headline::set_progress(bool include, const QString& re, bool on_top)
{
    include_progress_bar = include;
    progress_text_re = re;
    progress_on_top = on_top;
}

void Headline::set_compact_mode(bool compact, int original_width, int original_height)
{
    compact_mode = compact;
    original_w = original_width;
    original_h = original_height;
}

void Headline::initialize(bool stay_visible, FixedText fixed_text, int width, int height)
{
    this->stay_visible = stay_visible;

    // https://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
    if(stay_visible)
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    else
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);

//    setParent(0); // Create TopLevel-Widget
    setAttribute(Qt::WA_ShowWithoutActivating);
//    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
//    setAttribute(Qt::WA_PaintOnScreen); // not needed in Qt 5.2 and up

    if(width < height)
    {
        label = new VLabel(this);
        dynamic_cast<VLabel*>(label)->set_for_left(entry_type != AnimEntryType::DashboardInRightTop && entry_type != AnimEntryType::DashboardInRightBottom);
    }
    else
    {
        label = new HLabel(this);
        dynamic_cast<HLabel*>(label)->set_progress_detection(include_progress_bar, progress_text_re, progress_on_top);
    }

    dynamic_cast<ILabel*>(label)->set_shrink_to_fit(fixed_text == FixedText::ScaleToFit);
    dynamic_cast<ILabel*>(label)->set_compact_mode(compact_mode);

    label->setAlignment(alignment);
    label->setContentsMargins(0, 0, 0, 0);
    label->setTextFormat(Qt::AutoText);
    label->setMargin(margin);
    label->setFont(font);
    label->setStyleSheet(stylesheet);
    label->setText(headline);

    QRect r = geometry();
    setGeometry(r.x(), r.y(), width, height);
    label->setGeometry(0, 0, width, height);
}

void Headline::zoom_out()
{
    if(mouse_in_widget || !is_zoomed)
        return;

    dynamic_cast<ILabel*>(label)->set_compact_mode(compact_mode);

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry", this);
    animation->setDuration(200);
    animation->setStartValue(QRect(0, 0, original_w, original_h));
    animation->setEndValue(QRect(0, 0, georect.width(), georect.height()));
    animation->setEasingCurve(QEasingCurve::InCubic);
    connect(animation, &QPropertyAnimation::finished, this, &Headline::slot_turn_on_compact_mode);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    is_zoomed = false;
}

void Headline::slot_turn_on_compact_mode()
{
    stay_visible = was_stay_visible;
    if(!stay_visible)
        lower();

    setGeometry(QRect(georect.x(), georect.y(), georect.width(), georect.height()));
    label->repaint();
}

void Headline::slot_zoom_in()
{
    hover_timer->stop();

    if(!mouse_in_widget || is_zoomed)
        return;

    georect = geometry();
    setGeometry(QRect(georect.x(), georect.y(), original_w, original_h));

    QPropertyAnimation* animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(200);
    animation->setStartValue(QRect(0, 0, georect.width(), georect.height()));
    animation->setEndValue(QRect(0, 0, original_w, original_h));
    animation->setEasingCurve(QEasingCurve::InCubic);
    connect(animation, &QPropertyAnimation::finished, this, &Headline::slot_turn_off_compact_mode);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    is_zoomed = true;
}

void Headline::slot_turn_off_compact_mode()
{
    was_stay_visible = stay_visible;
    stay_visible = true;
    raise();

    dynamic_cast<ILabel*>(label)->set_compact_mode(false);
    label->repaint();
}
