#pragma once

// Order is important here so they match the GUI controls
enum AnimEntryType
{
    SlideDownLeftTop,
    SlideDownCenterTop,
    SlideDownRightTop,
    SlideInLeftTop,
    SlideInRightTop,
    SlideInLeftBottom,
    SlideInRightBottom,
    SlideUpLeftBottom,
    SlideUpRightBottom,
    SlideUpCenterBottom,
    FadeInCenter,
    PopCenter,
    PopLeftTop,
    PopRightTop,
    PopLeftBottom,
    PopRightBottom
};

enum AnimExitType
{
    Pop,
    Fade,
    SlideLeft,
    SlideRight,
    SlideUp,
    SlideDown,
    SlideFadeLeft,
    SlideFadeRight,
    SlideFadeUp,
    SlideFadeDown
};

enum ReportStacking
{
    Stacked,
    Intermixed
};

enum LocalTrigger
{
    NewContent,
    FileChange
};
