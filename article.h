#pragma once

#include <QWidget>

#include <QtGui/QFont>

#include <QtCore/QUrl>

#include "types.h"
#include "specialize.h"

class QPropertyAnimation;

/// @class Article
/// @brief Contains data submitted by a Reporter
///
/// The Article class contains the headline submitted by a Reporter watching
/// a specific story.  It inherits from QWidget, so it is the visible element
/// on the screen, but its visibility and life cycle are managed by the Chyron.

class Article : public QWidget
{
    Q_OBJECT
public:
    explicit Article(const QUrl& story, const QString& article, QWidget* parent = nullptr);
    explicit Article(const Article& source)
    {
        story = source.story;
        article = source.article;
    }

protected:  // methods
    /*!
      This method is employed by the Chyron class to configure the Article for display.

      \param font This is the font that the Article should use when displaying information.
      \param stay_visible Indicates whether or not the widget should be top-most in the Z order
     */
    void    configure(const QFont& font, bool stay_visible);    // Chyron

protected:  // data members
    QUrl            story;
    QString         article;

    bool                ignore;     // Chyron
    uint                viewed;     // Chyron
    QFont               font;       // Chyron
    int                 pointsize;  // Chyron
    QPropertyAnimation* animation;  // Chyron

    friend class Chyron;        // the Chyron manages the articles on the screen
};

SPECIALIZE_SHAREDPTR(Article, Article)      // "ArticlePointer"
