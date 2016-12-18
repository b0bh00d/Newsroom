#pragma once

#include "types.h"

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
    int             ttl;

                    // Presentation
                    //   Display
    int             primary_screen;

                    //   Headlines
    bool            headlines_always_visible;

                    //   Size
    int             headlines_pixel_width, headlines_pixel_height;
    double          headlines_percent_width, headlines_percent_height;

                    //   Visibility
    int             limit_content_to;
    FixedText       headlines_fixed_type;

                    // Animation
    AnimEntryType   entry_type;
    AnimExitType    exit_type;
    int             anim_motion_duration;
    int             fade_target_duration;

                    //   Train
                    //     Age Effects
    AgeEffects      train_age_effect;
    int             train_age_percent;

                    //   Dashboard
                    //     Age Effects
    int             dashboard_age_effects;

                    //     Group ID
    QString         dashboard_group_id;

    StoryInfo()
        : trigger_type(LocalTrigger::NewContent),
          primary_screen(0),
          headlines_always_visible(true),
          headlines_pixel_width(0),
          headlines_pixel_height(0),
          headlines_percent_width(0.0),
          headlines_percent_height(0.0),
          limit_content_to(0),
          headlines_fixed_type(FixedText::None),
          anim_motion_duration(0),
          fade_target_duration(0),
          train_age_effect(AgeEffects::None),
          train_age_percent(0),
          dashboard_age_effects(0) {}
    StoryInfo(const StoryInfo& source) { *this = source; }
};
