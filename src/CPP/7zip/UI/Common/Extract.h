// Extract.h
// Copied from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.

#ifndef ZIP7_INC_EXTRACT_H
#define ZIP7_INC_EXTRACT_H

#include "Windows/FileFind.h"

#include "7zip/Archive/IArchive.h"

#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include "7zip/UI/Common/ExtractMode.h"
#include "7zip/UI/Common/Property.h"
#include "ArchiveExtractCallback.h"

#include "7zip/UI/Common/LoadCodecs.h"

namespace NExtractOutDirMode {
enum EEnum { k_Direct = 0, k_AddArcName, k_ReplaceAsterisk };
}

struct CExtractOptionsBase {
    CBoolPair ElimDup;

    bool ExcludeDirItems;
    bool ExcludeFileItems;

    bool PathMode_Force;
    bool OverwriteMode_Force;
    NExtract::NPathMode::EEnum PathMode;
    NExtract::NOverwriteMode::EEnum OverwriteMode;
    NExtract::NZoneIdMode::EEnum ZoneMode;
    NExtractOutDirMode::EEnum OutDirMode;

    CExtractNtOptions NtOptions;

    FString OutputDir; // normalized : with path separator at the end
    UString HashDir;

    CExtractOptionsBase()
        : ExcludeDirItems(false), ExcludeFileItems(false), PathMode_Force(false),
          OverwriteMode_Force(false), PathMode(NExtract::NPathMode::kFullPaths),
          OverwriteMode(NExtract::NOverwriteMode::kAsk), ZoneMode(NExtract::NZoneIdMode::kNone),
          OutDirMode(NExtractOutDirMode::k_ReplaceAsterisk) {}
};

struct CExtractOptions : public CExtractOptionsBase {
    bool StdInMode;
    bool StdOutMode;
    bool YesToAll;
    bool TestMode;

// bool ShowDialog;
// bool PasswordEnabled;
// UString Password;
#ifndef Z7_SFX
    CObjectVector<CProperty> Properties;
#endif

    /*
    #ifdef Z7_EXTERNAL_CODECS
    CCodecs *Codecs;
    #endif
    */

    CExtractOptions() : StdInMode(false), StdOutMode(false), YesToAll(false), TestMode(false) {}
};

struct CDecompressStat {
    UInt64 NumArchives;
    UInt64 UnpackSize;
    UInt64 AltStreams_UnpackSize;
    UInt64 PackSize;
    UInt64 NumFolders;
    UInt64 NumFiles;
    UInt64 NumAltStreams;

    void Clear() {
        NumArchives = UnpackSize = AltStreams_UnpackSize = PackSize = NumFolders = NumFiles =
            NumAltStreams = 0;
    }
};

// Integration boundary for a retained native reader. Selection, output paths,
// overwrite and closing remain in DecompressArchive/ArchiveExtractCallback.
struct IQtArchiveExtractService {
    virtual ~IQtArchiveExtractService() = default;
    virtual HRESULT Extract(const UInt32 *indices, UInt32 count, Int32 testMode,
                            IArchiveExtractCallback *callback) = 0;
};

HRESULT DecompressArchive(CCodecs *codecs, const CArchiveLink &arcLink, UInt64 packSize,
                          const NWildcard::CCensorNode &wildcardCensor,
                          const CExtractOptions &options, bool calcCrc,
                          IExtractCallbackUI *callback, IFolderArchiveExtractCallback *callbackFAE,
                          CArchiveExtractCallback *ecs, UString &errorMessage,
                          UInt64 &stdInProcessed, IQtArchiveExtractService *qtExtract = nullptr,
                          const UStringVector *qtRemovePathParts = nullptr,
                          bool qtRemovePartsForAltStreams = false);

HRESULT Extract(
    // DECL_EXTERNAL_CODECS_LOC_VARS
    CCodecs *codecs, const CObjectVector<COpenType> &types, const CIntVector &excludedFormats,
    UStringVector &archivePaths, UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor, const CExtractOptions &options,
    IOpenCallbackUI *openCallback, IExtractCallbackUI *extractCallback,
    IFolderArchiveExtractCallback *faeCallback,
#ifndef Z7_SFX
    IHashCalc *hash,
#endif
    UString &errorMessage, CDecompressStat &st, IQtArchiveOpenService *qtOpenService = nullptr);

#endif
