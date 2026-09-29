// Adapted from 7-Zip UI/FileManager/FSFolder.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "Common/MyVector.h"
#include "Windows/FileFind.h"
#include <QFileDevice>
#include <QFileInfo>
#include <QString>
#include <functional>

namespace NFsFolder {
struct CDirItem : public NWindows::NFile::NFind::CFileInfo {
    UInt64 PackSize = 0;
    bool PackSize_Defined = false;
    bool FolderStat_Defined = false;
    int Parent = -1;
    UInt64 NumFolders = 0;
    UInt64 NumFiles = 0;
};

// Partial source folder owner. Runtime panel loading is integrated separately;
// mutations retain their Qt service boundary and existing callbacks.
class CFSFolder final : public IFolderFolder,
                        public IFolderSetFlatMode,
                        public IFolderWasChanged,
                        public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_3(IFolderFolder, IFolderSetFlatMode, IFolderWasChanged)
  public:
    CFSFolder() = default;
    explicit CFSFolder(QString path);
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IFolderSetFlatMode)
    Z7_IFACE_COM7_IMP(IFolderWasChanged)
    HRESULT Init(const FString &path);
    HRESULT InitToRoot() { return Init((FString)FSTRING_PATH_SEPARATOR); }
    CObjectVector<CDirItem> Files;
    FStringVector Folders;
    void Clear() {
        Files.Clear();
        Folders.Clear();
        // Streams.Clear();
    }
    QString ErrorMessage;
    std::function<bool(quint64)> SetTotal;
    std::function<bool(quint64)> SetCompleted;
    // LoadItems uses these existing integration callbacks for count progress
    // and cooperative cancellation; no Qt widgets run in the source body.
    std::function<bool(const QString &)> CheckBreak;
    void AddPath(const QString &path);
    FString GetRelPath(const CDirItem &item) const;
    void GetAbsPath(const QString &name, QString &absPath) const;
    bool CreateFolder(const QString &name);
    bool CreateFile(const QString &name);
    bool Rename(UInt32 index, const QString &newName);
    bool Delete(const UInt32 *indices, UInt32 numItems);

  private:
    HRESULT LoadSubItems(int dirItem, const FString &relPrefix);
    HRESULT BindToFolderSpec(CFSTR name, IFolderFolder **resultFolder);
    FString _path;
    bool _flatMode = false;
#ifdef _WIN32
    NWindows::NFile::NFind::CFindChangeNotification _findChangeNotification;
#endif
};

struct CCopyStateIO {
    int ErrorFileIndex = -1;
    QString ErrorMessage;
    quint64 CurrentSize = 0;
    quint64 StartPos = 0;
    bool DeleteSrcFile = false;
    std::function<bool(quint64)> Progress;
    bool MyCopyFile(const QString &inPath, const QString &outPath, QFileDevice::Permissions attrib);
};
bool IsDestChild(const QString &src, const QString &dest);

// Read/write branch of upstream CCopyState. The transaction owner retains
// move/rollback; this state copies into an unpublished destination only.
struct CCopyState {
    quint64 StartPos = 0;
    QString ErrorMessage;
    std::function<bool(quint64)> Progress;
    std::function<void(const QString &)> CurrentFile;
    bool CallProgress();
};
bool CopyFile_Ask(CCopyState &state, const QString &srcPath, const QFileInfo &srcFileInfo,
                  const QString &destPath);
bool CopyFolder(CCopyState &state, const QString &srcPath, const QString &destPath);
} // namespace NFsFolder
