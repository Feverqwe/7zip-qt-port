// Adapted from 7-Zip CPP/7zip/UI/FileManager/VerCtrl.cpp
// 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17; Igor Pavlov, LGPL-2.1-or-later.
// See third_party/7zip/License.txt. Qt/platform boundaries only.

#include "Common/StringConvert.h"
#include "Common/StringToInt.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "qtdiagnostictext.h"
#include "qtverctrlfileservices.h"

#include "App.h"
#include "OverwriteDialog.h"
#include "RegistryUtils.h"

#include "7zip/UI/FileManager/resource.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;
using namespace NDir;

static FString ConvertPath_to_Ctrl(const FString &path) {
    FString s = path;
    s.Replace(FChar(':'), FChar('_'));
    return s;
}

struct CFileDataInfo {
    CByteBuffer Data;
    QtVerCtrlFileInfo Info;
    bool IsOpen;

    CFileDataInfo() : IsOpen(false) {}
    UInt64 GetSize() const { return (((UInt64)Info.nFileSizeHigh) << 32) + Info.nFileSizeLow; }
    bool Read(const FString &path);
};

bool CFileDataInfo::Read(const FString &path) {
    IsOpen = false;
    QtVerCtrlInFile file;
    if (!file.Open(path))
        return false;
    if (!file.GetFileInformation(&Info))
        return false;

    const UInt64 size = GetSize();
    const size_t size2 = (size_t)size;
    if (size2 != size || size2 > (1 << 28)) {
        SetLastError(1);
        return false;
    }

    Data.Alloc(size2);

    size_t processedSize;
    if (!file.ReadFull(Data, size2, processedSize))
        return false;
    if (processedSize != size2) {
        SetLastError(1);
        return false;
    }
    IsOpen = true;
    return true;
}

static bool CreateComplexDir_for_File(const FString &path) {
    FString resDirPrefix;
    FString resFileName;
    if (!GetFullPathAndSplit(path, resDirPrefix, resFileName))
        return false;
    return CreateComplexDir(resDirPrefix);
}

static bool ParseNumberString(const FString &s, UInt32 &number) {
    const FChar *end;
    UInt64 result = ConvertStringToUInt64(s, &end);
    if (*end != 0 || s.IsEmpty() || result > (UInt32)0x7FFFFFFF)
        return false;
    number = (UInt32)result;
    return true;
}

static void WriteFile(const FString &path, bool createAlways, const CFileDataInfo &fdi,
                      const CPanel &panel) {
    QtVerCtrlOutFile outFile;
    if (!outFile.Create_ALWAYS_or_NEW(path,
                                      createAlways)) // (createAlways = false) means CREATE_NEW
    {
        panel.MessageBox_LastError();
        return;
    }
    UInt32 processedSize;
    if (!outFile.Write(fdi.Data, (UInt32)fdi.Data.Size(), processedSize)) {
        panel.MessageBox_LastError();
        return;
    }
    if (processedSize != fdi.Data.Size()) {
        panel.MessageBox_Error(
            QtDiagnosticText(QT_TRANSLATE_NOOP("SevenZipDiagnostics", "Write error")));
        return;
    }
    if (!outFile.SetTime(&fdi.Info.ftCreationTime, &fdi.Info.ftLastAccessTime,
                         &fdi.Info.ftLastWriteTime)) {
        panel.MessageBox_LastError();
        return;
    }

    if (!QtVerCtrlSetFileAttrib(path, fdi.Info.dwFileAttributes)) {
        panel.MessageBox_LastError();
        return;
    }
}

static UInt64 FILETIME_to_UInt64(const FILETIME &ft) {
    return ft.dwLowDateTime | ((UInt64)ft.dwHighDateTime << 32);
}

static void UInt64_TO_FILETIME(UInt64 v, FILETIME &ft) {
    ft.dwLowDateTime = (DWORD)v;
    ft.dwHighDateTime = (DWORD)(v >> 32);
}

void CApp::VerCtrl(unsigned id) {
    const CPanel &panel = GetFocusedPanel();

    if (!panel.Is_IO_FS_Folder()) {
        panel.MessageBox_Error_UnsupportOperation();
        return;
    }

    CRecordVector<UInt32> indices;
    panel.Get_ItemIndices_Selected(indices);

    if (indices.Size() != 1) {
        // panel.MessageBox_Error_UnsupportOperation();
        return;
    }

    const FString path = us2fs(panel.GetItemFullPath(indices[0]));

    UString vercPath;
    ReadReg_VerCtrlPath(vercPath, _settingsPath);
    if (vercPath.IsEmpty())
        return;
    NName::NormalizeDirPathPrefix(vercPath);

    FString dirPrefix;
    FString fileName;
    if (!GetFullPathAndSplit(path, dirPrefix, fileName)) {
        panel.MessageBox_LastError();
        return;
    }

    const FString dirPrefix2 = us2fs(vercPath) + ConvertPath_to_Ctrl(dirPrefix);
    const FString path2 = dirPrefix2 + fileName;

    bool sameTime = false;
    bool sameData = false;
    bool areIdentical = false;

    CFileDataInfo fdi, fdi2;
    if (!fdi.Read(path)) {
        panel.MessageBox_LastError();
        return;
    }

    if (fdi2.Read(path2)) {
        sameData = (fdi.Data == fdi2.Data);
        sameTime = (CompareFileTime(&fdi.Info.ftLastWriteTime, &fdi2.Info.ftLastWriteTime) == 0);
        areIdentical = (sameData && sameTime);
    }

    const bool isReadOnly = (fdi.Info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;

    if (id == IDM_VER_EDIT) {
        if (!isReadOnly) {
            panel.MessageBox_Error(QtDiagnosticText(
                QT_TRANSLATE_NOOP("SevenZipDiagnostics", "File is not read-only")));
            return;
        }

        if (!areIdentical) {
            if (fdi2.IsOpen) {
                QtVerCtrlEnumerator enumerator;
                FString d2 = dirPrefix2;
                d2 += "_7vc";
                d2.Add_PathSepar();
                d2 += fileName;
                d2.Add_PathSepar();
                enumerator.SetDirPrefix(d2);
                NFind::CDirEntry fi;
                Int32 maxVal = -1;
                while (enumerator.Next(fi)) {
                    UInt32 val;
                    if (!ParseNumberString(fi.Name, val))
                        continue;
                    if ((Int32)val > maxVal)
                        maxVal = (Int32)val;
                }

                UInt32 next = (UInt32)maxVal + 1;
                if (maxVal < 0) {
                    next = 1;
                    if (!::CreateComplexDir_for_File(path2)) {
                        panel.MessageBox_LastError();
                        return;
                    }
                }

                // we rename old file2 to some name;
                FString path_num = d2;
                {
                    AString t;
                    t.Add_UInt32((UInt32)next);
                    while (t.Len() < 3)
                        t.InsertAtFront('0');
                    path_num += t;
                }

                if (maxVal < 0) {
                    if (!::CreateComplexDir_for_File(path_num)) {
                        panel.MessageBox_LastError();
                        return;
                    }
                }

                if (!NDir::MyMoveFile(path2, path_num)) {
                    panel.MessageBox_LastError();
                    return;
                }
            } else {
                if (!::CreateComplexDir_for_File(path2)) {
                    panel.MessageBox_LastError();
                    return;
                }
            }
            /*
            if (!::CopyFile(fs2fas(path), fs2fas(path2), TRUE))
            {
              panel.MessageBox_LastError();
              return;
            }
            */
            WriteFile(path2,
                      false, // (createAlways = false) means CREATE_NEW
                      fdi, panel);
        }

        if (!QtVerCtrlSetFileAttrib(path,
                                    fdi.Info.dwFileAttributes & ~(DWORD)FILE_ATTRIBUTE_READONLY)) {
            panel.MessageBox_LastError();
            return;
        }

        return;
    }

    if (isReadOnly) {
        panel.MessageBox_Error(
            QtDiagnosticText(QT_TRANSLATE_NOOP("SevenZipDiagnostics", "File is read-only")));
        return;
    }

    if (id == IDM_VER_COMMIT) {
        if (sameData) {
            if (!sameTime) {
                panel.MessageBox_Error(QtDiagnosticText(QT_TRANSLATE_NOOP(
                    "SevenZipDiagnostics", "Same data, but different timestamps.\n"
                                           "Use `Revert` to recover timestamp.")));
                return;
            }
        }

        const UInt64 timeStampOriginal = FILETIME_to_UInt64(fdi.Info.ftLastWriteTime);
        UInt64 timeStamp2 = 0;
        if (fdi2.IsOpen)
            timeStamp2 = FILETIME_to_UInt64(fdi2.Info.ftLastWriteTime);

        if (timeStampOriginal > timeStamp2) {
            const UInt64 k_Ntfs_prec = 10000000;
            UInt64 timeStamp = timeStampOriginal;
            const UInt32 k_precs[] = {60 * 60, 60, 2, 1};
            for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_precs); i++) {
                timeStamp = timeStampOriginal;
                const UInt64 prec = k_Ntfs_prec * k_precs[i];
                // timeStamp += prec - 1; // for rounding up
                timeStamp /= prec;
                timeStamp *= prec;
                if (timeStamp > timeStamp2)
                    break;
            }

            if (timeStamp != timeStampOriginal && timeStamp > timeStamp2) {
                FILETIME mTime;
                UInt64_TO_FILETIME(timeStamp, mTime);
                // NDir::SetFileAttrib(path, 0);
                {
                    QtVerCtrlOutFile outFile;
                    if (!outFile.Open_EXISTING(path)) {
                        panel.MessageBox_LastError();
                        return;
                        // if (::GetLastError() != ERROR_SUCCESS)
                        // throw "open error";
                    } else {
                        const UInt64 cTime = FILETIME_to_UInt64(fdi.Info.ftCreationTime);
                        if (cTime > timeStamp)
                            outFile.SetTime(&mTime, NULL, &mTime);
                        else
                            outFile.SetMTime(&mTime);
                    }
                }
            }
        }

        if (!QtVerCtrlSetFileAttrib(path, fdi.Info.dwFileAttributes | FILE_ATTRIBUTE_READONLY)) {
            panel.MessageBox_LastError();
            return;
        }
        return;
    }

    if (id == IDM_VER_REVERT) {
        if (!fdi2.IsOpen) {
            panel.MessageBox_Error(
                QtDiagnosticText(QT_TRANSLATE_NOOP("SevenZipDiagnostics", "No file to revert")));
            return;
        }
        if (!sameData || !sameTime) {
            if (!sameData) {
                /*
                UString m;
                m = "Are you sure you want to revert file ?";
                m.Add_LF();
                m += path;
                if (::MessageBoxW(panel.GetParent(), m, L"Version Control: File Revert", MB_OKCANCEL
                | MB_ICONQUESTION) != IDOK) return;
                */
                COverwriteDialog dialog;

                dialog.OldFileInfo.SetTime(fdi.Info.ftLastWriteTime);
                dialog.OldFileInfo.SetSize(fdi.GetSize());
                dialog.OldFileInfo.Path = fs2us(path);

                dialog.NewFileInfo.SetTime(fdi2.Info.ftLastWriteTime);
                dialog.NewFileInfo.SetSize(fdi2.GetSize());
                dialog.NewFileInfo.Path = fs2us(path2);

                dialog.ShowExtraButtons = false;
                dialog.DefaultButton_is_NO = true;

                INT_PTR writeAnswer = dialog.Create(panel.GetParent());

                if (writeAnswer != IDYES)
                    return;
            }

            WriteFile(path,
                      true, // (createAlways = true) means CREATE_ALWAYS
                      fdi2, panel);
        } else {
            if (!QtVerCtrlSetFileAttrib(path,
                                        fdi2.Info.dwFileAttributes | FILE_ATTRIBUTE_READONLY)) {
                panel.MessageBox_LastError();
                return;
            }
        }
        return;
    }

    // if (id == IDM_VER_DIFF)
    {
        if (!fdi2.IsOpen)
            return;
        DiffFiles(fs2us(path2), fs2us(path));
    }
}
