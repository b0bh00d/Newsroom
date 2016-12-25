#pragma once

#include <QtCore/QUrl>

#include "types.h"
#include "specialize.h"

struct StoryInfo
{
    QUrl            story;

    // The 'story identity' is something that will uniquely identify this
    // story.  In the case of a local file system entity, the full path to
    // that entity will suffice.  However, in the case of a REST URL, the
    // URL itself will not be unique enough as it will simply be the base
    // value for addition API selectors.  So, this string value should
    // most uniquely represents that particular story type.

    QString         identity;

                    // Reporter
    QString         reporter_class;     // narrows the search criteria
    QString         reporter_id;        // returned by PluginID()
    QStringList     reporter_parameters;

                    // Notifications
    LocalTrigger    trigger_type;
    uint            ttl;

                    // Presentation
                    //   Display
    int             primary_screen;

                    //   Headlines
    bool            headlines_always_visible;

                    //   Size
    bool            interpret_as_pixels;
    int             headlines_pixel_width, headlines_pixel_height;
    double          headlines_percent_width, headlines_percent_height;

                    //   Visibility
    bool            limit_content;
    int             limit_content_to;
    FixedText       headlines_fixed_type;

                    // Animation
    AnimEntryType   entry_type;
    AnimExitType    exit_type;
    int             anim_motion_duration;
    int             fade_target_duration;

                    //   Train
                    //     Age Effects
    bool            train_use_age_effect;
    AgeEffects      train_age_effect;
    int             train_age_percent;

                    //   Dashboard
                    //     Age Effects
    bool            dashboard_use_age_effect;
    int             dashboard_age_percent;

                    //     Group ID
    QString         dashboard_group_id;

                    // Chyron settings (reference only; not saved)
    ReportStacking  stacking_type;
    int             margin;

                    // Producer settings (reference only; not saved)
    QFont           font;

    StoryInfo()
        : trigger_type(LocalTrigger::NewContent),
          ttl(5),
          primary_screen(0),
          headlines_always_visible(true),
          interpret_as_pixels(true),
          headlines_pixel_width(0),
          headlines_pixel_height(0),
          headlines_percent_width(0.0),
          headlines_percent_height(0.0),
          limit_content(false),
          limit_content_to(0),
          headlines_fixed_type(FixedText::None),
          anim_motion_duration(500),
          fade_target_duration(500),
          train_use_age_effect(false),
          train_age_effect(AgeEffects::None),
          train_age_percent(60),
          dashboard_use_age_effect(false),
          dashboard_age_percent(60),
          stacking_type(ReportStacking::Stacked),
          margin(5) {}
    StoryInfo(const StoryInfo& source) { *this = source; }
};

SPECIALIZE_SHAREDPTR(StoryInfo, StoryInfo)      // "StoryInfoPointer"
