#include "chartapifactory.h"

// IPluginFactory
IReporterPointer YahooChartAPIFactory::newInstance()
{
    return IReporterPointer(reinterpret_cast<IReporter*>(new YahooChartAPI()));
}
