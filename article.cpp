#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

#include <QtCore/QUrl>

#include "article.h"

Article::Article(const QUrl& story,
                 const QString& article,
                 QWidget* parent)
    : story(story),
      article(article),
      QWidget(parent)
{
    // https://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setParent(0); // Create TopLevel-Widget
    setAttribute(Qt::WA_ShowWithoutActivating);
//    setAttribute(Qt::WA_NoSystemBackground, true);
//    setAttribute(Qt::WA_TranslucentBackground, true);
//    setAttribute(Qt::WA_PaintOnScreen); // not needed in Qt 5.2 and up

    QVBoxLayout *main_layout = new QVBoxLayout;
    main_layout->setContentsMargins(0, 0, 0, 0);

    QLabel* label = new QLabel(this);
    label->setMargin(5);
    label->setText(article);
    label->setStyleSheet("color: rgb(255, 255, 255); background: rgb(50, 50, 50);");
    main_layout->addWidget(label);

    setLayout(main_layout);
    adjustSize();
}
