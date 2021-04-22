#pragma once

#include <ireporter.h>

#include "transmission.h"

class TRANSMISSIONSHARED_EXPORT TransmissionFactory : public IReporterFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IReporterFactory")
    Q_INTERFACES(IReporterFactory)

public:
    // IReporterFactory
    IReporterPointer newInstance() Q_DECL_OVERRIDE;
};
