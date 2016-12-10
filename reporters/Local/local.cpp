#include "local.h"

QString Local::DisplayName() const
{
    return QStringLiteral("Local File");
}

QByteArray Local::PluginID() const
{
    return "{F1949758-2A08-4E8A-8290-90DCD270A8B9}";
}

bool Local::CanGrok(const QString& file) const
{
    return false;
}

void Local::SetStory(const QString& file)
{
}

bool Local::CoverStory()
{
    return false;
}

bool Local::FinishStory()
{
    return false;
}

QString Local::Headline()
{
    return QString();
}
