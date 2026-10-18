#include "7zip/Common/MultiOutStream.h"
#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
thread_local int injectedError = 0;
thread_local const char *replacementPath = nullptr;
thread_local int replacement = -1;
thread_local int released = -1;

void arm(int error, const QByteArray &path) {
    replacementPath = path.constData();
    replacement = released = -1;
    injectedError = error;
}

bool checkReplacement(const char *scenario, int error) {
    injectedError = 0;
    const bool valid = replacement >= 0 && replacement == released &&
                       ::write(replacement, "still owned", 11) == 11;
    if (replacement >= 0)
        ::syscall(SYS_close, replacement);
    if (!valid)
        std::fprintf(stderr, "%s, error %d: an unrelated reused descriptor was closed\n", scenario,
                     error);
    return valid;
}

template <class File, class Open>
bool fileCleanup(const char *scenario, int error, const QByteArray &source,
                 const QByteArray &sentinel, Open open) {
    bool valid = true;
    {
        File file;
        if (!open(file, source.constData()))
            return false;
        arm(error, sentinel);
        const bool closed = file.Close();
        const int actualError = errno;
        injectedError = 0;
        if (closed || actualError != error || !file.Close()) {
            std::fprintf(stderr, "%s: close did not preserve error %d\n", scenario, error);
            valid = false;
        }
    }
    return checkReplacement(scenario, error) && valid;
}

bool volumeCleanup(int error, bool finalFlush) {
    QTemporaryDir fixture;
    if (!fixture.isValid())
        return false;
    const QByteArray sentinel = QFile::encodeName(fixture.filePath("sentinel"));
    const QString prefix = fixture.filePath(QStringLiteral("volume 空."));
    CRecordVector<UInt64> sizes;
    sizes.Add(32);
    bool valid = true;
    {
        CMultiOutStream stream;
        stream.Init(sizes);
        stream.Prefix = us2fs(UString(prefix.toStdWString().c_str()));
        UInt32 processed = 0;
        if (static_cast<IOutStream &>(stream).Write("abc", 3, &processed) != S_OK || processed != 3)
            return false;
        arm(error, sentinel);
        unsigned count = 0;
        const HRESULT result =
            finalFlush ? stream.FinalFlush_and_CloseFiles(count) : stream.Destruct();
        injectedError = 0;
        if (result != HRESULT_FROM_WIN32(error) || stream.Destruct() != S_OK) {
            std::fprintf(stderr, "volume %s: close error %d was lost or cleanup failed\n",
                         finalFlush ? "flush" : "destructor", error);
            valid = false;
        }
    }
    valid = checkReplacement(finalFlush ? "volume flush" : "volume destructor", error) && valid;
    const QStringList remaining =
        QDir(fixture.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
    if (remaining != QStringList{QStringLiteral("sentinel")}) {
        std::fprintf(stderr, "volume close error %d left temporary output files\n", error);
        valid = false;
    }
    return valid;
}
} // namespace

// Model Linux's delayed close error: the descriptor has already been released
// when EIO/ENOSPC/EDQUOT/EINTR is returned. Reuse it immediately for another
// disposable file, making an unsafe retry deterministic without threads/timing.
extern "C" int close(int descriptor) {
    if (!injectedError)
        return static_cast<int>(::syscall(SYS_close, descriptor));
    const int error = injectedError;
    injectedError = 0;
    if (::syscall(SYS_close, descriptor) != 0)
        return -1;
    released = descriptor;
    replacement = static_cast<int>(
        ::syscall(SYS_openat, AT_FDCWD, replacementPath, O_WRONLY | O_CREAT | O_TRUNC, 0600));
    errno = error;
    return -1;
}

int main() {
    QTemporaryDir fixture;
    if (!fixture.isValid())
        return 1;
    const QByteArray input = QFile::encodeName(fixture.filePath(QStringLiteral("input 空")));
    const QByteArray output = QFile::encodeName(fixture.filePath(QStringLiteral("output 空")));
    const QByteArray sentinel = QFile::encodeName(fixture.filePath("sentinel"));
    {
        QFile file(QFile::decodeName(input));
        if (!file.open(QIODevice::WriteOnly) || file.write("source") != 6)
            return 1;
    }
    bool valid = true;
    for (const int error : {EIO, ENOSPC, EDQUOT, EINTR}) {
        valid = fileCleanup<NWindows::NFile::NIO::CInFile>(
                    "input", error, input, sentinel,
                    [](auto &file, const char *path) { return file.Open(path); }) &&
                valid;
        valid = fileCleanup<NWindows::NFile::NIO::COutFile>(
                    "output", error, output, sentinel,
                    [](auto &file, const char *path) { return file.Create_ALWAYS(path); }) &&
                valid;
        valid = volumeCleanup(error, false) && valid;
        valid = volumeCleanup(error, true) && valid;
    }
    return valid ? 0 : 1;
}
