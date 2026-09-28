#pragma once

#include "CPP/7zip/UI/GUI/CompressDialog.h"

#include <QObject>
#include <QProcess>
#include <QStringList>

struct ArchiveEntry {
    QString path;
    qint64 size = 0;
    qint64 packedSize = 0;
    QString modified;
    bool directory = false;
    bool link = false;
};

struct ArchiveInfo {
    QString format;
    qint64 physicalSize = -1;
    qint64 unpackedSize = 0;
    int files = 0;
    int folders = 0;
    bool encrypted = false;
};

struct CompressionOptions : NCompressDialog::CInfo {
    int level = 5;
    QString method;
    int threads = 0;             // 0 lets 7-Zip choose.
    quint64 dictionaryBytes = 0; // 0 lets 7-Zip choose.
    int wordSize = 0;            // 0 lets 7-Zip choose.
};

struct ExtractionOptions {
    enum class PathMode { FullPaths, NoPaths };
    enum class OverwriteMode {
        RejectExisting,
        SkipExisting,
        OverwriteExisting,
        RenameExtracted,
        RenameExisting
    };
    PathMode pathMode = PathMode::FullPaths;
    OverwriteMode overwriteMode = OverwriteMode::RejectExisting;
};

class ArchiveBackend : public QObject {
    Q_OBJECT

  public:
    explicit ArchiveBackend(QObject *parent = nullptr, QString executable = QString());
    ~ArchiveBackend() override;
    virtual bool busy() const;
    virtual void list(const QString &archive, const QString &password = {});
    virtual void extract(const QString &archive, const QString &destination,
                         const QStringList &selected = {}, const QString &password = {},
                         const ExtractionOptions &options = {});
    virtual void test(const QString &archive, const QString &password = {});
    virtual void hash(const QStringList &paths, const QString &method);
    virtual void create(const QString &archive, const QStringList &sources,
                        const QString &password = {}, bool encryptHeaders = false,
                        const CompressionOptions &options = {});
    virtual void add(const QString &archive, const QStringList &sources);
    virtual void remove(const QString &archive, const QStringList &paths);
    virtual void renameEntry(const QString &archive, const QString &oldPath,
                             const QString &newPath);
    virtual void replaceEntry(const QString &archive, const QString &entry,
                              const QString &sourceRoot);
    virtual void cancel();
    void providePassword(const QString &password);

  signals:
    void entriesReady(const QList<ArchiveEntry> &entries);
    void infoReady(const ArchiveInfo &info);
    void progress(int percent, const QString &detail);
    void outputReceived(const QString &text);
    void finished(bool success, const QString &message);
    void passwordRequired();

  private:
    enum class Job {
        None,
        List,
        ExtractCheck,
        Extract,
        Test,
        Hash,
        Create,
        Add,
        Delete,
        Rename,
        ReplaceDelete,
        ReplaceAdd
    };
    void start(Job job, const QStringList &arguments, const QString &workingDirectory = {});
    void complete(int exitCode, QProcess::ExitStatus status);
    void consumeOutput(const QByteArray &chunk, bool flush = false);
    void consumeLine(const QByteArray &line);
    static QList<ArchiveEntry> parseListing(const QString &output);
    static bool safePath(const QString &path);
    bool validateExtraction(const QString &destination, const QStringList &selected,
                            const ExtractionOptions &options, QString *error) const;
    bool prepareMutation(const QString &archive);

    QProcess process_;
    Job job_ = Job::None;
    QString output_;
    QByteArray pendingOutput_;
    QString archive_;
    QString createTarget_;
    QString createTemporary_;
    QString addBackup_;
    QString replacementEntry_;
    QString replacementRoot_;
    QList<ArchiveEntry> entries_;
    bool listed_ = false;
    bool cancelled_ = false;
    QString pendingPassword_;
    bool passwordPrompted_ = false;
    bool encrypted_ = false;
    QStringList extractArguments_;
    QString extractPassword_;
};
