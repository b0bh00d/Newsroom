#pragma once

#include <ireporter.h>

#include "teamcity9.h"

class TEAMCITY9SHARED_EXPORT TeamCity9Factory : public IReporterFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IPluginFactory" FILE "")
    Q_INTERFACES(IReporterFactory)

public:
    // IPluginFactory
    IReporterPointer newInstance() Q_DECL_OVERRIDE;
};
