#pragma once

#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QByteArray>

#include <iplugin.h>

#include "../../specialize.h"

#include "textfile_global.h"

#define ASSERT_UNUSED(cond) Q_ASSERT(cond); Q_UNUSED(cond)

class TEXTFILE_SHARED_EXPORT TextFile : public IPlugin
{
    Q_OBJECT
public:
    TextFile(QObject* parent = nullptr);

    // IPlugin
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }
    QStringList DisplayName() const Q_DECL_OVERRIDE;
    QString PluginClass() const                 { return "Local"; }
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    bool Supports(const QString& file) const Q_DECL_OVERRIDE;
    QStringList Requires() const  Q_DECL_OVERRIDE { return QStringList(); }
    bool SetRequirements(const QStringList& /*parameters*/) Q_DECL_OVERRIDE { return false; }
    void SetStory(const QUrl& story) Q_DECL_OVERRIDE;
    void SetCoverage(bool notices_only) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;

protected slots:
    void            slot_poll();

protected:
    QFileInfo       target;
    int             stabilize_count;
//    QDateTime       last_modified;
    qint64          seek_offset;
    qint64          last_size;

    QTimer*         poll_timer;

    QString         report;

    bool            notices_only;
};
