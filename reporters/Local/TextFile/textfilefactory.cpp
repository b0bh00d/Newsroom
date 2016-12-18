#include "textfilefactory.h"

// IPluginFactory
IReporterPointer TextFileFactory::newInstance()
{
    return IReporterPointer(reinterpret_cast<IReporter*>(new TextFile()));
}
