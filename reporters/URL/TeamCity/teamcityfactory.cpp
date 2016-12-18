#include "teamcityfactory.h"

// IPluginFactory
IReporterPointer TeamCityFactory::newInstance()
{
    return IReporterPointer(reinterpret_cast<IReporter*>(new TeamCity()));
}
