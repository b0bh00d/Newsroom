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
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
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
    label->setStyleSheet("color: rgb(255, 255, 255); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 50, 255), stop:1 rgba(0, 0, 255, 255)); border: 1px solid black; border-radius: 10px;");

    QFont f = label->font();
    f.setPointSize(f.pointSize() + 5);
    label->setFont(f);

    main_layout->addWidget(label);

    setLayout(main_layout);
    adjustSize();
}
