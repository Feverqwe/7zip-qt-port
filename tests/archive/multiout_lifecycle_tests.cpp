#include "Common/StringConvert.h"
#include "archive/streams/MultiOutStream.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
thread_local int allocationsBeforeFailure = -1;
thread_local bool allocationFailed = false;
} // namespace

void *operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void *memory = std::malloc(size ? size : 1))
        return memory;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

static bool prefixFailureCleanup() {
    // Update and the Qt volume worker set the name before Init. A failed
    // allocation must still permit cleanup of the partially prepared stream.
    // Model reused heap storage instead of relying on zero-filled fresh pages.
    alignas(CMultiOutStream) unsigned char storage[sizeof(CMultiOutStream)];
    std::memset(storage, 0xa5, sizeof(storage));
    auto *stream = new (storage) CMultiOutStream;
    UString name;
    for (unsigned i = 0; i != 1024; ++i)
        name += L'x';
    const FString prefix = us2fs(name);
    bool failed = false;
    allocationsBeforeFailure = 0;
    try {
        stream->Prefix = prefix;
    } catch (const std::bad_alloc &) {
        failed = true;
    }
    allocationsBeforeFailure = -1;
    const HRESULT cleanup = stream->Destruct();
    stream->~CMultiOutStream();
    if (!failed) {
        std::fputs("volume-name allocation failure was not injected\n", stderr);
        return false;
    }
    if (cleanup != S_OK) {
        std::fputs("cleanup failed after volume-name allocation failure\n", stderr);
        return false;
    }
    return true;
}

static bool writeFailureCleanup() {
    QTemporaryDir fixture;
    if (!fixture.isValid())
        return false;
    const QString prefix = fixture.filePath(QStringLiteral("volume 空."));
    const QString collision = prefix + "001.tmp";
    {
        QFile file(collision);
        if (!file.open(QIODevice::WriteOnly) || file.write("preserved") != 9)
            return false;
    }
    const QStringList preserved{QFileInfo(collision).fileName()};
    CRecordVector<UInt64> sizes;
    sizes.Add(2);
    bool valid = true;
    // One three-byte write crosses a volume boundary. Visit its allocation
    // failures until an uninjected write succeeds, without pinning their count.
    for (int budget = 0;; ++budget) {
        HRESULT result, cleanup;
        bool injected;
        {
            CMultiOutStream stream;
            stream.Init(sizes);
            stream.Prefix = us2fs(UString(prefix.toStdWString().c_str()));
            UInt32 processed = 0;
            allocationFailed = false;
            allocationsBeforeFailure = budget;
            result = static_cast<IOutStream &>(stream).Write("abc", 3, &processed);
            allocationsBeforeFailure = -1;
            injected = allocationFailed;
            cleanup = stream.Destruct();
        }
        if (result != (injected ? E_OUTOFMEMORY : S_OK) || cleanup != S_OK) {
            std::fprintf(stderr, "allocation budget %d: write/cleanup result failed\n", budget);
            valid = false;
        }
        const QStringList remaining =
            QDir(fixture.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
        QFile file(collision);
        if (remaining != preserved || !file.open(QIODevice::ReadOnly) ||
            file.readAll() != QByteArray("preserved")) {
            std::fprintf(stderr, "allocation budget %d: volume ownership cleanup failed\n", budget);
            for (const QString &name : remaining)
                std::fprintf(stderr, "remaining: %s\n", qPrintable(name));
            return false;
        }
        if (!injected)
            return valid;
    }
}

static bool preallocationThenTruncation() {
    QTemporaryDir fixture;
    if (!fixture.isValid())
        return false;
    const QString prefix = fixture.filePath(QStringLiteral("preallocated 空."));
    CRecordVector<UInt64> sizes;
    sizes.Add(2);
    CMultiOutStream stream;
    stream.Init(sizes);
    stream.Prefix = us2fs(UString(prefix.toStdWString().c_str()));
    auto &output = static_cast<IOutStream &>(stream);
    unsigned count = 0;
    if (output.SetSize(5) != S_OK || output.SetSize(0) != S_OK ||
        stream.FinalFlush_and_CloseFiles(count) != S_OK)
        return false;
    QFile volume(prefix + "001");
    if (!volume.open(QIODevice::ReadOnly) || !volume.readAll().isEmpty()) {
        std::fputs("truncating a preallocated volume left stale file data\n", stderr);
        return false;
    }
    volume.close();
    return stream.Destruct() == S_OK;
}

int main() {
    const bool prefix = prefixFailureCleanup();
    const bool write = writeFailureCleanup();
    const bool truncate = preallocationThenTruncation();
    return prefix && write && truncate ? 0 : 1;
}
