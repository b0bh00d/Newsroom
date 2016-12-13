#include "textfilefactory.h"

// IPluginFactory
IPluginPointer TextFileFactory::newInstance()
{
    return IPluginPointer(reinterpret_cast<IPlugin*>(new TextFile()));
}
