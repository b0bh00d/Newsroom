#pragma once

#include <QObject>

#include <ipluginlocal.h>

#include "../../specialize.h"

#include "local_global.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

class LOCALHARED_EXPORT Local : public QObject, public IPluginLocal
{
    Q_OBJECT
    Q_INTERFACES(IPluginLocal)

public:
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }

    QString DisplayName() const Q_DECL_OVERRIDE;
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    bool CanGrok(const QString& file) const Q_DECL_OVERRIDE;
    void SetStory(const QString& file) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;
    QString Headline() Q_DECL_OVERRIDE;

signals:
    void    signal_headline(const QString& headline);
};
