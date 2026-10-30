// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the dynamic archive ABI against native Linux fscrypt data.
#include "7zip/Archive/IArchive.h"
#include "7zip/IPassword.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QtEndian>
#include <QtTest>
#include <vector>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    UInt64 failAt = UINT64_MAX;
    HRESULT failure = E_FAIL;
    unsigned faults = 0;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ <= failAt && failAt - position_ < size) {
            ++faults;
            return failure;
        }
        size = (UInt32)qMin<UInt64>(
            size, position_ < UInt64(bytes_.size()) ? bytes_.size() - position_ : 0);
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
class Keys final : public IArchiveOpenCallback,
                   public ICryptoGetTextPassword,
                   public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveOpenCallback, ICryptoGetTextPassword)
  public:
    QString text;
    HRESULT result = S_OK;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(CryptoGetTextPassword(BSTR *out)) override {
        *out = nullptr;
        if (result != S_OK)
            return result;
        const auto value = text.toStdWString();
        *out = SysAllocStringLen(value.data(), (UINT)value.size());
        return *out ? S_OK : E_OUTOFMEMORY;
    }
};
class Copy final : public IArchiveExtractCallback,
                   public ISequentialOutStream,
                   public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveExtractCallback, ISequentialOutStream)
    QByteArray current_;

  public:
    QVector<Int32> results;
    QVector<QByteArray> files;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32 mode)) override {
        current_.clear();
        *stream = mode == NArchive::NExtract::NAskMode::kExtract ? this : nullptr;
        if (*stream)
            (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        results.append(result);
        files.append(current_);
        return S_OK;
    }
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        current_.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
QByteArray load(const QString &name) {
    QFile file(QString::fromUtf8(SEVENZIP_FSCRYPT_FIXTURE) + '/' + name);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

class ExtFscryptTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_EXT_CORE)};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_;
    QJsonArray cases_;

    void create() {
        archive_.Release();
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto factory = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && factory);
        NWindows::NCOM::CPropVariant clsid;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
        QCOMPARE(clsid.vt, VARTYPE(VT_BSTR));
        GUID guid;
        memcpy(&guid, clsid.bstrVal, sizeof(guid));
        QCOMPARE(factory(&guid, &IID_IInArchive, (void **)&archive_), S_OK);
    }
    QString keyText() const {
        QStringList values;
        for (const auto &entry : cases_) {
            const auto c = entry.toObject();
            values << (c["key_type"].toInt() == 1 ? "v1:" + c["descriptor"].toString() + ":"
                                                  : "v2:") +
                          c["raw_key"].toString();
        }
        return "fscrypt:" + values.join(';');
    }
    UInt32 index(const QString &path) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UINT32_MAX;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant value;
            if (archive_->GetProperty(i, kpidPath, &value) == S_OK && value.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal)) == path)
                return i;
        }
        return UINT32_MAX;
    }
    CMyComPtr<IInStream> stream(UInt32 item) {
        CMyComPtr<IInArchiveGetStream> streams;
        archive_->QueryInterface(IID_IInArchiveGetStream, (void **)&streams);
        CMyComPtr<ISequentialInStream> sequential;
        if (!streams || streams->GetStream(item, &sequential) != S_OK)
            return {};
        CMyComPtr<IInStream> result;
        sequential->QueryInterface(IID_IInStream, (void **)&result);
        return result;
    }
    void readEquals(IInStream *input, const QByteArray &expected, UInt64 offset = 0) {
        QVERIFY(input);
        QCOMPARE(input->Seek((Int64)offset, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual(expected.size(), '\0');
        size_t pos = 0;
        while (pos < size_t(actual.size())) {
            UInt32 done = 0;
            QCOMPARE(input->Read(actual.data() + pos,
                                 (UInt32)qMin<size_t>(997, actual.size() - pos), &done),
                     S_OK);
            QVERIFY(done);
            pos += done;
        }
        QCOMPARE(actual, expected);
    }
  private slots:
    void initTestCase() {
        const auto manifest = QJsonDocument::fromJson(load("native.json")).object();
        cases_ = manifest["cases"].toArray();
        QVERIFY(!cases_.isEmpty());
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(manifest["image_size"].toDouble()), packed.data());
        packed += load("native.ext4.zlib");
        image_ = qUncompress(packed);
        QVERIFY(!image_.isEmpty());
        QCOMPARE(QCryptographicHash::hash(image_, QCryptographicHash::Sha256).toHex(),
                 manifest["image_sha256"].toString().toLatin1());
        create();
    }
    void nativePlaintextNamesAndRetainedStreams() {
        CMyComPtr2<IArchiveOpenCallback, Keys> keys;
        keys.Create_if_Empty();
        keys->text = keyText();
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, keys), S_OK);
        std::vector<CMyComPtr<IInStream>> retained;
        for (const auto &entry : cases_) {
            const auto c = entry.toObject();
            const QString directory = c["name"].toString() + '/';
            const UInt32 item = index(directory + c["filename"].toString());
            QVERIFY2(item != UINT32_MAX, qPrintable(directory));
            auto file = stream(item);
            QVERIFY(file);
            QByteArray plain = QByteArray::fromHex(c["plain_first"].toString().toLatin1()) +
                               QByteArray(4096, '\0') + "end";
            readEquals(file, plain);
            readEquals(file, QByteArray(7, '\0') + "end", 8185);
            retained.push_back(file);
            const UInt32 link = index(directory + "link");
            QVERIFY(link != UINT32_MAX);
            const QByteArray target = c["symlink_target"].toString().toUtf8();
            readEquals(stream(link), target);
            NWindows::NCOM::CPropVariant size;
            QCOMPARE(archive_->GetProperty(link, kpidSize, &size), S_OK);
            QCOMPARE(size.uhVal.QuadPart, UInt64(target.size()));
        }
        QCOMPARE(archive_->Close(), S_OK);
        archive_.Release();
        input.Release();
        keys->text.clear();
        for (size_t i = 0; i < retained.size(); ++i) {
            const auto first = QByteArray::fromHex(
                cases_[(qsizetype)i].toObject()["plain_first"].toString().toLatin1());
            readEquals(retained[i], first.first(32));
            readEquals(retained[i], "end", 8192);
        }
        create();
    }
    void missingKeysAndFailedOpenAreReusable() {
        CMyComPtr2<IArchiveOpenCallback, Keys> keys;
        keys.Create_if_Empty();
        CMyComPtr<IInStream> input = new Input(image_);
        keys->result = E_ABORT;
        QCOMPARE(archive_->Open(input, nullptr, keys), E_ABORT);
        keys->result = S_OK;
        keys->text = "ordinary password";
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, keys), E_INVALIDARG);
        keys->text.clear();
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, keys), S_OK);
        for (const auto &entry : cases_) {
            const auto c = entry.toObject();
            const UInt32 item = index(c["name"].toString() + '/' + c["nokey_name"].toString());
            QVERIFY(item != UINT32_MAX);
            QVERIFY(!stream(item));
        }
        keys->text = keyText();
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, keys), S_OK);
        const auto first = cases_.first().toObject();
        auto file = stream(index(first["name"].toString() + '/' + first["filename"].toString()));
        QVERIFY(file);
        readEquals(file, "end", 8192);
    }
    void inputFailureDoesNotPublishAndCanRetry() {
        CMyComPtr2<IArchiveOpenCallback, Keys> keys;
        keys.Create_if_Empty();
        keys->text = keyText();
        auto *source = new Input(image_);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, keys), S_OK);
        const auto first = cases_.first().toObject();
        auto file = stream(index(first["name"].toString() + '/' + first["filename"].toString()));
        QVERIFY(file);
        source->failAt = UInt64(first["extents"].toArray().first().toArray()[1].toDouble()) + 16;
        QByteArray output(32, '?');
        UInt32 done = 99;
        QCOMPARE(file->Read(output.data(), (UInt32)output.size(), &done), E_FAIL);
        QCOMPARE(done, UInt32(0));
        QCOMPARE(output, QByteArray(32, '?'));
        source->failAt = UINT64_MAX;
        readEquals(file, QByteArray::fromHex(first["plain_first"].toString().toLatin1()).first(32));
    }
    void extractionSourceFailureKeepsNextEncryptedFile() {
        CMyComPtr2<IArchiveOpenCallback, Keys> keys;
        keys.Create_if_Empty();
        keys->text = keyText();
        auto *source = new Input(image_);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, keys), S_OK);
        for (qsizetype i = 0; i < cases_.size(); ++i) {
            const auto damaged = cases_[i].toObject();
            const auto healthy = cases_[(i + 1) % cases_.size()].toObject();
            const UInt32 selected[] = {
                index(damaged["name"].toString() + '/' + damaged["filename"].toString()),
                index(healthy["name"].toString() + '/' + healthy["filename"].toString())};
            QVERIFY(selected[0] != UINT32_MAX && selected[1] != UINT32_MAX);
            source->failAt =
                UInt64(damaged["extents"].toArray().first().toArray()[1].toDouble()) + 16;
            for (HRESULT error : {E_FAIL, S_FALSE, S_OK, E_ABORT, E_OUTOFMEMORY}) {
                source->failure = error;
                for (bool testOnly : {false, true}) {
                    CMyComPtr2<IArchiveExtractCallback, Copy> callback;
                    callback.Create_if_Empty();
                    const unsigned faults = source->faults;
                    const bool fatal = error == E_ABORT || error == E_OUTOFMEMORY;
                    QCOMPARE(archive_->Extract(selected, 2, testOnly, callback),
                             fatal ? error : S_OK);
                    QVERIFY(source->faults > faults);
                    if (fatal) {
                        QVERIFY(callback->results.isEmpty());
                    } else {
                        QCOMPARE(callback->results,
                                 QVector<Int32>({NArchive::NExtract::NOperationResult::kDataError,
                                                 NArchive::NExtract::NOperationResult::kOK}));
                        QVERIFY(callback->files[0].isEmpty());
                        if (testOnly)
                            QVERIFY(callback->files[1].isEmpty());
                        else
                            QCOMPARE(QCryptographicHash::hash(callback->files[1],
                                                              QCryptographicHash::Sha256)
                                         .toHex(),
                                     healthy["native_sha256"].toString().toLatin1());
                    }
                }
            }
            source->failAt = UINT64_MAX;
            CMyComPtr2<IArchiveExtractCallback, Copy> retry;
            retry.Create_if_Empty();
            QCOMPARE(archive_->Extract(selected, 1, 0, retry), S_OK);
            QCOMPARE(retry->results, QVector<Int32>({NArchive::NExtract::NOperationResult::kOK}));
            QCOMPARE(QCryptographicHash::hash(retry->files[0], QCryptographicHash::Sha256).toHex(),
                     damaged["native_sha256"].toString().toLatin1());
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(ExtFscryptTests)
#include "ext_fscrypt_tests.moc"
