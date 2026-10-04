// Update.h
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include "7zip/UI/Common/DirItem.h"
#include "7zip/UI/Common/UpdateAction.h"
#include "7zip/UI/Common/UpdateProduce.h"
#include "CPP/Common/Wildcard.h"
#include "UpdateCallback.h"

enum EArcNameMode { k_ArcNameMode_Smart, k_ArcNameMode_Exact, k_ArcNameMode_Add };

struct CArchivePath {
    UString OriginalPath;

    UString Prefix;        // path(folder) prefix including slash
    UString Name;          // base name
    UString BaseExtension; // archive type extension or "exe" extension
    UString VolExtension;  // archive type extension for volumes

    bool Temp;
    FString TempPrefix; // path(folder) for temp location
    FString TempPostfix;

    CArchivePath() : Temp(false) {}

    void ParseFromPath(const UString &path, EArcNameMode mode);
    UString GetPathWithoutExt() const { return Prefix + Name; }
    UString GetFinalPath() const;
    UString GetFinalVolPath(bool nativeSfx = false) const;
    FString GetTempPath() const;
};

struct CUpdateArchiveCommand {
    UString UserArchivePath;
    CArchivePath ArchivePath;
    NUpdateArchive::CActionSet ActionSet;
};

struct CCompressionMethodMode {
    bool Type_Defined;
    COpenType Type;
    CObjectVector<CProperty> Properties;

    CCompressionMethodMode() : Type_Defined(false) {}
};

namespace NRecursedType {
enum EEnum { kRecursed, kWildcardOnlyRecursed, kNonRecursed };
}

struct CRenamePair {
    UString OldName;
    UString NewName;
    bool WildcardParsing;
    NRecursedType::EEnum RecursedType;

    CRenamePair() : WildcardParsing(true), RecursedType(NRecursedType::kNonRecursed) {}

    bool Prepare();
    bool GetNewPath(bool isFolder, const UString &src, UString &dest) const;
};

struct CUpdateOptions {
    bool UpdateArchiveItself;
    bool SfxMode;

    bool PreserveATime;
    bool OpenShareForWrite;
    bool StopAfterOpenError;

    bool StdInMode;
    bool StdOutMode;

    bool EMailMode;
    bool EMailRemoveAfter;

    bool DeleteAfterCompressing;
    bool SetArcMTime;
    bool RenameMode;

    CBoolPair NtSecurity;
    CBoolPair AltStreams;
    CBoolPair HardLinks;
    CBoolPair SymLinks;

    CBoolPair StoreOwnerId;
    CBoolPair StoreOwnerName;

    EArcNameMode ArcNameMode;
    NWildcard::ECensorPathMode PathMode;

    CCompressionMethodMode MethodMode;

    CObjectVector<CUpdateArchiveCommand> Commands;
    CArchivePath ArchivePath;

    FString SfxModule;
    UString StdInFileName;
    UString EMailAddress;
    FString WorkingDir;
    // UString AddPathPrefix;

    CObjectVector<CRenamePair> RenamePairs;
    CRecordVector<UInt64> VolumesSizes;

    bool InitFormatIndex(const CCodecs *codecs, const CObjectVector<COpenType> &types,
                         const UString &arcPath);
    bool SetArcPath(const CCodecs *codecs, const UString &arcPath);

    CUpdateOptions()
        : UpdateArchiveItself(true), SfxMode(false),

          PreserveATime(false), OpenShareForWrite(false), StopAfterOpenError(false),

          StdInMode(false), StdOutMode(false),

          EMailMode(false), EMailRemoveAfter(false),

          DeleteAfterCompressing(false), SetArcMTime(false), RenameMode(false),

          ArcNameMode(k_ArcNameMode_Smart), PathMode(NWildcard::k_RelatPath)

    {}

    void SetActionCommand_Add() {
        Commands.Clear();
        CUpdateArchiveCommand c;
        c.ActionSet = NUpdateArchive::k_ActionSet_Add;
        Commands.Add(c);
    }
};

struct CUpdateErrorInfo {
    DWORD SystemError;
    AString Message;
    FStringVector FileNames;
    bool ThereIsError() const {
        return SystemError != 0 || !Message.IsEmpty() || !FileNames.IsEmpty();
    }
    HRESULT Get_HRESULT_Error() const {
        return SystemError == 0 ? E_FAIL : HRESULT_FROM_WIN32(SystemError);
    }
    void SetFromLastError(const char *message);
    HRESULT SetFromLastError(const char *message, const FString &fileName);
    HRESULT SetFromError_DWORD(const char *message, const FString &fileName, DWORD error);
    CUpdateErrorInfo() : SystemError(0) {}
};

struct CFinishArchiveStat {
    UInt64 OutArcFileSize;
    unsigned NumVolumes;
    bool IsMultiVolMode;

    CFinishArchiveStat() : OutArcFileSize(0), NumVolumes(0), IsMultiVolMode(false) {}
};

Z7_PURE_INTERFACES_BEGIN

// INTERFACE_IUpdateCallbackUI(x)
// INTERFACE_IDirItemsCallback(x)

#define Z7_IFACEN_IUpdateCallbackUI2(x)                                                            \
    virtual HRESULT OpenResult(const CCodecs *codecs, const CArchiveLink &arcLink,                 \
                               const wchar_t *name, HRESULT result)                                \
        x virtual HRESULT StartScanning() x virtual HRESULT FinishScanning(                        \
            const CDirItemsStat &st) x virtual HRESULT StartOpenArchive(const wchar_t *name)       \
            x virtual HRESULT StartArchive(const wchar_t *name, bool updating)                     \
                x virtual HRESULT FinishArchive(const CFinishArchiveStat &st)                      \
                    x virtual HRESULT DeletingAfterArchiving(const FString &path, bool isDir)      \
                        x virtual HRESULT FinishDeletingAfterArchiving()                           \
                            x virtual HRESULT MoveArc_Start(const wchar_t *srcTempPath,            \
                                                            const wchar_t *destFinalPath,          \
                                                            UInt64 size, Int32 updateMode)         \
                                x virtual HRESULT MoveArc_Progress(UInt64 total, UInt64 current)   \
                                    x virtual HRESULT MoveArc_Finish() x

DECLARE_INTERFACE(IUpdateCallbackUI2)
    : public IUpdateCallbackUI, public IDirItemsCallback{Z7_IFACE_PURE(IUpdateCallbackUI2)};
Z7_PURE_INTERFACES_END

// Original UpdateArchive SFX-module validation. Qt supplies the runtime folder
// instead of the Windows executable/registry installation directory.
HRESULT PrepareSfxModule(FString &module, const FString &baseFolder, CUpdateErrorInfo &errorInfo);
// Compress's separate SFX-module output for volume creation. The Qt worker
// supplies a staging path; the original streams and copy body remain here.
HRESULT WriteSfxModule(const FString &module, const FString &realPath, CUpdateErrorInfo &errorInfo);

// Narrow publication adapter for UpdateArchive's deletion phase. The archive
// writer is the official external engine; its transaction must finish first.
class IUpdateDeletionCallback {
  public:
    virtual ~IUpdateDeletionCallback() = default;
    virtual HRESULT DeletingAfterArchiving(const FString &path, bool isDir) = 0;
    virtual HRESULT FinishDeletingAfterArchiving() = 0;
};

HRESULT CheckDeleteAfterCompressingActionSet(const NUpdateArchive::CActionSet &actionSet);
void GetUpdateArchiveMTime(const CDirItems &dirItems, const CObjectVector<CArcItem> &arcItems,
                           const CRecordVector<CUpdatePair2> &updatePairs2, CFiTime &ft,
                           bool &ft_Defined);
HRESULT UpdateArchive(const CDirItems &dirItems, const CByteBuffer &processedItems,
                      const CBoolPair &SymLinks, IUpdateDeletionCallback *callback);

HRESULT UpdateArchive(CCodecs *codecs, const CObjectVector<COpenType> &types,
                      const UString &cmdArcPath2, NWildcard::CCensor &censor,
                      CUpdateOptions &options, CUpdateErrorInfo &errorInfo,
                      IOpenCallbackUI *openCallback, IUpdateCallbackUI2 *callback,
                      bool needSetPath);
