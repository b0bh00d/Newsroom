#pragma once

#include <QtCore/QUrl>
#include <QtCore/QEasingCurve>

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
    QString         reporter_beat;      // what special genre knowledge do this Reporter possess?
    QString         reporter_id;        // unique identity for thie Reporter
    QStringList     reporter_parameters;// what up-front information does the Reporter need to cover the Story?

                    // Notifications
    uint            ttl;

                    // Display
    int             primary_screen;

                    // Headlines
    bool            headlines_always_visible;

                    //   Size
    bool            interpret_as_pixels;
    int             headlines_pixel_width, headlines_pixel_height;
    double          headlines_percent_width, headlines_percent_height;

                    //   Visibility
    bool            limit_content;
    int             limit_content_to;
    FixedText       headlines_fixed_type;

                    //   Extras
    bool            include_progress_bar;
    QString         progress_text_re;
    bool            progress_on_top;        // true = bar on top, false = bar on bottom

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
    int             margin;
    bool            dashboard_compact_mode;
    qreal           dashboard_compression;

                    // Producer settings (reference only; not saved)
    QFont           font;

    QEasingCurve    motion_curve;
    QEasingCurve    fading_curve;

    StoryInfo()
        : ttl(5),
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
          include_progress_bar(false),
          progress_text_re("\\s(\\d+)%"),
          progress_on_top(false),
          entry_type(AnimEntryType::PopCenter),
          exit_type(AnimExitType::Pop),
          anim_motion_duration(500),
          fade_target_duration(500),
          train_use_age_effect(false),
          train_age_effect(AgeEffects::None),
          train_age_percent(60),
          dashboard_use_age_effect(false),
          dashboard_age_percent(60),
          margin(5),
          dashboard_compact_mode(false),
          dashboard_compression(25),
          motion_curve(QEasingCurve::OutCubic),
          fading_curve(QEasingCurve::InCubic) {}
    StoryInfo(const StoryInfo& source) { *this = source; }

    void get_dimensions(int& w, int& h)
    {
        w = 0;
        h = 0;
        if(interpret_as_pixels)
        {
            w = headlines_pixel_width;
            h = headlines_pixel_height;
        }
        else
        {
            QDesktopWidget* desktop = QApplication::desktop();
            QRect r_desktop = desktop->screenGeometry(primary_screen);

            w = (headlines_percent_width / 100.0) * r_desktop.width();
            h = (headlines_percent_height / 100.0) * r_desktop.height();
        }
    }
};

SPECIALIZE_SHAREDPTR(StoryInfo, StoryInfo)      // "StoryInfoPointer"
