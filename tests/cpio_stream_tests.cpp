#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLibrary>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    UInt64 failAt = UINT64_MAX;
    HRESULT failure = E_FAIL;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ == failAt)
            return failure;
        size = UInt32(qMin<UInt64>(
            size, position_ < UInt64(bytes_.size()) ? UInt64(bytes_.size()) - position_ : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : bytes_.size();
        if (offset < -base || (offset > 0 && base > INT64_MAX - offset))
            return E_INVALIDARG;
        position_ = UInt64(base + offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
class Cancel final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
QString runtimeFile(const char *name) {
    return QDir(QString::fromUtf8(SEVENZIP_CPIO_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class CpioStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_;
    QMap<QString, QByteArray> files_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.cpio");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_CPIO_PYTHON),
                      {QString::fromUtf8(SEVENZIP_CPIO_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const auto files =
            QJsonDocument::fromJson(process.readAllStandardOutput()).object()["files"].toObject();
        files_.clear();
        for (auto it = files.begin(); it != files.end(); ++it)
            files_[it.key()] = QByteArray::fromHex(it.value().toString().toLatin1());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        image_ = file.readAll();
    }
    CMyComPtr<IInStream> stream(UInt32 index) {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        CMyComPtr<ISequentialInStream> sequential;
        if (provider->GetStream(index, &sequential) != S_OK || !sequential)
            return {};
        CMyComPtr<IInStream> result;
        sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
        return result;
    }
    void check(IInStream *data, const QByteArray &expected) {
        QVERIFY(data);
        QCOMPARE(data->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray result;
        for (;;) {
            char bytes[7];
            UInt32 processed = 99;
            QCOMPARE(data->Read(bytes, sizeof(bytes), &processed), S_OK);
            if (!processed)
                break;
            result.append(bytes, processed);
            QVERIFY(result.size() <= expected.size());
        }
        QCOMPARE(result, expected);
    }

  private slots:
    void init() {
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto count =
            reinterpret_cast<Func_GetNumberOfFormats>(module_.resolve("GetNumberOfFormats"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && count && create);
        UInt32 formats = 0;
        QCOMPARE(count(&formats), S_OK);
        for (UInt32 i = 0; i < formats; ++i) {
            NWindows::NCOM::CPropVariant name, id;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "Cpio")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no CPIO handler");
    }
    void cleanup() { archive_.Release(); }
    void streams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"newc", "name-nuls", "no-final-padding", "leading-zeros",
                                 "member-chain", "links-first", "links-overwrite", "inode-single",
                                 "inode-types", "trailer-reset", "no-reset", "binary-be", "odc"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void streams() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QMap<QString, UInt32> indices;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(archive_->GetProperty(i, kpidPath, &name), S_OK);
            QCOMPARE(name.vt, VARTYPE(VT_BSTR));
            indices[QString::fromWCharArray(name.bstrVal)] = i;
        }
        for (auto it = files_.begin(); it != files_.end(); ++it) {
            QVERIFY(indices.contains(it.key()));
            const UInt32 index = indices[it.key()];
            NWindows::NCOM::CPropVariant size;
            QCOMPARE(archive_->GetProperty(index, kpidSize, &size), S_OK);
            QCOMPARE(size.vt, VARTYPE(VT_UI8));
            QCOMPARE(size.uhVal.QuadPart, UInt64(it.value().size()));
            auto first = stream(index), second = stream(index);
            QVERIFY(first && second);
            char byte = 0;
            UInt32 processed = 0;
            if (it.value().size() > 1) {
                QCOMPARE(first->Read(&byte, 1, &processed), S_OK);
                QCOMPARE(processed, UInt32(1));
                QCOMPARE(byte, it.value()[0]);
                check(second, it.value());
                QCOMPARE(first->Read(&byte, 1, &processed), S_OK);
                QCOMPARE(processed, UInt32(1));
                QCOMPARE(byte, it.value()[1]);
            }
            check(first, it.value());
        }
        const auto expected = files_.first();
        auto retained = stream(indices[files_.firstKey()]);
        QCOMPARE(archive_->Close(), S_OK);
        fixture("crc");
        CMyComPtr<IInStream> replacement = new Input(image_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        check(retained, expected);
    }
    void failuresAndCancellation() {
        fixture("member-chain");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image_));
        input->failAt = UInt64(image_.indexOf("070701", 6));
        UInt32 count = 99;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            input->failure = result;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        input->failAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel = new Cancel;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto data = stream(0);
        QVERIFY(data);
        input->failAt = image_.indexOf('A', 112);
        char byte;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            input->failure = result;
            UInt32 processed = 99;
            QCOMPARE(data->Read(&byte, 1, &processed), result);
            QCOMPARE(processed, UInt32(0));
        }
        fixture("name-garbage");
        CMyComPtr<IInStream> invalid = new Input(image_);
        QCOMPARE(archive_->Open(invalid, nullptr, nullptr), S_FALSE);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream(0), "A");
    }
    void backendExtraction() {
        fixture("name-nuls");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QString::fromUtf8("архив 空.cpio"));
        QFile image(path);
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(image_), image_.size());
        image.close();
        ArchiveBackend backend(nullptr, runtimeFile(
#ifdef Q_OS_WIN
                                            "7z.exe"
#else
                                            "7z"
#endif
                                            ));
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core()));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 10000);
        QVERIFY(ready[0][0].toBool());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString saved = temporary.filePath("payload.bin");
        backend.extractSourceEntry(path, files_.firstKey(), saved, "Cpio", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), files_.first());
    }
};
QTEST_GUILESS_MAIN(CpioStreamTests)
#include "cpio_stream_tests.moc"
