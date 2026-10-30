// clang-format off
#include "Common/MyInitGuid.h"
// clang-format on
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QFile>
#include <QFileInfo>
#include <QLibrary>
#include <QProcess>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <array>

namespace {
const QString filename = QStringLiteral("файл_空.bin");
QByteArray contents() {
    QByteArray bytes;
    for (int i = 0; i < 5000; ++i)
        bytes += QStringLiteral("XFS: файл 空\n").toUtf8();
    return bytes + "tail\n";
}
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QFile file_;

  public:
    HRESULT failure = S_OK;
    explicit Input(const QString &path) : file_(path) {
        if (!file_.open(QIODevice::ReadOnly))
            qFatal("fixture input open failed");
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        const qint64 count = file_.read(static_cast<char *>(data), size);
        if (count < 0)
            return E_FAIL;
        if (processed)
            *processed = UInt32(count);
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const UInt64 base = origin == STREAM_SEEK_SET   ? 0
                            : origin == STREAM_SEEK_CUR ? UInt64(file_.pos())
                                                        : UInt64(file_.size());
        if ((offset < 0 && UInt64(-(offset + 1)) + 1 > base) ||
            (offset >= 0 && UInt64(offset) > UInt64(INT64_MAX) - base))
            return E_INVALIDARG;
        const UInt64 next = offset < 0 ? base - (UInt64(-(offset + 1)) + 1) : base + UInt64(offset);
        if (!file_.seek(qint64(next)))
            return E_FAIL;
        if (position)
            *position = next;
        return S_OK;
    }
};
class Output final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray bytes;
    bool fail = false;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return E_FAIL;
        bytes.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IProgress, IArchiveExtractCallback)
  public:
    CMyComPtr2<ISequentialOutStream, Output> output;
    Int32 result = -1;
    bool cancel = false;
    Extract() { output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = output;
        (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        return S_OK;
    }
};
class Opening final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    bool cancel = true;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return cancel ? E_ABORT : S_OK;
    }
};
struct Patch {
    QString path;
    quint64 offset;
    QByteArray old;
    Patch(QString name, quint64 at, const QByteArray &bytes) : path(std::move(name)), offset(at) {
        QFile f(path);
        if (!f.open(QIODevice::ReadWrite) || !f.seek(qint64(offset)))
            qFatal("fixture patch open failed");
        old = f.read(bytes.size());
        if (old.size() != bytes.size() || !f.seek(qint64(offset)) || f.write(bytes) != bytes.size())
            qFatal("fixture patch write failed");
    }
    ~Patch() {
        QFile f(path);
        if (!f.open(QIODevice::ReadWrite) || !f.seek(qint64(offset)) || f.write(old) != old.size())
            qFatal("fixture restore failed");
    }
};
QByteArray readAt(const QString &path, quint64 offset, qsizetype length) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly) || !f.seek(qint64(offset)))
        return {};
    return f.read(length);
}
quint64 inodeOffset(const QString &path, quint64 inode) {
    const auto sb = readAt(path, 0, 512);
    const unsigned inop = quint8(sb[123]), agbits = quint8(sb[124]);
    const quint64 agsize = qFromBigEndian<quint32>(sb.constData() + 84);
    const quint64 agino = inode & ((quint64(1) << (inop + agbits)) - 1);
    return ((inode >> (inop + agbits)) * agsize + (agino >> inop)) *
               qFromBigEndian<quint32>(sb.constData() + 4) +
           (agino & ((quint64(1) << inop) - 1)) * (quint64(1) << quint8(sb[122]));
}
} // namespace

class XfsModuleTests final : public QObject {
    Q_OBJECT
    QTemporaryDir fixtures_;
    QLibrary module_{QString::fromUtf8(SEVENZIP_XFS_MODULE)};
    CMyComPtr<IInArchive> archive_;
    CMyComPtr<IInStream> input_;
    Input *source_ = nullptr;
    QString image(int version) const {
        return fixtures_.filePath(QStringLiteral("v%1.xfs").arg(version));
    }
    HRESULT open(int version, IArchiveOpenCallback *callback = nullptr) {
        archive_->Close();
        input_ = source_ = new Input(image(version));
        return archive_->Open(input_, nullptr, callback);
    }
    UInt32 indexOf(const QString &name) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UInt32(-1);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QString::fromWCharArray(path.bstrVal) == name)
                return i;
        }
        return UInt32(-1);
    }
    quint64 fileInode() {
        NWindows::NCOM::CPropVariant value;
        if (archive_->GetProperty(indexOf(filename), kpidINode, &value) != S_OK ||
            value.vt != VT_UI8)
            return 0;
        return value.uhVal.QuadPart;
    }
    HRESULT extract(UInt32 index, IArchiveExtractCallback *callback) {
        return archive_->Extract(&index, 1, 0, callback);
    }
  private slots:
    void initTestCase() {
        QVERIFY(fixtures_.isValid());
        QProcess unpack;
        unpack.start(QString::fromUtf8(SEVENZIP_XFS_PYTHON),
                     {QString::fromUtf8(SEVENZIP_XFS_FIXTURES) + "/unpack.py",
                      QString::fromUtf8(SEVENZIP_XFS_FIXTURES), fixtures_.path()});
        QVERIFY(unpack.waitForFinished(30000));
        QCOMPARE(unpack.exitCode(), 0);
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(create);
        auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        QVERIFY(property);
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &value), S_OK);
        GUID clsid;
        memcpy(&clsid, value.bstrVal, sizeof(clsid));
        QCOMPARE(create(&clsid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
    }
    void mkfsImages_data() {
        QTest::addColumn<int>("version");
        QTest::newRow("v4") << 4;
        QTest::newRow("v5-bigtime") << 5;
    }
    void mkfsImages() {
        QFETCH(int, version);
        QCOMPARE(open(version), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 607u);
        QVERIFY(indexOf(QStringLiteral("many/entry-0599")) != UInt32(-1));
        for (const auto &name : {filename, QStringLiteral("nested/with-space.bin")}) {
            const auto index = indexOf(name);
            QVERIFY(index != UInt32(-1));
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(extract(index, callback), S_OK);
            QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(callback->output->bytes, contents());
        }
        NWindows::NCOM::CPropVariant link;
        QCOMPARE(archive_->GetProperty(indexOf(QStringLiteral("nested/link")), kpidSymLink, &link),
                 S_OK);
        QCOMPARE(QString::fromWCharArray(link.bstrVal), QStringLiteral("with-space.bin"));
        link.Clear();
        QCOMPARE(
            archive_->GetProperty(indexOf(QStringLiteral("nested/long-link")), kpidSymLink, &link),
            S_OK);
        QCOMPARE(QString::fromWCharArray(link.bstrVal), QString(200, 'a'));
    }
    void retainedIndependentStreams() {
        QCOMPARE(open(5), S_OK);
        CMyComPtr<IInArchiveGetStream> streams;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&streams)),
            S_OK);
        CMyComPtr<ISequentialInStream> first, second;
        const UInt32 index = indexOf(filename);
        QCOMPARE(streams->GetStream(index, &first), S_OK);
        QCOMPARE(streams->GetStream(index, &second), S_OK);
        CMyComPtr<IInStream> seek;
        QCOMPARE(first->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&seek)), S_OK);
        QCOMPARE(seek->Seek(65530, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Close(), S_OK);
        std::array<char, 32> bytes;
        UInt32 count = 0;
        QCOMPARE(first->Read(bytes.data(), bytes.size(), &count), S_OK);
        QCOMPARE(QByteArray(bytes.data(), count), contents().mid(65530, 32));
        QCOMPARE(second->Read(bytes.data(), bytes.size(), &count), S_OK);
        QCOMPARE(QByteArray(bytes.data(), count), contents().left(32));
        QCOMPARE(seek->Seek(-1, STREAM_SEEK_SET, nullptr), E_INVALIDARG);
    }
    void failuresAndCancellation() {
        CMyComPtr2<IArchiveOpenCallback, Opening> opening;
        opening.Create_if_Empty();
        QCOMPARE(open(5, opening), E_ABORT);
        UInt32 count = 1;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        QCOMPARE(open(5), S_OK);
        const UInt32 index = indexOf(filename);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        QCOMPARE(extract(index, callback), E_ABORT);
        QCOMPARE(callback->result, -1);
        callback->cancel = false;
        callback->output->fail = true;
        QCOMPARE(extract(index, callback), E_FAIL);
        callback->output->fail = false;
        source_->failure = E_OUTOFMEMORY;
        QCOMPARE(extract(index, callback), E_OUTOFMEMORY);
        source_->failure = S_OK;
        QCOMPARE(extract(index, callback), S_OK);
        QCOMPARE(callback->output->bytes, contents());
    }
    void corruptMetadataRejected() {
        QCOMPARE(open(5), S_OK);
        const quint64 offset = inodeOffset(image(5), fileInode());
        archive_->Close();
        {
            Patch corrupt(image(5), offset + 100, QByteArray(4, '\0'));
            QCOMPARE(open(5), S_FALSE);
        }
        QCOMPARE(open(5), S_OK);
        {
            Patch geometry(image(5), 120, QByteArray(1, '\x3f'));
            QCOMPARE(open(5), S_FALSE);
        }
        QCOMPARE(open(5), S_OK);
    }
    void unwrittenAndDistantExtents() {
        QCOMPARE(open(4), S_OK);
        const quint64 at = inodeOffset(image(4), fileInode());
        const auto inode = readAt(image(4), at, 256);
        QCOMPARE(quint8(inode[5]), 2u);
        const quint64 first = qFromBigEndian<quint64>(inode.constData() + 100);
        QByteArray changed = inode;
        qToBigEndian(first | (quint64(1) << 63), changed.data() + 100);
        {
            Patch patch(image(4), at, changed);
            QCOMPARE(open(4), S_OK);
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(extract(indexOf(filename), callback), S_OK);
            QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(callback->output->bytes, QByteArray(contents().size(), '\0'));
        }
        const quint64 logical = quint64(1) << 32;
        const quint64 position = logical * 4096;
        changed = inode;
        qToBigEndian(quint64(position + contents().size()), changed.data() + 56);
        qToBigEndian((first & 511) | (logical << 9), changed.data() + 100);
        Patch patch(image(4), at, changed);
        QCOMPARE(open(4), S_OK);
        CMyComPtr<IInArchiveGetStream> streams;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&streams)),
            S_OK);
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(streams->GetStream(indexOf(filename), &stream), S_OK);
        CMyComPtr<IInStream> seek;
        QCOMPARE(stream->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&seek)), S_OK);
        char buffer[32];
        UInt32 read = 0;
        QCOMPARE(stream->Read(buffer, sizeof(buffer), &read), S_OK);
        QCOMPARE(QByteArray(buffer, read), QByteArray(32, '\0'));
        QCOMPARE(seek->Seek(Int64(position), STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(stream->Read(buffer, sizeof(buffer), &read), S_OK);
        QCOMPARE(QByteArray(buffer, read), contents().left(32));
    }
    void btreeFileAndInvalidLeaf() {
        QCOMPARE(open(4), S_OK);
        const quint64 at = inodeOffset(image(4), fileInode());
        const QByteArray inode = readAt(image(4), at, 256);
        const unsigned count = qFromBigEndian<quint32>(inode.constData() + 76);
        QVERIFY(count > 0 && count * 16 <= unsigned(inode.size() - 100));
        const quint64 leafBlock = 10000;
        QByteArray root = inode, leaf(4096, '\0');
        root[5] = 3;
        const int forkSize = quint8(root[82]) ? quint8(root[82]) * 8 : 156;
        std::fill_n(root.data() + 100, forkSize, char(0));
        qToBigEndian<quint16>(2, root.data() + 100);
        qToBigEndian<quint16>(1, root.data() + 102);
        qToBigEndian<quint64>(leafBlock + 1, root.data() + 104 + ((forkSize - 4) / 16) * 8);
        memcpy(leaf.data(), "BMAP", 4);
        qToBigEndian<quint16>(quint16(count), leaf.data() + 6);
        memcpy(leaf.data() + 24, inode.constData() + 100, count * 16);
        QByteArray parent(4096, '\0');
        memcpy(parent.data(), "BMAP", 4);
        qToBigEndian<quint16>(1, parent.data() + 4);
        qToBigEndian<quint16>(1, parent.data() + 6);
        qToBigEndian<quint64>(leafBlock, parent.data() + 24 + ((4096 - 24) / 16) * 8);
        Patch inodePatch(image(4), at, root);
        Patch leafPatch(image(4), leafBlock * 4096, leaf);
        Patch parentPatch(image(4), (leafBlock + 1) * 4096, parent);
        QCOMPARE(open(4), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(extract(indexOf(filename), callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, contents());
        // A child that does not descend to the expected level cannot loop or
        // be accepted as file data, even when its magic and records are valid.
        QByteArray wrongLevel(2, '\0');
        qToBigEndian<quint16>(1, wrongLevel.data());
        Patch cycle(image(4), leafBlock * 4096 + 4, wrongLevel);
        callback->output->bytes.clear();
        QCOMPARE(extract(indexOf(filename), callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        QVERIFY(callback->output->bytes.isEmpty());
    }
    void unsafeNamesRejected() {
        const auto super = readAt(image(4), 0, 512);
        const quint64 root = qFromBigEndian<quint64>(super.constData() + 56);
        const quint64 offset = inodeOffset(image(4), root);
        const auto inode = readAt(image(4), offset, 256);
        QCOMPARE(quint8(inode[5]), 1u);
        const int headerSize = quint8(inode[101]) ? 10 : 6;
        Patch name(image(4), offset + 100 + headerSize + 3, QByteArray("/"));
        QCOMPARE(open(4), S_FALSE);
        UInt32 count = 1;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
    }
    void refusesExtentOutsideVolume() {
        QCOMPARE(open(4), S_OK);
        const quint64 at = inodeOffset(image(4), fileInode());
        const auto super = readAt(image(4), 0, 512);
        QByteArray inode = readAt(image(4), at, 256);
        const quint64 outside = quint64(qFromBigEndian<quint32>(super.constData() + 88))
                                << quint8(super[124]);
        const quint64 first = qFromBigEndian<quint64>(inode.constData() + 100);
        const quint64 second = qFromBigEndian<quint64>(inode.constData() + 108);
        qToBigEndian((first & ~quint64(511)) | (outside >> 43), inode.data() + 100);
        qToBigEndian((outside << 21) | (second & ((quint64(1) << 21) - 1)), inode.data() + 108);
        struct RestoreSize {
            QString path;
            qint64 size;
            ~RestoreSize() {
                if (!QFile::resize(path, size))
                    qFatal("fixture truncate failed");
            }
        } restore{image(4), QFileInfo(image(4)).size()};
        QFile source(image(4));
        QVERIFY(source.open(QIODevice::Append));
        // Bytes exist in the backing stream beyond the declared filesystem.
        // A reader that checks only stream bounds would leak adjacent data.
        const QByteArray adjacent(contents().size() + 4096, 'S');
        QCOMPARE(source.write(adjacent), adjacent.size());
        source.close();
        Patch extent(image(4), at, inode);
        QCOMPARE(open(4), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(extract(indexOf(filename), callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        QVERIFY(callback->output->bytes.isEmpty());
        archive_->Close();
        input_.Release();
    }
    void rejectsTruncatedInlineDirectory() {
        const auto super = readAt(image(4), 0, 512);
        const quint64 root = qFromBigEndian<quint64>(super.constData() + 56);
        const quint64 offset = inodeOffset(image(4), root);
        QByteArray size(8, '\0');
        qToBigEndian<quint64>(6, size.data());
        // The backing inode still contains all names; bytes past the logical
        // inline directory must not be accepted as live entries.
        Patch truncated(image(4), offset + 56, size);
        QCOMPARE(open(4), S_FALSE);
        UInt32 count = 1;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
    }
    void cleanupTestCase() {
        archive_.Release();
        input_.Release();
        module_.unload();
    }
};
QTEST_GUILESS_MAIN(XfsModuleTests)
#include "xfs_module_tests.moc"
