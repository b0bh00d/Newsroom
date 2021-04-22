#pragma once

#include <QtCore/QUrl>
#include <QtCore/QEasingCurve>

#include "types.h"
#include "specialize.h"

/// @class StoryInfo
/// @brief Container class for Story information
///
/// This class is intended as a container for all the information needed
/// for a given Story.

struct StoryInfo
{
    QUrl            story;

    // This value is an internal identify for a Story.  It uniquely
    // identifies a Story within the system, regardless of any other
    // of its settings.  The user never sees this value.

    QString         identity;

    // The Story 'angle' is something that will uniquely identify this
    // Story for the user.  In the case of a local file system entity,
    // the full path to that entity will suffice.  However, in the case
    // of a REST URL, the URL itself may not be unique enough as it will
    // simply be the entry point for addition API selectors.  So, this
    // string value should most uniquely represent that particular story
    // type.

    QString         angle;

                    // Reporter
    QString         reporter_beat;      // what special genre knowledge do this Reporter possess?
    QString         reporter_id;        // unique identity for thie Reporter
    int             reporter_parameters_version{0};
    QStringList     reporter_parameters;// what up-front information does the Reporter need to cover the Story?

                    // Notifications
    uint            ttl{5};

                    // Display
    int             primary_screen{0};

                    // Headlines
    bool            headlines_always_visible{true};

                    //   Size
    bool            interpret_as_pixels{true};
    int             headlines_pixel_width{0}, headlines_pixel_height{0};
    double          headlines_percent_width{0.0}, headlines_percent_height{0.0};

                    //   Visibility
    bool            limit_content{false};
    int             limit_content_to{0};
    FixedText       headlines_fixed_type{FixedText::None};

                    //   Extras
    bool            include_progress_bar{false};
    QString         progress_text_re{"\\s(\\d+)%"};
    bool            progress_on_top{false}; // true = bar on top, false = bar on bottom

                    // Animation
    AnimEntryType   entry_type{AnimEntryType::PopCenter};
    AnimExitType    exit_type{AnimExitType::Pop};
    int             anim_motion_duration{500};
    int             fade_target_duration{500};

                    //   Train
                    //     Age Effects
    bool            train_use_age_effect{false};
    AgeEffects      train_age_effect{AgeEffects::None};
    int             train_age_percent{60};

                    //   Dashboard
                    //     Age Effects
    bool            dashboard_use_age_effect{false};
    int             dashboard_age_percent{60};

                    //     Group ID
    QString         dashboard_group_id;

                    // Chyron settings (reference only; not saved)
    int             margin{5};
    bool            dashboard_compact_mode{false};
    qreal           dashboard_compression{25.0};

                    // Producer settings (reference only; not saved)
    QFont           font;

    QEasingCurve    motion_curve{QEasingCurve::OutCubic};
    QEasingCurve    fading_curve{QEasingCurve::InCubic};

    StoryInfo() {}
    StoryInfo(const StoryInfo& source) { *this = source; }

    /*!
      Retrieves the dimensions of a Headline that will display information
      about this particular Story.  It uses the 'Size' parameters.

      \param w The calculated width value
      \param h The calculated height value
     */
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
            auto desktop = QApplication::desktop();
            auto r_desktop = desktop->screenGeometry(primary_screen);

            w = static_cast<int>((headlines_percent_width / 100.0) * r_desktop.width());
            h = static_cast<int>((headlines_percent_height / 100.0) * r_desktop.height());
        }
    }
};

SPECIALIZE_SHAREDPTR(StoryInfo, StoryInfo)      // "StoryInfoPointer"
