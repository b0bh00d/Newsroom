#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include <QtCore/QUrl>

#include "article.h"

Article::Article(const QUrl& story,
                 const QString& article,
                 QWidget* parent)
    : story(story),
      article(article),
      x(0), y(0), w(0), h(0),
      QWidget(parent)
{
    // https://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setParent(0); // Create TopLevel-Widget
    setAttribute(Qt::WA_ShowWithoutActivating);
//    setAttribute(Qt::WA_NoSystemBackground, true);
//    setAttribute(Qt::WA_TranslucentBackground, true);
//    setAttribute(Qt::WA_PaintOnScreen); // not needed in Qt 5.2 and up

    calculate_dimensions();
}

void Article::calculate_dimensions()
{
    QFontMetrics fm(fontMetrics());

    w = 0;
    h = 0;

    QStringList lines = article.split("\n");
    foreach(const QString& line, lines)
    {
        int strwidth = fm.width(line);
        if(strwidth > w)
            w = strwidth;
        h += fm.height();
    }
}
