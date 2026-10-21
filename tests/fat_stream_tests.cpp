#include "Windows/PropVariant.h"
#include "fat_test_support.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QtEndian>
#include <QtTest>

using namespace FatTestSupport;

class FatStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_FAT_MODULE)};
    CMyComPtr<IInArchive> archive_;
    CMyComPtr2<IInStream, Input> source_;
    QJsonObject fixture_;
    void open(int bits) {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_FAT_PYTHON),
                        {QString::fromUtf8(SEVENZIP_FAT_FIXTURE),
                         "--streams=" + QString::number(bits), "--bundle"});
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        fixture_ = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(fixture_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(fixture_["image"].toString().toLatin1());
        const auto bytes = qUncompress(packed);
        QVERIFY(!bytes.isEmpty());
        source_.SetFromCls(new Input(bytes));
        QCOMPARE(archive_->Open(source_, nullptr, nullptr), S_OK);
    }
    QByteArray expected(const QString &name) {
        return QByteArray::fromBase64(fixture_["files"].toObject()[name].toString().toLatin1());
    }
    void stream(const QString &name, CMyComPtr<IInStream> &result) {
        UInt32 count = 0, index = UINT32_MAX;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            QCOMPARE(archive_->GetProperty(i, kpidPath, &path), S_OK);
            if (path.vt == VT_BSTR && QString::fromWCharArray(path.bstrVal) == name)
                index = i;
        }
        QVERIFY2(index != UINT32_MAX, qPrintable(name));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(index, &sequential), S_OK);
        QVERIFY(sequential);
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result)),
                 S_OK);
    }
    void check(IInStream *stream, const QByteArray &expected) {
        QByteArray actual(expected.size(), '\0');
        UInt32 total = 0;
        while (total < UInt32(actual.size())) {
            UInt32 done = 0;
            const UInt32 remaining = UInt32(actual.size()) - total;
            QCOMPARE(stream->Read(actual.data() + total, remaining, &done), S_OK);
            QVERIFY(done > 0 && done <= remaining);
            total += done;
        }
        QCOMPARE(actual, expected);
    }
  private slots:
    void initTestCase() {
        QVERIFY(module_.load());
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && create);
        NWindows::NCOM::CPropVariant clsid;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
        GUID guid;
        memcpy(&guid, clsid.bstrVal, sizeof(guid));
        QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
    }
    void interleavedFragments_data() {
        QTest::addColumn<int>("bits");
        for (int bits : {12, 16, 32})
            QTest::newRow(qPrintable(QString::number(bits))) << bits;
    }
    void interleavedFragments() {
        QFETCH(int, bits);
        open(bits);
        const QString main = fixture_["name"].toString();
        CMyComPtr<IInStream> streams[2];
        stream(main, streams[0]);
        stream("SECOND.BIN", streams[1]);
        QVERIFY(streams[0] && streams[1]);
        const QByteArray data[] = {expected(main), expected("SECOND.BIN")};
        int positions[] = {0, 0};
        QCOMPARE(archive_->Close(), S_OK);
        source_.SetFromCls(nullptr);
        while (positions[0] < data[0].size() || positions[1] < data[1].size()) {
            for (int i = 0; i < 2; ++i) {
                if (positions[i] >= data[i].size())
                    continue;
                QByteArray bytes(37 + i * 12, '\0');
                UInt32 done = 0;
                QCOMPARE(streams[i]->Read(bytes.data(), UInt32(bytes.size()), &done), S_OK);
                QVERIFY(done > 0 && done <= UInt32(bytes.size()));
                bytes.truncate(int(done));
                QCOMPARE(bytes, data[i].mid(positions[i], int(done)));
                positions[i] += int(done);
            }
        }
        for (int i = 0; i < 2; ++i) {
            char byte;
            UInt32 done = 99;
            QCOMPARE(streams[i]->Read(&byte, 1, &done), S_OK);
            QCOMPARE(done, UInt32(0));
            QCOMPARE(streams[i]->Seek(510, STREAM_SEEK_SET, nullptr), S_OK);
            check(streams[i], data[i].mid(510, 27));
        }
    }
    void failedSeeksAndReadsRetryAtSameByte() {
        open(12);
        const QString main = fixture_["name"].toString();
        const auto data = expected(main), second = expected("SECOND.BIN");
        const auto offsets = fixture_["stream_offsets"].toArray();
        for (HRESULT failure : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
            CMyComPtr<IInStream> a, b;
            stream(main, a);
            stream("SECOND.BIN", b);
            QVERIFY(a && b);
            source_->failSeek = true;
            source_->failAt = UInt64(offsets[1].toInteger());
            source_->failure = failure;
            QCOMPARE(a->Seek(512, STREAM_SEEK_SET, nullptr), S_OK);
            char bytes[19];
            UInt32 done = 99;
            QCOMPARE(a->Read(bytes, sizeof(bytes), &done), failure);
            QCOMPARE(done, UInt32(0));
            UInt64 position = 0;
            QCOMPARE(a->Seek(0, STREAM_SEEK_CUR, &position), S_OK);
            QCOMPARE(position, UInt64(512));
            source_->failAt = UINT64_MAX;
            check(b, second.left(17));
            check(a, data.mid(512, 19));
        }
        for (HRESULT failure : {S_OK, E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
            source_->failSeek = false;
            CMyComPtr<IInStream> a;
            stream(main, a);
            QVERIFY(a);
            check(a, data.left(37));
            source_->failAt = UInt64(offsets[0].toInteger()) + 37;
            source_->failure = failure;
            char bytes[19];
            UInt32 done = 99;
            QCOMPARE(a->Read(bytes, sizeof(bytes), &done), failure);
            QCOMPARE(done, UInt32(0));
            source_->failAt = UINT64_MAX;
            CMyComPtr<IInStream> b;
            stream("SECOND.BIN", b);
            QVERIFY(b);
            check(b, second.left(17));
            check(a, data.mid(37, 19));
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(FatStreamTests)
#include "fat_stream_tests.moc"
