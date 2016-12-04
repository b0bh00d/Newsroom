//#include <windows.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

#include <QtCore/QUrl>

#include "headline.h"

Headline::Headline(const QUrl& story,
                 const QString& headline,
                 QWidget* parent)
    : ignore(false),
      story(story),
      headline(headline),
      QWidget(parent)
{
}

//void Headline::showEvent(QShowEvent *event)
//{
//    QWidget::showEvent(event);

////    QRect r = geometry();
////    SetWindowPos((HWND)winId(), HWND_BOTTOM, r.x(), r.y(), r.width(), r.height(), SWP_NOACTIVATE|SWP_SHOWWINDOW);
////    SetWindowPos((HWND)winId(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
//}

void Headline::configure(bool stay_visible)
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

    QVBoxLayout *main_layout = new QVBoxLayout;
    main_layout->setContentsMargins(0, 0, 0, 0);

    QLabel* label = new QLabel(this);
    label->setMargin(5);
    label->setText(headline);
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
    label->setFont(font);

    main_layout->addWidget(label);

    setLayout(main_layout);
    adjustSize();
}
