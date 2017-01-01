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

#include "headline.h"
#include "label.h"

Headline::Headline(const QUrl& story,
                 const QString& headline,
                 AnimEntryType entry_type,
                 Qt::Alignment alignment,
                 QWidget* parent)
    : stay_visible(false),
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
      QWidget(parent)
{
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
                    GetWindowRect(w, &r);
                    if((r.right - r.left) == desktop_width && (r.bottom - r.top) == desktop_height)
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
    emit signal_mouse_enter();
    event->accept();
}

void Headline::leaveEvent(QEvent *event)
{
    emit signal_mouse_exit();
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
