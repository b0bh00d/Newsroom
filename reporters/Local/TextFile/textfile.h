#pragma once

#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QByteArray>

#include <ireporter.h>

#include "textfile_global.h"

/// @class TextFile
/// @brief A Reporter for Newsroom that covers local files
///
/// TextFile is a Reporter plug-in for Newsroom that knows how to read
/// and report on a single text file on the local machine.  Such files
/// are expected to add new content occassionally, appended to the end
/// of the file (e.g., log files).  TextFile can report on a change in
/// the file, or it can report the new contents.

class TEXTFILE_SHARED_EXPORT TextFile : public IReporter
{
    Q_OBJECT
public:
    TextFile(QObject* parent = nullptr);

    // IReporter
    QString ErrorString() const Q_DECL_OVERRIDE { return error_message; }
    QStringList DisplayName() const Q_DECL_OVERRIDE;
    QString PluginClass() const override { return "Local"; }
    QByteArray PluginID() const Q_DECL_OVERRIDE;
    float Supports(const QUrl &entity) const Q_DECL_OVERRIDE;
    int RequiresVersion() const override;
    RequirementsFormats RequiresFormat() const override;
    bool RequiresUpgrade(int, QStringList&) override;
    QStringList Requires(int = 0) const Q_DECL_OVERRIDE;
    bool SetRequirements(const QStringList& parameters) Q_DECL_OVERRIDE;
    void SetStory(const QUrl& story) Q_DECL_OVERRIDE;
    bool CoverStory() Q_DECL_OVERRIDE;
    bool FinishStory() Q_DECL_OVERRIDE;
    // we have no secure data
    void Secure(QStringList& /*params*/) const Q_DECL_OVERRIDE {}
    void Unsecure(QStringList& /*params*/) const Q_DECL_OVERRIDE {}

private slots:
    void            slot_poll();

private:    // typedefs and enums
    typedef enum
    {
        Trigger,
        LeftStrip,
        RightStrip,
        Count,
    } Param;

    enum class LocalTrigger
    {
        NewContent,
        FileChange
    };

private:    // methods
    void    preprocess(QByteArray& ba);

private:    // data membvers
    QFileInfo       target;
    int             stabilize_count{0};

    qint64          seek_offset{0};
    qint64          last_size{0};

    QTimer*         poll_timer{nullptr};

    QString         report;

    LocalTrigger    trigger{LocalTrigger::NewContent};

    int             left_strip{0};
    int             right_strip{0};
};
