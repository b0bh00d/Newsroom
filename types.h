#pragma once

#include <QMetaType>

enum class AnimEntryType
{
#   define X(a) a,
#   include "animentrytype.def"
#   undef X
};
Q_DECLARE_METATYPE(AnimEntryType)

enum class AnimExitType
{
#   define X(a) a,
#   include "animexittype.def"
#   undef X
};
Q_DECLARE_METATYPE(AnimExitType)

enum class ReportStacking
{
    Stacked,
    Intermixed
};
Q_DECLARE_METATYPE(ReportStacking)

enum class LocalTrigger
{
    NewContent,
    FileChange
};
Q_DECLARE_METATYPE(LocalTrigger)

enum class AgeEffects
{
    None,
    ReduceOpacityFixed,
    ReduceOpacityByAge
};
Q_DECLARE_METATYPE(AgeEffects)

enum class FixedText
{
    None,
    ScaleToFit,
    ClipToFit
};
Q_DECLARE_METATYPE(FixedText)
