#include "teamcity9factory.h"

// IPluginFactory
IReporterPointer TeamCity9Factory::newInstance()
{
    return IReporterPointer(reinterpret_cast<IReporter*>(new TeamCity9()));
}
