#pragma once
struct CExternalCodecs;
struct CProperty;
class CArc;

#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "processpause.h"

#include <QDateTime>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QProcess>
#include <QStringList>
#include <QTimer>
#include <functional>
#include <memory>
#include <optional>
class QtArchiveVolumeOutput;
class QtArchiveCreationLifecycle;
class QtArchiveHashOperation;
class QtArchiveReadOperation;
class QtOperationLauncher;
class QtArchiveCopyPreparation;
class ArchiveCodecCatalog;
class ArchiveSfxModule;
class ArchiveCodecProbe;
class ArchiveNativeOpen;
class ArchiveDevice;
class QtOwnedTempDir;
class QtArchiveTransferBackup;
class QThread;
struct IOpenCallbackUI;
struct NativeArchiveListing;
struct QtArchiveDirectoryExtractRequest;
struct IUpdateCallbackUI2;
struct CUpdateOptions;
struct CUpdateErrorInfo;
struct COpenType;
struct COpenOptions;
namespace NWildcard {
class CCensor;
}

struct ArchiveEntry {
    QString path;
    qint64 size = 0;
    qint64 packedSize = 0;
    QString modified;
    bool directory = false;
    bool link = false;
    // Actual technical-listing fields; no value is synthesized for absent data.
    QMap<QString, QString> properties;
    std::optional<quint32> crc;
    QMap<quint32, QByteArray> rawProperties;
    std::optional<quint64> sizeValue, packedSizeValue;
};

struct ArchiveInfo {
    QString format;
    qint64 physicalSize = -1;
    qint64 totalPhysicalSize = -1;
    int volumeCount = 1;
    qint64 offset = 0;
    qint64 unpackedSize = 0;
    int files = 0;
    int folders = 0;
    bool encrypted = false;
    QMap<QString, QString> properties;
    QString readLimitations;
    unsigned containerLevels = 1;
};

struct CompressionOptions : NCompressDialog::CInfo {
    CompressionOptions() { Level = 5; }
    QString SfxModule;
    // Stable native handler name at the process boundary; never a CLI switch.
    QString format;
};

struct ExtractionOptions {
    enum class PathMode { FullPaths, NoPaths, CurrentPaths, AbsolutePaths };
    enum class OverwriteMode {
        Ask,
        RejectExisting,
        SkipExisting,
        OverwriteExisting,
        RenameExtracted,
        RenameExisting
    };
    PathMode pathMode = PathMode::FullPaths;
    QString currentFolder;
    OverwriteMode overwriteMode = OverwriteMode::RejectExisting;
    CBoolPair ElimDup;
    CBoolPair NtSecurity;
};

class ArchiveBackend : public QObject {
    Q_OBJECT

  public:
    explicit ArchiveBackend(QObject *parent = nullptr, QString executable = QString());
    ~ArchiveBackend() override;
    virtual bool busy() const;
    virtual HRESULT lastResult() const;
    // Item/block or opening diagnostic result, or -1 when no result is available.
    Int32 lastSourceExtractOperation() const { return sourceExtractOperation_; }
    // Explicit module/version boundary. The engine probe/discovery service
    // supplies its verified version; widgets consume immutable metadata only.
    HRESULT loadCodecCatalog(const QString &module, quint32 engineVersion);
    std::shared_ptr<const ArchiveCodecCatalog> codecCatalog() const {
        return std::atomic_load(&codecCatalog_);
    }
    ArchiveBackend *createIndependentBackend(QObject *parent = nullptr) const;
    virtual bool discoverCodecCatalog(const QString &module = {});
    // GUI-thread state; opening workers request it through their manager.
    bool discoveringCodecs() const { return codecDiscoveryActive_; }
    void cancelCodecDiscovery();
    QString openDialogFilter() const;
    bool supportsArchiveUpdate(const QString &format) const;
    static bool compressionProperties(const QString &format, const QString &password,
                                      bool encryptHeaders, const CompressionOptions &options,
                                      CObjectVector<CProperty> *properties, QString *error);
    ArchiveSfxModule sfxModule(const QString &path = {}) const;
    // Called by the source opening worker; no process or widget state is accessed.
    HRESULT openNativeArchive(const QString &path, const QString &type, IOpenCallbackUI *callback,
                              std::shared_ptr<ArchiveNativeOpen> &reader,
                              const COpenOptions *options = nullptr) const;
    static HRESULT openNativeArchive(const std::shared_ptr<const ArchiveCodecCatalog> &catalog,
                                     const QString &path, const QString &type,
                                     IOpenCallbackUI *callback,
                                     std::shared_ptr<ArchiveNativeOpen> &reader,
                                     const COpenOptions *options);
    static HRESULT openDeviceFilesystem(const std::shared_ptr<const ArchiveCodecCatalog> &catalog,
                                        IInStream *input, const QString &name,
                                        IOpenCallbackUI *callback,
                                        std::shared_ptr<ArchiveNativeOpen> &reader,
                                        QString *error = nullptr,
                                        const COpenOptions *options = nullptr);
    static HRESULT inspectDevice(const std::shared_ptr<const ArchiveCodecCatalog> &catalog,
                                 IInStream *input, quint64 size, const QString &name,
                                 IOpenCallbackUI *callback, std::shared_ptr<ArchiveDevice> &source);
    HRESULT updateNativeComment(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                ISequentialOutStream *output, UInt32 count,
                                IArchiveUpdateCallback *callback) const;
    HRESULT updateSourceArchive(const QString &path, const QString &type,
                                NWildcard::CCensor &censor, CUpdateOptions &options,
                                CUpdateErrorInfo &errorInfo, IOpenCallbackUI *openCallback,
                                IUpdateCallbackUI2 *callback, bool needSetPath) const;
    // Source worker transport. GUI preflight owns operation-state checks.
    HRESULT updateSourceArchive(const UString &path, const CObjectVector<COpenType> &types,
                                NWildcard::CCensor &censor, CUpdateOptions &options,
                                CUpdateErrorInfo &errorInfo, IOpenCallbackUI *openCallback,
                                IUpdateCallbackUI2 *callback, bool needSetPath,
                                const std::shared_ptr<const ArchiveCodecCatalog> &catalog) const;
    HRESULT updateNativeArchive(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                ISequentialOutStream *output, UInt32 count,
                                IArchiveUpdateCallback *callback) const;
    HRESULT setNativeArchiveProperties(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                       const wchar_t *const *names, const PROPVARIANT *values,
                                       UInt32 count) const;
    HRESULT updateNativeArchive(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                ISequentialOutStream *output, UInt32 count,
                                IArchiveUpdateCallback *callback,
                                const CompressionOptions &options) const;
    static HRESULT extractNativeArchive(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                        const UInt32 *indices, UInt32 count, Int32 testMode,
                                        IArchiveExtractCallback *callback);
    virtual void list(const QString &archive, const QString &password = {});
    virtual void openFolder(const QString &archive, const QString &format,
                            const QString &password = {});
    virtual void extract(const QString &archive, const QString &destination,
                         const QStringList &selected = {}, const QString &password = {},
                         const ExtractionOptions &options = {});
    virtual void test(const QString &archive, const QString &password = {});
    // Independent filesystem operations. The observer belongs to the app;
    // releasing this backend/window does not cancel an accepted GUI operation.
    virtual QtOperationLauncher *startGuiTest(const QStringList &archives, QString &error,
                                              bool hashMode = false);
    // A nonempty basePrefix retains literal relative panel paths and its
    // recursion mode. An empty prefix selects the source shell hash workflow.
    virtual QtOperationLauncher *startGuiHash(const QStringList &paths, const QString &method,
                                              const QString &basePrefix, bool enterToDirs,
                                              QString &error);
    virtual QtOperationLauncher *startGuiCompress(const QString &archive, const QString &type,
                                                  bool addExtension, const QStringList &paths,
                                                  bool email, bool showDialog, QString &error);
    virtual QtOperationLauncher *startGuiExtract(const QStringList &archives,
                                                 const QString &destination, bool showDialog,
                                                 bool elimDup, quint32 writeZone, QString &error);
    // The native source extraction callback already owns listed metadata for
    // this archive, independent of the backend's last UI listing.
    virtual void testSourceArchive(const QString &archive, const QString &password);
    virtual void hash(const QStringList &paths, const QString &method);
    void hashMethods(const QStringList &paths, const QStringList &methods);
    HRESULT loadHashProviders(CExternalCodecs &providers) const;
    HRESULT loadBenchmarkProviders(CExternalCodecs &providers) const;
    virtual std::unique_ptr<class IQtBenchmarkServices> benchmarkServices() const;
    virtual void create(const QString &archive, const QStringList &sources,
                        const QString &password = {}, bool encryptHeaders = false,
                        const CompressionOptions &options = {});
    virtual void add(const QString &archive, const QStringList &sources);
    std::unique_ptr<QtArchiveTransferBackup>
    saveArchiveTransferDestination(const QString &path) const;
    virtual void remove(const QString &archive, const QStringList &paths);
    virtual void renameEntry(const QString &archive, const QString &oldPath,
                             const QString &newPath);
    virtual void replaceEntry(const QString &archive, const QString &entry,
                              const QString &sourceRoot);
    // Source IOutArchive transport writes a new archive; native WorkDir owns publication.
    virtual void updateSourceFile(const QString &archive, const QString &entry,
                                  const QString &sourceRoot, const QString &output,
                                  const QString &format, const QString &password);
    virtual void updateSourceAdd(const QString &archive, const QStringList &entries,
                                 const QString &sourceRoot, const QString &output,
                                 const QString &format, const QString &password);
    struct SourceUpdatePlan {
        QStringList removed;
        QStringList removedDirectories;
        QStringList retainedPaths;
        QStringList files;
        QStringList directories;
        QList<QPair<QString, QString>> renames;
        bool allowAbsolute = false;
        bool allowProtected = false;
        QString fileRoot;
        QString directoryRoot;
    };
    virtual void updateSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                                  const QString &output, const QString &format,
                                  const QString &password,
                                  const std::optional<CompressionOptions> &options = std::nullopt);
    enum class SourceMutation { Delete, Rename, CreateFolder };
    virtual void updateSourceMutation(const QString &archive, const QString &output,
                                      const QString &format, SourceMutation operation,
                                      const QStringList &paths, const QString &sourceRoot,
                                      const QString &password = {});
    // Extract one literal member into a private spool. Source callbacks own
    // the final output stream and filesystem presentation.
    virtual void extractSourceEntry(const QString &archive, const QString &entry,
                                    const QString &output, const QString &format,
                                    const QString &password, bool isAltStream = false);
    bool hasListedSourceArchive(const QString &archive) const;
    // sourceFolder is the exact bound Agent prefix removed by the source callback.
    bool validateExtraction(const QString &destination, const QStringList &selected,
                            const ExtractionOptions &options, QString *error,
                            bool nativeSource = false, const QString &sourceFolder = {},
                            const CArc *nativeMetadata = nullptr) const;
    static bool validateExtractionEntries(const QList<ArchiveEntry> &entries,
                                          const QString &destination, const QStringList &selected,
                                          const ExtractionOptions &options, QString *error,
                                          bool nativeSource = false,
                                          const QString &sourceFolder = {});
    static bool validateExtractionOutput(const QString &destination, const QString &path,
                                         bool directory, QString *error);
    static bool validateExtractionStreamOutput(const QString &destination, const QString &path,
                                               QString *error);
    static bool sourceUpdatePathIsSafe(const QString &path, bool allowAbsolute = false);
    static bool prepareSourceSelection(const QString &archive, const QStringList &sources,
                                       QString *root, QStringList *relativePaths, QString *error,
                                       bool storeLinks = false);
    virtual void cancel();
    // Native controls affect only this backend's live engine child.
    virtual HRESULT setPaused(bool paused);
    // Stop the transport before callers release its source/destination resources.
    void cancelAndWait();
    void providePassword(const QString &password);
    void provideOverwriteAnswer(int answer);

  signals:
    void entriesReady(const QList<ArchiveEntry> &entries);
    void infoReady(const ArchiveInfo &info);
    void progress(int percent, const QString &detail);
    void outputReceived(const QString &text);
    void finished(bool success, const QString &message);
    void passwordRequired();
    void overwriteRequired(const QString &existing, const ArchiveEntry &incoming);
    void dismissOverwriteRequested();
    void engineStarted();
    void phaseStarted();
    void codecCatalogReady(bool success, const QString &message);

  private:
    struct ListedSourceSnapshot {
        qint64 Size = -1;
        QDateTime Modified;
        QDateTime MetadataChanged;
        QList<ArchiveEntry> Entries;
        bool Encrypted = false;
        bool LimitedRead = false;
        QString Format;
        unsigned ContainerLevels = 1;
        bool ReadOnly = false;
    };
    enum class Job {
        None,
        List,
        Extract,
        Test,
        Hash,
        Create,
        CreatePrepare,
        CopyPrepare,
        CreateDeleteInputs,
        CreateVolumes,
        Add,
        Delete,
        Rename,
        ReplaceDelete,
        ReplaceAdd,
        SourceUpdate,
        SourcePlanDelete,
        SourcePlanFiles,
        SourcePlanDirectories,
        SourcePlanRename,
        SourceMutation,
        SourceExtract
    };
    void start(Job job, const QStringList &arguments, const QString &workingDirectory = {});
    std::shared_ptr<QProcess> makeProcess(const QString &executable);
    HRESULT processControlResult(HRESULT result) const;
    void complete(int exitCode, QProcess::ExitStatus status);
    bool advanceProcessJob(Job completed);
    bool failProcessJob(Job completed, int exitCode, QProcess::ExitStatus status);
    bool publishProcessMutation();
    void finishSourceExtractOperation(const QtArchiveReadOperation &operation);
    void consumeOutput(const QByteArray &chunk, bool flush = false);
    void consumeLine(const QByteArray &line);
    static bool safePath(const QString &path);
    bool prepareMutation(const QString &archive);
    void prepareArchiveCopy(const QString &source, const QString &output,
                            std::function<void()> continuation);
    void finishCopyPreparation();
    void reportCopyPreparation();
    void startSourcePlanStage(Job stage);
    bool startKeepNameSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                                 const QString &output, const QString &format,
                                 const QString &password, const QStringList &switches);
    bool validateSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                            const QString &output, const QString &format, const QString &password,
                            const std::optional<CompressionOptions> &options);
    static bool checkVolumeTargets(const QString &archive, QString *error);
    void startVolumeOutput();
    void finishVolumeOutput();
    void finishCreationLifecycle();
    bool startCreationDeletion();
    static bool compressionSwitches(const QString &format, const QString &password,
                                    bool encryptHeaders, const CompressionOptions &options,
                                    QStringList *switches, QString *error);
    bool selectCreationFormat(const QString &archive, const QStringList &sources,
                              const CompressionOptions &options, QString &format,
                              QString &error) const;
    bool restoreListedArchive(const QString &archive, bool allowLimited = true);
    void startListing(const QString &archive, const QString &password, const QString &format);

    std::shared_ptr<QProcess> process_;
    QString configuredEngine_;
    std::shared_ptr<const ArchiveCodecCatalog> codecCatalog_;
    std::unique_ptr<ArchiveCodecProbe> codecProbe_;
    bool codecDiscoveryActive_ = false;
    bool codecDiscoveryCancelled_ = false;
    CProcessPause processPause_;
    Job job_ = Job::None;
    QString output_;
    QByteArray pendingOutput_;
    QString archive_;
    QString createTarget_;
    bool createNativeSfx_ = false;
    QString createSfxModule_;
    QString createVolumeTarget_;
    QString createTemporary_;
    bool createSetArcMTime_ = false;
    struct CreateStage {
        QString directory;
        QStringList arguments;
    };
    QList<CreateStage> createStages_;
    QString createPassword_;
    CRecordVector<UInt64> createVolumeSizes_;
    std::unique_ptr<QtArchiveVolumeOutput> volumeOutput_;
    std::unique_ptr<QtArchiveCreationLifecycle> creationLifecycle_;
    std::unique_ptr<QtArchiveHashOperation> hashOperation_;
    std::unique_ptr<QtArchiveReadOperation> readOperation_;
    std::shared_ptr<NativeArchiveListing> nativeListing_;
    quint64 readGeneration_ = 0;
    std::unique_ptr<QtArchiveCopyPreparation> copyPreparation_;
    std::function<void()> copyContinuation_;
    QTimer copyProgressTimer_{this};
    QString copyProgressDetail_;
    quint64 copyGeneration_ = 0;
    quint64 hashGeneration_ = 0;
    quint64 creationGeneration_ = 0;
    quint64 volumeGeneration_ = 0;
    QTimer workerProgressTimer_{this};
    QMutex workerProgressMutex_;
    quint64 workerProgressGeneration_ = 0;
    bool workerProgressPending_ = false;
    int workerProgressPercent_ = 0;
    QString workerProgressPath_;
    quint64 beginWorkerProgress();
    bool startWorkerPhase(QThread &worker, std::function<void()> launch = {});
    void postWorkerProgress(quint64 generation, int percent, const QString &path);
    void reportWorkerProgress();
    void finishWorkerProgress();
    void finishHashOperation();
    void startNativeRead(Job job, const QString &archive, const QString &password,
                         const QString &format = {},
                         std::shared_ptr<const QtArchiveDirectoryExtractRequest> directory = {});
    void finishReadOperation();
    static HRESULT validateNativeHashReads(const ArchiveNativeOpen &reader, const UInt32 *indices,
                                           UInt32 count);
    QString addBackup_;
    QString replacementEntry_;
    QString replacementRoot_;
    SourceUpdatePlan sourcePlan_;
    QString sourcePlanPassword_;
    QString sourcePlanFormat_;
    QStringList sourcePlanSwitches_;
    QList<ArchiveEntry> entries_;
    QHash<QString, ListedSourceSnapshot> listedSourceArchives_;
    bool listed_ = false;
    bool cancelled_ = false;
    HRESULT lastResult_ = E_FAIL;
    QString pendingPassword_;
    bool passwordPrompted_ = false;
    bool encrypted_ = false;
    bool sourceExtractRecovery_ = false;
    QString sourceExtractEntry_;
    bool sourceExtractAltStream_ = false;
    Int32 sourceExtractOperation_ = -1;
    bool overwritePromptActive_ = false;
};
