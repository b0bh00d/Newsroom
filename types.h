#pragma once

enum class AnimEntryType
{
#   define X(a) a,
#   include "animentrytype.def"
#   undef X
};

enum class AnimExitType
{
#   define X(a) a,
#   include "animexittype.def"
#   undef X
};

enum class ReportStacking
{
#   define X(a) a,
#   include "reportstacking.def"
#   undef X
};

enum class LocalTrigger
{
    NewContent,
    FileChange
};

enum class AgeEffects
{
    None,
    ReduceOpacityFixed,
    ReduceOpacityByAge
};

enum class FixedText
{
    None,
    ScaleToFit,
    ClipToFit
};
