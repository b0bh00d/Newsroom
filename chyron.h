#pragma once

#include <QWidget>

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QParallelAnimationGroup>

#include "types.h"
#include "specialize.h"
#include "headline.h"

/// @class Chyron
/// @brief Manages headlines submitted by Reporters
///
/// The Chyron class manages the display of headlines submitted by Reporters.

class Chyron : public QObject
{
    Q_OBJECT
public:
    struct Settings
    {
        uint            ttl;
        int             display;
        bool            always_visible;
        AnimEntryType   entry_type;
        AnimExitType    exit_type;
        ReportStacking  stacking_type;
        int             headline_fixed_width;
        int             headline_fixed_height;
        AgeEffects      effect;
        int             train_reduce_opacity;
        int             margin;

        Settings()
            : headline_fixed_width(0),
              headline_fixed_height(0),
              effect(AgeEffects::None),
              train_reduce_opacity(0),
              margin(5)
        {}

        Settings(const Settings& source)
        {
            *this = source;
        }
    };

    explicit Chyron(const QUrl& story, const Chyron::Settings& chyron_settings, QObject* parent = nullptr);
    ~Chyron();

    AnimEntryType   get_entry_type()    const   { return settings.entry_type; }
    AnimExitType    get_exit_type()     const   { return settings.exit_type; }

    // stacking priority determines the lane occupied by the chyron
    // among identical entry types
    void        set_stacking_lane(int /*lane*/) {}

public slots:
    void        slot_file_headline(HeadlinePointer headline);

protected slots:
    void        slot_age_headlines();
    void        slot_headline_posted();
    void        slot_headline_expired();
    void        slot_release_animation();
    void        slot_train_expire_headlines();

protected:  // typedefs and enums
    SPECIALIZE_LIST(HeadlinePointer, Headline)          // "HeadlineList"
    SPECIALIZE_QUEUE(HeadlinePointer, Transition)       // "TransitionQueue"
    SPECIALIZE_MAP(QPropertyAnimation*, HeadlinePointer, PropertyAnimation) // "PropertyAnimationMap"
    SPECIALIZE_MAP(HeadlinePointer, bool, Entering)     // "EnteringMap"
    SPECIALIZE_MAP(HeadlinePointer, bool, Exiting)      // "ExitingMap"

protected:  // methods
    void        initialize_headline(HeadlinePointer headline);
    void        start_headline_entry(HeadlinePointer headline);
    void        start_headline_exit(HeadlinePointer headline);

protected:  // data members
    QUrl            story;
    Settings        settings;

    QTimer*         age_timer;

    TransitionQueue incoming_headlines;
    HeadlineList    headline_list;
    HeadlineList    reduce_list;

    PropertyAnimationMap prop_anim_map;
    EnteringMap     entering_map;
    ExitingMap      exiting_map;
};

SPECIALIZE_SHAREDPTR(Chyron, Chyron)        // "ChyronPointer"
