#pragma once

#include <QWidget>

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QParallelAnimationGroup>

#include "types.h"
#include "specialize.h"
#include "article.h"

class Chyron : public QObject
{
    Q_OBJECT
public:
    explicit Chyron(const QUrl& story,
                    uint ttl,
                    int display,
                    AnimEntryType entry_type,
                    AnimExitType exit_type,
                    ReportStacking stacking_type,
                    int margin = 5,
                    QObject* parent = nullptr);

public slots:
    void        slot_file_article(ArticlePointer article);

protected slots:
    void        slot_age_articles();
    void        slot_article_posted();
    void        slot_article_expired();
    void        slot_release_animation();

protected:  // typedefs and enums
    SPECIALIZE_LIST(ArticlePointer, Article)            // "ArticleList"
    SPECIALIZE_QUEUE(ArticlePointer, Transition)        // "TransitionQueue"
    SPECIALIZE_MAP(QPropertyAnimation*, ArticlePointer, PropertyAnimation) // "PropertyAnimationMap"
    SPECIALIZE_MAP(ArticlePointer, bool, Entering)      // "EnteringMap"
    SPECIALIZE_MAP(ArticlePointer, bool, Exiting)       // "ExitingMap"

protected:  // methods
    void        initialize_article_position(ArticlePointer article);
    void        start_article_entry(ArticlePointer article);
    void        start_article_exit(ArticlePointer article);

protected:  // data members
    QUrl            story;
    uint            ttl;
    int             display;
    AnimEntryType   entry_type;
    AnimExitType    exit_type;
    ReportStacking  stacking_type;
    int             margin;

    QTimer*         age_timer;

    TransitionQueue incoming_articles;
    ArticleList     article_list;

    PropertyAnimationMap prop_anim_map;
    EnteringMap     entering_map;
    ExitingMap      exiting_map;
};

SPECIALIZE_SHAREDPTR(Chyron, Chyron)        // "ChyronPointer"
