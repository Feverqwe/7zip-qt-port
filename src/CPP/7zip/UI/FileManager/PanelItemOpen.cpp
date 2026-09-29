// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelItemOpen.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source archive-link/update bodies; Qt presentation and typed manager boundary.
#include "../Common/ExtractingFilePath.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "Common/AutoPtr.h"
#include "Common/StringConvert.h"
#include "ExtractCallback.h"
#include "FileFolderPluginOpen.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "Panel.h"
#include "ProgressDialog2.h"
#include "UpdateCallback100.h"
#include "Windows/ErrorMsg.h"
#include "Windows/FileName.h"
#include "Windows/PropVariantConv.h"
#include "Windows/System.h"
#include <QMessageBox>
#include <QWidget>

using namespace NWindows;
using namespace NFile;
using namespace NDir;
static const FChar *const kTempDirPrefix = FTEXT("7zO");

void CExitEventLauncher::Exit(bool hardExit) {
    // Native event/owned-thread lifecycle. Always join on POSIX/Qt: detaching
    // after the source Windows 100 ms hard-exit timeout could outlive this owner.
    (void)hardExit;
    if (_needExit) {
        _exitEvent.Set();
        _needExit = false;
    }
    if (_numActiveThreads == 0)
        return;
    FOR_VECTOR(i, _threads) {
        ::CThread &th = _threads[i];
        if (Thread_WasCreated(&th)) {
            if (Thread_Wait_Close(&th) != 0)
                continue;
            _numActiveThreads--;
        }
    }
}

struct CTmpProcessInfo : public CTempFileInfo {
    // No-handle association subset. Known/child-process tracking remains
    // mandatory; metadata fields stay compatible with the source update sender.
    CExitEventLauncher *Launcher;
    CPanel *Window;
    UString FullPathFolderPrefix;
    bool UsePassword;
    UString Password;
    bool ReadOnly;
    CTmpProcessInfo() : UsePassword(false), ReadOnly(false) {}
};

class CTmpProcessInfoRelease {
    CTmpProcessInfo *_tmpProcessInfo;

  public:
    bool _needDelete;
    CTmpProcessInfoRelease(CTmpProcessInfo &tpi) : _tmpProcessInfo(&tpi), _needDelete(true) {}
    ~CTmpProcessInfoRelease() {
        if (_needDelete)
            _tmpProcessInfo->DeleteDirAndFile();
    }
};

static THREAD_FUNC_DECL MyThreadFunction(void *param) {
    CMyUniquePtr<CTmpProcessInfo> tpi((CTmpProcessInfo *)param);
    // Source mainProcessWasSet == false path: an association may launch with
    // no waitable handle, so file lifetime continues until the exit event.
    bool needCheckTimestamp = true;
    for (;;) {
        NFind::CFileInfo newFileInfo;
        if (!newFileInfo.Find(tpi->FilePath))
            break;
        const WRes waitResult = tpi->Launcher->_exitEvent.Lock();
        if (waitResult == 0)
            break;
        return (THREAD_FUNC_RET_TYPE)(uintptr_t)1;
    }
    {
        NFind::CFileInfo newFileInfo;
        const bool finded = newFileInfo.Find(tpi->FilePath);
        if (!needCheckTimestamp || !finded || !tpi->WasChanged_from_TempFileInfo(newFileInfo))
            tpi->DeleteDirAndFile();
    }
    return 0;
}

void CPanel::MessageBox_Error_HRESULT(HRESULT result) const {
    MessageBox_Error(NError::MyFormatMessage(result));
}
void CPanel::MessageBox_LastError() const {
    MessageBox_Error_HRESULT(GetLastError_noZero_HRESULT());
}

QWidget *CPanel::GetParent() const { return _listView ? _listView->window() : nullptr; }

void CPanel::MessageBox_Error(const UString &message) const {
    QMessageBox box(QMessageBox::Critical, QStringLiteral("7-Zip"),
                    QString::fromWCharArray(message.Ptr()), QMessageBox::Ok, GetParent());
    box.setObjectName("panelArchiveError");
    box.setTextFormat(Qt::PlainText);
    box.exec();
}

void CPanel::MessageBox_Error_UnsupportOperation() const {
    MessageBox_Error(LangString(IDS_OPERATION_IS_NOT_SUPPORTED));
}

HRESULT CPanel::OpenAsArc(IInStream *inStream, const CTempFileInfo &tempFileInfo,
                          const UString &virtualFilePath, const UString &arcFormat,
                          COpenResult &openRes) {
    openRes.Encrypted = false;
    CFolderLink folderLink;
    (CTempFileInfo &)folderLink = tempFileInfo;
    if (inStream)
        folderLink.IsVirtual = true;
    else {
        if (!folderLink.FileInfo.Find(folderLink.FilePath))
            return GetLastError_noZero_HRESULT();
        if (folderLink.FileInfo.IsDir())
            return S_FALSE;
        folderLink.IsVirtual = false;
    }
    folderLink.VirtualPath = virtualFilePath;
    CFfpOpen ffp(_folderManager);
    ffp.SettingsPath = _settingsPath;
    const HRESULT res = ffp.OpenFileFolderPlugin(
        inStream, folderLink.FilePath.IsEmpty() ? us2fs(virtualFilePath) : folderLink.FilePath,
        arcFormat, GetParent());
    openRes.Encrypted = ffp.Encrypted;
    openRes.ErrorMessage = ffp.ErrorMessage;
    RINOK(res)
    folderLink.Password = ffp.Password;
    folderLink.UsePassword = ffp.Encrypted;
    if (_folder)
        folderLink.ParentFolderPath = GetFolderPath(_folder);
    else
        folderLink.ParentFolderPath = _currentFolderPrefix;
    if (!_parentFolders.IsEmpty())
        folderLink.ParentFolder = _folder;
    _parentFolders.Add(folderLink);
    _parentFolders.Back().Library.Attach(_library.Detach());
    ReleaseFolder();
    _library.Free();
    SetNewFolder(ffp.Folder);
    _library.Attach(ffp.Library.Detach());
    _flatMode = _flatModeForArc;
    _thereAreDeletedItems = false;
    if (!openRes.ErrorMessage.IsEmpty())
        MessageBox_Error(openRes.ErrorMessage);
    return S_OK;
}

HRESULT CPanel::OpenAsArc_Msg(IInStream *inStream, const CTempFileInfo &tempFileInfo,
                              const UString &virtualFilePath, const UString &arcFormat) {
    COpenResult opRes;
    HRESULT res = OpenAsArc(inStream, tempFileInfo, virtualFilePath, arcFormat, opRes);
    if (res == S_OK)
        return res;
    if (res == E_ABORT)
        return res;
    if (opRes.Encrypted || res != S_FALSE) {
        UString message;
        if (res == S_FALSE)
            message = MyFormatNew(opRes.Encrypted ? IDS_CANT_OPEN_ENCRYPTED_ARCHIVE
                                                  : IDS_CANT_OPEN_ARCHIVE,
                                  virtualFilePath);
        else
            message = HResultToMessage(res);
        MessageBox_Error(message);
    }
    return res;
}

HRESULT CPanel::OpenAsArc_Name(const UString &relPath, const UString &arcFormat) {
    CTempFileInfo tfi;
    tfi.RelPath = relPath;
    tfi.FolderPath = us2fs(GetFsPath());
    const UString fullPath = GetFsPath() + relPath;
    tfi.FilePath = us2fs(fullPath);
    return OpenAsArc_Msg(NULL, tfi, fullPath, arcFormat);
}

HRESULT CPanel::OpenParentArchiveFolder() {
    CDisableTimerProcessing disableTimerProcessing(*this);
    CDisableNotify disableNotify(*this);
    if (_parentFolders.Size() < 2)
        return S_OK;
    const CFolderLink &folderLinkPrev = _parentFolders[_parentFolders.Size() - 2];
    const CFolderLink &folderLink = _parentFolders.Back();
    NFind::CFileInfo newFileInfo;
    if (newFileInfo.Find(folderLink.FilePath)) {
        if (folderLink.WasChanged_from_FolderLink(newFileInfo)) {
            const UString message = MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE, folderLink.RelPath);
            QMessageBox box(QMessageBox::Question, QStringLiteral("7-Zip"),
                            QString::fromWCharArray(message.Ptr()),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, GetParent());
            box.setObjectName("modifiedArchiveQuestion");
            box.setTextFormat(Qt::PlainText);
            box.setDefaultButton(QMessageBox::Yes);
            box.setEscapeButton(QMessageBox::Cancel);
            if (box.exec() == QMessageBox::Yes) {
                if (OnOpenItemChanged(folderLink.FileIndex, fs2us(folderLink.FilePath),
                                      folderLinkPrev.UsePassword,
                                      folderLinkPrev.Password) != S_OK) {
                    MessageBox_Error(
                        MyFormatNew(IDS_CANNOT_UPDATE_FILE, fs2us(folderLink.FilePath)));
                    return S_OK;
                }
            }
        }
    }
    folderLink.DeleteDirAndFile();
    return S_OK;
}

HRESULT CPanel::OpenAsArc_Index(unsigned index, const wchar_t *type) {
    CDisableTimerProcessing disableTimerProcessing1(*this);
    CDisableNotify disableNotify(*this);
    HRESULT res = OpenAsArc_Name(GetItemRelPath2(index), type ? type : L"");
    if (res != S_OK) {
        RefreshTitle(true); // Restore the changed 7zFM title after an open error.
        return res;
    }
    RefreshListCtrl();
    return S_OK;
}

static const char *const kExeExtensions = " exe bat ps1 com lnk"
                                          " ";

static const char *const kStartExtensions =
#ifdef UNDER_CE
    " cab"
#endif
    " exe bat ps1 com lnk"
    " chm"
    " msi doc dot xls ppt pps wps wpt wks xlr wdb vsd pub"

    " docx docm dotx dotm xlsx xlsm xltx xltm xlsb xps"
    " xlam pptx pptm potx potm ppam ppsx ppsm vsdx xsn"
    " mpp"
    " msg"
    " dwf"

    " flv swf"

    " epub"
    " odt ods"
    " wb3"
    " pdf"
    " ps"
    " txt"
    " xml xsd xsl xslt hxk hxc htm html xhtml xht mht mhtml htw asp aspx css cgi jsp shtml"
    " h hpp hxx c cpp cxx m mm go swift"
    " awk sed hta js json php php3 php4 php5 phptml pl pm py pyo rb tcl ts vbs"
    " asm"
    " mak clw csproj vcproj sln dsp dsw"
    " ";

// bool FindExt(const char *p, const UString &name, AString &s);
bool FindExt(const char *p, const UString &name, CStringFinder &finder);

static bool DoItemAlwaysStart(const UString &name) {
    CStringFinder finder;
    return FindExt(kStartExtensions, name, finder);
}

void SplitCmdLineSmart(const UString &cmd, UString &prg, UString &params) {
    params.Empty();
    prg = cmd;
    prg.Trim();
    if (prg.Len() >= 2 && prg[0] == L'"') {
        int pos = prg.Find(L'"', 1);
        if (pos >= 0) {
            if ((unsigned)(pos + 1) == prg.Len() || prg[pos + 1] == ' ') {
                params = prg.Ptr((unsigned)(pos + 1));
                params.Trim();
                prg.DeleteFrom((unsigned)pos);
                prg.DeleteFrontal(1);
            }
        }
    }
}

bool CPanel::IsVirus_Message(const UString &name) {
    UString name2;

    const wchar_t cRLO = (wchar_t)0x202E;
    bool isVirus = false;
    bool isSpaceError = false;
    {
        unsigned numSpaces = 0;
        for (unsigned i = 0; i < name.Len(); i++) {
            const wchar_t c = name[i];
            switch (c) {
            case ' ':
            case '\t':
            case 0x00A0: // Non-Breaking Space
            case 0x2002: // En Space
            case 0x2003: // Em Space
            case 0x2004: // Three-Per-Em Space
            case 0x2005: // Four-Per-Em Space
            case 0x2006: // Six-Per-Em Space
            case 0x2007: // Figure Space
            case 0x2008: // Punctuation Space
            case 0x2009: // Thin Space
            case 0x200A: // Hair Space
            case 0x200B: // Zero Width Space
            case 0x202F: // Narrow Non-Breaking Space
            case 0x205F: // Medium Mathematical Space
            case 0x3000: // Ideographic Space
                numSpaces++;
                break;
            default:
                numSpaces = 0;
                break;
            }
            if (numSpaces <= 1)
                name2 += c;
            if (numSpaces >= 5) {
                isVirus = true;
                isSpaceError = true;
            }
        }
    }
    if (name2.Find(cRLO) >= 0) {
        const UString badString(cRLO);
        name2.Replace(badString, L"[RLO]");
        isVirus = true;
    }

#ifdef _WIN32
    {
        unsigned i;
        for (i = name2.Len(); i != 0;) {
            wchar_t c = name2[i - 1];
            if (c != '.' && c != ' ')
                break;
            i--;
            name2.ReplaceOneCharAtPos(i, '_');
        }
        if (i != name2.Len()) {
            CStringFinder finder;
            UString name3 = name2;
            name3.DeleteFrom(i);
            if (FindExt(kExeExtensions, name3, finder))
                isVirus = true;
        }
    }
#endif

    if (!isVirus)
        return false;

    UString s = LangString(IDS_VIRUS);

    if (!isSpaceError) {
        const int pos1 = s.Find(L'(');
        if (pos1 >= 0) {
            const int pos2 = s.Find(L')', (unsigned)pos1 + 1);
            if (pos2 >= 0) {
                s.Delete((unsigned)pos1, (unsigned)pos2 + 1 - (unsigned)pos1);
                if (pos1 > 0 && s[pos1 - 1] == ' ' && s[pos1] == '.')
                    s.Delete(pos1 - 1);
            }
        }
    }

    UString name3 = name;
    name3.Replace(L'\n', L'_');
    name2.Replace(L'\n', L'_');

    s.Add_LF();
    s += name2;
    s.Add_LF();
    s += name3;

    MessageBox_Error(s);
    return true;
}

void CPanel::OpenItemInArchive(unsigned index, bool tryInternal, bool tryExternal, bool editMode,
                               bool useEditor, const wchar_t *type) {
    // we don't want to change hash data here
    if (IsHashFolder())
        return;

    const UString name = GetItemName(index);
    const UString relPath = GetItemRelPath(index);

    if (tryExternal)
        if (IsVirus_Message(name))
            return;

    if (!_folderOperations) {
        MessageBox_Error_UnsupportOperation();
        return;
    }

    bool tryAsArchive = tryInternal && (!tryExternal || !DoItemAlwaysStart(name));

    const UString fullVirtPath = _currentFolderPrefix + relPath;

    CTempDir tempDirectory;
    if (!tempDirectory.Create(kTempDirPrefix)) {
        MessageBox_LastError();
        return;
    }

    FString tempDir = tempDirectory.GetPath();
    FString tempDirNorm = tempDir;
    NName::NormalizeDirPathPrefix(tempDirNorm);
    const FString tempFilePath = tempDirNorm + us2fs(Get_Correct_FsFile_Name(name));

    CTempFileInfo tempFileInfo;
    tempFileInfo.FileIndex = index;
    tempFileInfo.RelPath = relPath;
    tempFileInfo.FolderPath = tempDir;
    tempFileInfo.FilePath = tempFilePath;
    tempFileInfo.NeedDelete = true;

    if (tryAsArchive) {
        CMyComPtr<IInArchiveGetStream> getStream;
        _folder.QueryInterface(IID_IInArchiveGetStream, &getStream);
        if (getStream) {
            CMyComPtr<ISequentialInStream> subSeqStream;
            getStream->GetStream(index, &subSeqStream);
            if (subSeqStream) {
                CMyComPtr<IInStream> subStream;
                subSeqStream.QueryInterface(IID_IInStream, &subStream);
                if (subStream) {
                    HRESULT res =
                        OpenAsArc_Msg(subStream, tempFileInfo, fullVirtPath, type ? type : L""
                                      // , true // showErrorMessage
                        );
                    if (res == S_OK) {
                        tempDirectory.DisableDeleting();
                        RefreshListCtrl();
                        return;
                    }
                    if (res == E_ABORT || res != S_FALSE)
                        return;
                    if (!tryExternal)
                        return;
                    tryAsArchive = false;
                }
            }
        }
    }

    CRecordVector<UInt32> indices;
    indices.Add(index);

    UStringVector messages;

    bool usePassword = false;
    UString password;
    if (!_parentFolders.IsEmpty()) {
        const CFolderLink &fl = _parentFolders.Back();
        usePassword = fl.UsePassword;
        password = fl.Password;
    }

    /*
    #if defined(_WIN32) && !defined(UNDER_CE)
    CByteBuffer zoneBuf;
    #ifndef _UNICODE
    if (g_IsNT)
    #endif
    if (!_parentFolders.IsEmpty())
    {
      const CFolderLink &fl = _parentFolders.Front();
      if (!fl.IsVirtual && !fl.FilePath.IsEmpty())
        ReadZoneFile(fl.FilePath + k_ZoneId_StreamName, zoneBuf);
    }
    #endif
    */

    CVirtFileSystem *virtFileSystemSpec = NULL;
    CMyComPtr<ISequentialOutStream> virtFileSystem;

    const bool isAltStream = IsItem_AltStream(index);

    CCopyToOptions options;
    options.includeAltStreams = true;
    options.replaceAltStreamChars = isAltStream;
    {
        // CContextMenuInfo ci;
        // ci.Load();
        // if (ci.WriteZone != (UInt32)(Int32)-1)
        // we use kAll when we unpack just one file.
        options.ZoneIdMode = NExtract::NZoneIdMode::kAll;
        options.NeedRegistryZone = false;
    }

    size_t g_RAM_Size = 0;
    const bool g_RAM_Size_Defined = NSystem::GetRamSize(g_RAM_Size);
    if (tryAsArchive) {
        // actually we want to get sum: size of main file plus sizes of altStreams.
        // but now there is no interface to get altStreams sizes.
        NCOM::CPropVariant prop;
        _folder->GetProperty(index, kpidSize, &prop);
        const size_t fileLimit = g_RAM_Size_Defined
                                     ? (MyMax(_parentFolders.Size() + 1, 8u) < sizeof(size_t) * 8
                                            ? g_RAM_Size >> MyMax(_parentFolders.Size() + 1, 8u)
                                            : 0)
                                     : 1u << 22;
        UInt64 fileSize = 0;
        if (!ConvertPropVariantToUInt64(prop, fileSize))
            fileSize = fileLimit;
#if 0 // 1 : for debug
    fileLimit = 1;
#endif

        if (fileSize <= fileLimit) {
            options.streamMode = true;
            virtFileSystemSpec = new CVirtFileSystem;
            virtFileSystem = virtFileSystemSpec;
            virtFileSystemSpec->FileName = name;
            virtFileSystemSpec->IsAltStreamFile = isAltStream;

#if defined(_WIN32) && !defined(UNDER_CE)
#ifndef _UNICODE
            if (g_IsNT)
#endif
            {
                Get_ZoneId_Stream_from_ParentFolders(virtFileSystemSpec->ZoneBuf);
                options.ZoneBuf = virtFileSystemSpec->ZoneBuf;
            }
#endif

            virtFileSystemSpec->MaxTotalAllocSize =
                (size_t)fileSize +
                (1 << 16); // we allow additional total size for small alt streams.
            virtFileSystemSpec->DirPrefix = tempDirNorm;
            // options.VirtFileSystem = virtFileSystem;
            options.VirtFileSystemSpec = virtFileSystemSpec;
        }
    }

    options.folder = fs2us(tempDirNorm);
    options.showErrorMessages = true;

    const HRESULT result = CopyTo(options, indices, &messages, usePassword, password);

    if (!_parentFolders.IsEmpty()) {
        CFolderLink &fl = _parentFolders.Back();
        fl.UsePassword = usePassword;
        fl.Password = password;
    }

    if (!messages.IsEmpty())
        return;
    if (result != S_OK) {
        if (result != E_ABORT)
            MessageBox_Error_HRESULT(result);
        return;
    }

    if (virtFileSystemSpec && !virtFileSystemSpec->WasStreamFlushedToFS()) {
        int index_in_Files = virtFileSystemSpec->Index_of_MainExtractedFile_in_Files;
        if (index_in_Files < 0) {
            if (virtFileSystemSpec->Files.Size() != 1) {
                MessageBox_Error_HRESULT(E_FAIL);
                return;
            }
            // it's not expected case that index was not set, but we support that case
            index_in_Files = 0;
        }
        {
            const CVirtFile &file = virtFileSystemSpec->Files[index_in_Files];
            CMyComPtr2_Create<IInStream, CBufInStream> bufInStream;
            bufInStream->Init(file.Data, file.WrittenSize, virtFileSystem);
            const HRESULT res =
                OpenAsArc_Msg(bufInStream, tempFileInfo, fullVirtPath, type ? type : L""
                              // , encrypted
                              // , true // showErrorMessage
                );
            if (res == S_OK) {
                if (virtFileSystemSpec->Index_of_ZoneBuf_AltStream_in_Files >= 0 &&
                    !_parentFolders.IsEmpty()) {
                    const CVirtFile &fileZone =
                        virtFileSystemSpec
                            ->Files[virtFileSystemSpec->Index_of_ZoneBuf_AltStream_in_Files];
                    _parentFolders.Back().ZoneBuf.CopyFrom(fileZone.Data, fileZone.WrittenSize);
                }

                tempDirectory.DisableDeleting();
                RefreshListCtrl();
                return;
            }
            if (res == E_ABORT || res != S_FALSE)
                return;
            if (!tryExternal)
                return;
            tryAsArchive = false;
            if (virtFileSystemSpec->FlushToDisk(true) != S_OK)
                return;
        }
    }

    /*
    #if defined(_WIN32) && !defined(UNDER_CE)
    if (zoneBuf.Size() != 0)
    {
      if (NFind::DoesFileExist_Raw(tempFilePath))
      {
        WriteZoneFile(tempFilePath + k_ZoneId_StreamName, zoneBuf);
      }
    }
    #endif
    */

    if (tryAsArchive) {
        const HRESULT res = OpenAsArc_Msg(NULL, tempFileInfo, fullVirtPath, type ? type : L""
                                          // , encrypted
                                          // , true // showErrorMessage
        );
        if (res == S_OK) {
            tempDirectory.DisableDeleting();
            RefreshListCtrl();
            return;
        }
        if (res == E_ABORT || res != S_FALSE)
            return;
    }

    if (!tryExternal)
        return;

    if (!_externalFileServices || editMode) {
        // Known-process/editor transport remains mandatory P0-04/P1-08 work.
        MessageBox_Error_UnsupportOperation();
        return;
    }
    CMyUniquePtr<CTmpProcessInfo> tpi(new CTmpProcessInfo());
    tpi->FolderPath = tempDir;
    tpi->FilePath = tempFilePath;
    tpi->NeedDelete = true;
    tpi->UsePassword = usePassword;
    tpi->Password = password;
    tpi->ReadOnly = IsThereReadOnlyFolder();
    if (IsHashFolder())
        tpi->ReadOnly = true;
    if (!tpi->FileInfo.Find(tempFilePath))
        return;
    CTmpProcessInfoRelease tmpProcessInfoRelease(*tpi);
    if (!_exitEventLauncher)
        _exitEventLauncher = std::make_unique<CExitEventLauncher>();
    // Allocate tracking before launch/thread creation; Add must not throw while
    // the worker already owns this temp record.
    _exitEventLauncher->_threads.ReserveOnePosition();
    const HRESULT res = _externalFileServices->StartApplication(fs2us(tempDirNorm),
                                                                fs2us(tempFilePath), GetParent());
    // No handle never proves completion of a successful association launch.
    if (res != S_OK)
        return;
    tpi->Window = this;
    tpi->FullPathFolderPrefix = _currentFolderPrefix;
    tpi->FileIndex = index;
    tpi->RelPath = relPath;
    tpi->Launcher = _exitEventLauncher.get();
    ::CThread th;
    Thread_CONSTRUCT(&th) if (Thread_Create(&th, MyThreadFunction, tpi.get()) != 0) throw 271824;
    _exitEventLauncher->_threads.AddInReserved(th);
    _exitEventLauncher->_numActiveThreads++;
    tempDirectory.DisableDeleting();
    tpi.release();
    tmpProcessInfoRelease._needDelete = false;
}

// Native OnOpenItemChanged message body. A typed request replaces LPARAM and
// the window handle; the external service retains the temp file during the call.
bool CPanel::OnOpenItemChanged(const CTempFileInfo &tpi, const UString &fullPathFolderPrefix,
                               bool usePassword, const UString &password) {
    if (fullPathFolderPrefix != _currentFolderPrefix || !_folder)
        return false;
    UInt32 fileIndex = tpi.FileIndex;
    UInt32 numItems;
    _folder->GetNumberOfItems(&numItems);
    // Source limitation: duplicate names can still make a relocated index
    // ambiguous after another archive update.
    if (fileIndex >= numItems || GetItemRelPath(fileIndex) != tpi.RelPath) {
        UInt32 i;
        for (i = 0; i < numItems; i++)
            if (GetItemRelPath(i) == tpi.RelPath)
                break;
        if (i == numItems)
            return false;
        fileIndex = i;
    }
    CSelectedState state;
    SaveSelectedState(state);
    CDisableNotify disableNotify(*this);
    HRESULT result = OnOpenItemChanged(fileIndex, fs2us(tpi.FilePath), usePassword, password);
    RefreshListCtrl(state);
    if (result != S_OK)
        return false;
    return true;
}

class CThreadCopyFrom : public CProgressThreadVirt {
    HRESULT ProcessVirt() override;

  public:
    UString FullPath;
    UInt32 ItemIndex;
    CMyComPtr<IFolderOperations> FolderOperations;
    CMyComPtr<IProgress> UpdateCallback;
    CUpdateCallback100Imp *UpdateCallbackSpec;
};

HRESULT CThreadCopyFrom::ProcessVirt() {
    return FolderOperations->CopyFromFile(ItemIndex, FullPath, UpdateCallback);
}

HRESULT CPanel::OnOpenItemChanged(UInt32 index, const wchar_t *fullFilePath, bool usePassword,
                                  const UString &password) {
    if (!_folderOperations) {
        MessageBox_Error_UnsupportOperation();
        return E_FAIL;
    }
    CThreadCopyFrom t;
    t.UpdateCallbackSpec = new CUpdateCallback100Imp;
    t.UpdateCallback = t.UpdateCallbackSpec;
    t.UpdateCallbackSpec->ProgressDialog = &t;
    t.ItemIndex = index;
    t.FullPath = fullFilePath;
    t.FolderOperations = _folderOperations;
    std::unique_ptr<IQtProgressTransport> transport;
    if (auto *services =
            dynamic_cast<IQtProgressServices *>((IFolderOperations *)_folderOperations))
        transport = services->CreateProgressTransport(t);
    t.UpdateCallbackSpec->Init();
    t.UpdateCallbackSpec->PasswordIsDefined = usePassword;
    t.UpdateCallbackSpec->Password = password;
    RINOK(t.Create(GetItemName(index), GetParent()))
    return t.Result;
}
