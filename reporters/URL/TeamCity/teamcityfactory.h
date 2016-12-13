#pragma once

#include <iplugin.h>

#include "teamcity.h"

class TEAMCITYSHARED_EXPORT TeamCityFactory : public IPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IPluginFactory" FILE "")
    Q_INTERFACES(IPluginFactory)

public:
    // IPluginFactory
    IPluginPointer newInstance() Q_DECL_OVERRIDE;
};
