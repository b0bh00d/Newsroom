#pragma once

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include <QtCore/QPluginLoader>
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

enum class UnsubscribeAction
{
    Immediate,
    Graceful
};

SPECIALIZE_SHAREDPTR(QPluginLoader, Factory)            // "FactoryPointer"

struct ReporterInfo
{
    FactoryPointer  factory;
    QString         path;
    QString         name;
    QString         tooltip;
    QString         id;
    int             params_version;
    QStringList     params_requires;
};

SPECIALIZE_VECTOR(ReporterInfo, ReportersInfo)          // "ReportersInfoVector"
SPECIALIZE_MAP(QString, ReportersInfoVector, Beats)     // "BeatsMap"

struct HeadlineStyle
{
    QString     name;
    QString     stylesheet;
    QStringList triggers;
};

SPECIALIZE_LIST(HeadlineStyle, HeadlineStyle)           // "HeadlineStyleList"
SPECIALIZE_SHAREDPTR(HeadlineStyleList, StyleList)      // "StyleListPointer"

SPECIALIZE_PAIR(QString, QStringList, Series)           // "SeriesPair"
SPECIALIZE_LIST(SeriesPair, Series)                     // "SeriesList"

class QPropertyAnimation;
SPECIALIZE_SHAREDPTR(QPropertyAnimation, Animation)     // "AnimationPointer"
