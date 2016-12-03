#pragma once

#include <QWidget>

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QParallelAnimationGroup>

#include "types.h"
#include "specialize.h"
#include "article.h"

/// @class Chyron
/// @brief Manages headlines submitted by Reporters
///
/// The Chyron class manages the display of headlines submitted by Reporters.

class Chyron : public QObject
{
    Q_OBJECT
public:
    explicit Chyron(const QUrl& story,
                    uint ttl,
                    int display,
                    const QFont& font,
                    bool always_visible,
                    AnimEntryType entry_type,
                    AnimExitType exit_type,
                    ReportStacking stacking_type,
                    int train_fixed_width = 0,
                    AgeEffects effect = AgeEffects::None,
                    int train_reduce_opacity = 0,
                    int margin = 5,
                    QObject* parent = nullptr);

    AnimEntryType   get_entry_type()    const   { return entry_type; }
    AnimExitType    get_exit_type()     const   { return exit_type; }

    // stacking priority determines the lane occupied by the chyron
    // among identical entry types
    void        set_stacking_lane(int /*lane*/) {}

public slots:
    void        slot_file_article(ArticlePointer article);

protected slots:
    void        slot_age_articles();
    void        slot_article_posted();
    void        slot_article_expired();
    void        slot_release_animation();
    void        slot_train_expire_articles();

protected:  // typedefs and enums
    SPECIALIZE_LIST(ArticlePointer, Article)            // "ArticleList"
    SPECIALIZE_QUEUE(ArticlePointer, Transition)        // "TransitionQueue"
    SPECIALIZE_MAP(QPropertyAnimation*, ArticlePointer, PropertyAnimation) // "PropertyAnimationMap"
    SPECIALIZE_MAP(ArticlePointer, bool, Entering)      // "EnteringMap"
    SPECIALIZE_MAP(ArticlePointer, bool, Exiting)       // "ExitingMap"

protected:  // methods
    void        initialize_article(ArticlePointer article);
    void        start_article_entry(ArticlePointer article);
    void        start_article_exit(ArticlePointer article);

protected:  // data members
    QUrl            story;
    uint            ttl;
    int             display;
    QFont           font;
    bool            always_visible;
    AnimEntryType   entry_type;
    AnimExitType    exit_type;
    ReportStacking  stacking_type;
    int             margin;
    int             train_fixed_width;
    AgeEffects      age_effect;
    int             train_reduce_opacity;

    QTimer*         age_timer;

    TransitionQueue incoming_articles;
    ArticleList     article_list;
    ArticleList     reduce_list;

    PropertyAnimationMap prop_anim_map;
    EnteringMap     entering_map;
    ExitingMap      exiting_map;
};

SPECIALIZE_SHAREDPTR(Chyron, Chyron)        // "ChyronPointer"
