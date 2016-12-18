#pragma once

#include <ireporter.h>

#include "textfile.h"

class TEXTFILE_SHARED_EXPORT TextFileFactory : public IReporterFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IPluginFactory" FILE "")
    Q_INTERFACES(IReporterFactory)

public:
    // IPluginFactory
    IReporterPointer newInstance() Q_DECL_OVERRIDE;
};
