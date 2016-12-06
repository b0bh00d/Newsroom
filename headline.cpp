//#include <windows.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

#include <QtGui/QFontMetrics>

#include <QtCore/QUrl>

#include "headline.h"

Headline::Headline(const QUrl& story,
                 const QString& headline,
                 QWidget* parent)
    : margin(5),
      ignore(false),
      story(story),
      headline(headline),
      QWidget(parent)
{
}

//void Headline::showEvent(QShowEvent *event)
//{
//    QWidget::showEvent(event);

////    SetWindowLong((HWND)winId(), GWL_STYLE, GetWindowLong((HWND)winId(), GWL_STYLE) | WS_EX_TRANSPARENT);

////    QRect r = geometry();
////    SetWindowPos((HWND)winId(), HWND_BOTTOM, r.x(), r.y(), r.width(), r.height(), SWP_NOACTIVATE|SWP_SHOWWINDOW);
////    SetWindowPos((HWND)winId(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
//}

void Headline::initialize(bool stay_visible, FixedText fixed_text, int width, int height)
{
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
    label->setMargin(margin);
    label->setFont(font);

    QString stylesheet = normal_stylesheet;
    foreach(const QString& keyword, alert_keywords)
    {
        if(headline.contains(keyword))
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

//bool Headline::winEvent(MSG* message, long* result)
//{
//}
