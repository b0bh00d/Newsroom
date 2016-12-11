#pragma once

#include <QObject>

#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QByteArray>

#include <ipluginlocal.h>

#include "../../specialize.h"

#include "textfile_global.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

class TEXTFILE_SHARED_EXPORT TextFile : public QObject, public IPluginLocal
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.lucidgears.Newsroom.IPluginLocal" FILE "")
    Q_INTERFACES(IPluginLocal)

public:
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }

    QString DisplayName() const Q_DECL_OVERRIDE;
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    bool Supports(const QString& file) const Q_DECL_OVERRIDE;
    void SetStory(const QString& file) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;
    QString Headline() Q_DECL_OVERRIDE;

signals:
    void            signal_new_data(const QByteArray& data);

protected slots:
    void            slot_poll();

protected:
    QString         error_message;

    QFileInfo       target;
    int             stabilize_count;
//    QDateTime       last_modified;
    qint64          seek_offset;
    qint64          last_size;

    QTimer*         poll_timer;

    QString         report;
};
