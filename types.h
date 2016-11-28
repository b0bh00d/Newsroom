#pragma once

// Order is important here so they match the GUI controls
enum AnimEntryType
{
#   define X(a) a,
#   include "animentrytype.def"
#   undef X
};

enum AnimExitType
{
#   define X(a) a,
#   include "animexittype.def"
#   undef X
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
