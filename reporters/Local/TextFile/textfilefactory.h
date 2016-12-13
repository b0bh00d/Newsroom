#pragma once

#include <iplugin.h>

#include "textfile.h"

class TEXTFILE_SHARED_EXPORT TextFileFactory : public IPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IPluginFactory" FILE "")
    Q_INTERFACES(IPluginFactory)

public:
    // IPluginFactory
    IPluginPointer newInstance() Q_DECL_OVERRIDE;
};
