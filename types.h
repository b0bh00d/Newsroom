#pragma once

#include <QtCore/QPluginLoader>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "specialize.h"

enum class AnimEntryType
{
#   define X(a) a,
#   include "animentrytype.def"
#   undef X
};

#define IS_TRAIN(type) (type >= AnimEntryType::TrainDownLeftTop && type <= AnimEntryType::TrainUpCenterBottom)
#define IS_DASHBOARD(type) (type >= AnimEntryType::DashboardDownLeftTop && type <= AnimEntryType::DashboardUpRightBottom)

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

SPECIALIZE_SHAREDPTR(QPluginLoader, Factory)            // "FactoryPointer"
SPECIALIZE_SHAREDPTR(QSettings, Settings)               // "SettingsPointer"

struct PluginInfo
{
    FactoryPointer  factory;
    QString         path;
    QString         name;
    QString         tooltip;
    QString         id;
};

SPECIALIZE_VECTOR(PluginInfo, PluginsInfo)              // "PluginsInfoVector"

struct HeadlineStyle
{
    QString     name;
    QString     stylesheet;
    QStringList triggers;
};

SPECIALIZE_LIST(HeadlineStyle, HeadlineStyle)           // "HeadlineStyleList"
SPECIALIZE_SHAREDPTR(HeadlineStyleList, StyleList)      // "StyleListPointer"
