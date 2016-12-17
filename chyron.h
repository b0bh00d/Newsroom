#pragma once

#include <QWidget>

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QParallelAnimationGroup>

#include "types.h"
#include "specialize.h"

#include "lanemanager.h"
#include "headline.h"

/// @class Chyron
/// @brief Manages headlines submitted by Reporters
///
/// The Chyron class manages the display of headlines submitted by Reporters.

#ifdef HIGHLIGHT_LANES
class HighlightWidget;
#endif

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
        int             headline_pixel_width;
        int             headline_pixel_height;
        double          headline_percent_width;
        double          headline_percent_height;
        FixedText       headline_fixed_text;
        AgeEffects      train_effect;
        int             train_reduce_opacity;
        QString         dashboard_group;
        bool            dashboard_effect;
        int             dashboard_reduce_opacity;
        int             margin;

        Settings()
            : headline_pixel_width(0),
              headline_pixel_height(0),
              headline_percent_width(0.0),
              headline_percent_height(0.0),
              headline_fixed_text(FixedText::None),
              train_effect(AgeEffects::None),
              train_reduce_opacity(0),
              dashboard_effect(false),
              dashboard_reduce_opacity(0),
              margin(5)
        {}

        Settings(const Settings& source)
        {
            *this = source;
        }
    };

    explicit Chyron(const QUrl& story,
                    const Chyron::Settings& chyron_settings,
                    LaneManagerPointer lane_manager,
                    QObject* parent = nullptr);
    ~Chyron();

    const Settings& get_settings()      const   { return settings; }

    // These methods are used by the Lane Manager to adjust lanes
    // when a Chyron is deleted.  This does an immediate move of
    // any visible Headlines in the current lane.

    void        shift_left(int amount);
    void        shift_right(int amount);
    void        shift_up(int amount);
    void        shift_down(int amount);

public slots:
    void        slot_file_headline(HeadlinePointer headline);

protected slots:
    void        slot_age_headlines();
    void        slot_headline_posted();
    void        slot_headline_expired();
    void        slot_release_animation();
    void        slot_train_expire_headlines();

    void        slot_headline_mouse_enter();
    void        slot_headline_mouse_exit();

protected:  // typedefs and enums
    SPECIALIZE_LIST(HeadlinePointer, Headline)          // "HeadlineList"
    SPECIALIZE_QUEUE(HeadlinePointer, Transition)       // "TransitionQueue"
    SPECIALIZE_MAP(QPropertyAnimation*, HeadlinePointer, PropertyAnimation) // "PropertyAnimationMap"
    SPECIALIZE_MAP(HeadlinePointer, bool, Entering)     // "EnteringMap"
    SPECIALIZE_MAP(HeadlinePointer, bool, Exiting)      // "ExitingMap"
    SPECIALIZE_MAP(Headline*, double, Opacity)          // "OpacityMap"

protected:  // methods
    void        initialize_headline(HeadlinePointer headline);
    void        start_headline_entry(HeadlinePointer headline);
    void        start_headline_exit(HeadlinePointer headline);
    void        dashboard_expire_headlines();

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

    LaneManagerPointer  lane_manager;

    OpacityMap      opacity_map;

#ifdef HIGHLIGHT_LANES
    HighlightWidget*    highlight;
#endif
};

SPECIALIZE_SHAREDPTR(Chyron, Chyron)        // "ChyronPointer"
