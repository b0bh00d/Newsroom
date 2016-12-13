#include "teamcityfactory.h"

// IPluginFactory
IPluginPointer TeamCityFactory::newInstance()
{
    return IPluginPointer(reinterpret_cast<IPlugin*>(new TeamCity()));
}
