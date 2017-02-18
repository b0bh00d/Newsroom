#include "transmissionfactory.h"

// IPluginFactory
IReporterPointer TransmissionFactory::newInstance()
{
    return IReporterPointer(reinterpret_cast<IReporter*>(new Transmission()));
}
