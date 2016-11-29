#pragma once

#include <QWidget>

#include <QtCore/QUrl>

#include "types.h"
#include "specialize.h"

class QPropertyAnimation;

class Article : public QWidget
{
    Q_OBJECT
public:
    explicit Article(const QUrl& story,
                     const QString& article,
                     QWidget* parent = nullptr);
    explicit Article(const Article& source)
    {
        story = source.story;
        article = source.article;
    }

protected:  // data members
    QUrl            story;
    QString         article;

    uint                viewed;     // Chyron
    QPropertyAnimation* animation;  // Chyron

    friend class Chyron;        // the Chyron manages the articles on the screen
};

SPECIALIZE_SHAREDPTR(Article, Article)      // "ArticlePointer"
