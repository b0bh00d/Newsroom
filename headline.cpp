#include <windows.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGraphicsOpacityEffect>

#include <QtGui/QFontMetrics>

#include <QtCore/QUrl>

#include "headline.h"

Headline::Headline(const QUrl& story,
                 const QString& headline,
                 QWidget* parent)
    : stay_visible(false),
      margin(5),
      ignore(false),
      story(story),
      headline(headline),
      old_opacity(1.0),
      QWidget(parent)
{
}

bool Headline::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    *result = 0L;

    if(!stay_visible)
    {
        if(!QString(eventType).compare("windows_generic_MSG"))
        {
            MSG* msg = (MSG*)message;

            if(msg->message == WM_WINDOWPOSCHANGING)
            {
                RECT    r;
                HWND    bottom = nullptr;

                // assume the first window consuming the entire virtual
                // desktop size IS the desktop (e.g., "Program Manager"),
                // and glue ourselves right on top of it
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
    }

    return false;
}

void Headline::enterEvent(QEvent *event)
{
    QGraphicsEffect* eff = graphicsEffect();
    if(eff)
    {
        QGraphicsOpacityEffect* oeff = qobject_cast<QGraphicsOpacityEffect*>(eff);
        if(oeff)
        {
            old_opacity = oeff->opacity();
            oeff->setOpacity(1.0);
        }
    }

    event->accept();
}

void Headline::leaveEvent(QEvent *event)
{
    QGraphicsEffect* eff = graphicsEffect();
    if(eff)
    {
        QGraphicsOpacityEffect* oeff = qobject_cast<QGraphicsOpacityEffect*>(eff);
        if(oeff)
            oeff->setOpacity(old_opacity);
    }

    event->accept();
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

    label = new QLabel(this);
    label->setTextFormat(Qt::AutoText);
    label->setMargin(margin);
    label->setFont(font);

    QString stylesheet = normal_stylesheet;
    QString lower_headline = headline.toLower();
    foreach(const QString& keyword, alert_keywords)
    {
        if(lower_headline.contains(keyword.toLower()))
        {
            stylesheet = alert_stylesheet;
            break;
        }
    }

    label->setStyleSheet(stylesheet);

    if(fixed_text == FixedText::None)
    {
        QVBoxLayout *main_layout = new QVBoxLayout;
        main_layout->setContentsMargins(0, 0, 0, 0);

        label->setText(headline);

        main_layout->addWidget(label);

        setLayout(main_layout);
        adjustSize();

        return;
    }

    QFont f = label->font();
    QStringList lines = headline.split('\n');

    if(fixed_text == FixedText::ClipToFit)
    {
        // we destructively adjust the line width, if it exceeds the dimensions
        QFontMetrics metrics(f);

        bool too_tall = ((metrics.height() * lines.count()) > height);
        while(too_tall)
        {
            lines.pop_back();
            too_tall = ((metrics.height() * lines.count()) > height);
        }

        int l, t, r, b;
        label->getContentsMargins(&l, &t, &r, &b);
        for(int i = 0;i < lines.count();++i)
            lines[i] = metrics.elidedText(lines[i], Qt::ElideRight, width - (l + r + margin * 2));
    }
    else if(fixed_text == FixedText::ScaleToFit)
    {
        // we adjust the font size to fit the dimensions, if they exceed it
        for(;;)
        {
            QFontMetrics metrics(f);

            bool too_wide = false;
            bool too_tall = ((metrics.height() * lines.count()) > height);
            if(!too_tall)
            {
                for(int i = 0;i < lines.count() && !too_wide;++i)
                {
                    int w = metrics.width(lines[i]);
                    too_wide = (w > width);
                }
            }

            if(!too_tall && !too_wide)
            {
                label->setFont(f);
                break;
            }

            f.setPointSize(f.pointSize() - 1);
        }
    }

    label->setText(lines.join('\n'));
    QRect r = geometry();
    setGeometry(r.x(), r.y(), width, height);
    label->setGeometry(0, 0, width, height);
}
