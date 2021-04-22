#pragma once

#include <ireporter.h>

#include "chartapi.h"

class YAHOOCHARTAPISHARED_EXPORT YahooChartAPIFactory : public IReporterFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IReporterFactory")
    Q_INTERFACES(IReporterFactory)

public:
    // IReporterFactory
    IReporterPointer newInstance() Q_DECL_OVERRIDE;
};
