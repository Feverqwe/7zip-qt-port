#pragma once
#include "Windows/FileFind.h"
#include "Windows/FileIO.h"

// Only unavailable Win32 file services are projected here. VerCtrl.cpp keeps
// the original buffer, backup, comparison and commit/revert algorithm.
#ifdef _WIN32
using QtVerCtrlFileInfo = BY_HANDLE_FILE_INFORMATION;
using QtVerCtrlInFile = NWindows::NFile::NIO::CInFile;
using QtVerCtrlOutFile = NWindows::NFile::NIO::COutFile;
using QtVerCtrlEnumerator = NWindows::NFile::NFind::CEnumerator;
#else
struct QtVerCtrlFileInfo {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
};
class QtVerCtrlInFile : public NWindows::NFile::NIO::CInFile {
  public:
    bool GetFileInformation(QtVerCtrlFileInfo *info) const;
};
class QtVerCtrlOutFile : public NWindows::NFile::NIO::COutFile {
  public:
    bool Create_ALWAYS_or_NEW(CFSTR path, bool createAlways);
    bool Write(const void *data, UInt32 size, UInt32 &processedSize);
    bool SetTime(const FILETIME *creation, const FILETIME *access, const FILETIME *modified);
    bool SetMTime(const FILETIME *modified);
};
class QtVerCtrlEnumerator : public NWindows::NFile::NFind::CEnumerator {
  public:
    bool Next(NWindows::NFile::NFind::CDirEntry &entry) {
        bool found;
        return NWindows::NFile::NFind::CEnumerator::Next(entry, found) && found;
    }
};
#endif
bool QtVerCtrlSetFileAttrib(CFSTR path, DWORD attributes);
