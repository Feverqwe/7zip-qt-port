#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "Common/StringConvert.h"
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class CNativeOpenUi final : public IOpenCallbackUI {
  public:
    bool Cancel = false;
    unsigned Finished = 0;
    unsigned BreakChecks = 0;
    HRESULT Open_CheckBreak() override {
        ++BreakChecks;
        return Cancel ? E_ABORT : S_OK;
    }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_Finished() override {
        ++Finished;
        return S_OK;
    }
    HRESULT Open_CryptoGetTextPassword(BSTR *) override { return E_ABORT; }
};
} // namespace

class NativeArchiveOpeningTests final : public QObject {
    Q_OBJECT
    QString engine_, module_;
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; source native opening remains unverified");
#else
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        engine_ = QStandardPaths::findExecutable("7zz");
        if (engine_.isEmpty())
            QSKIP("Official engine unavailable; source native opening remains unverified");
#endif
    }
    void fullSourceOpensActualHandlersAndParser_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("openType");
        QTest::addColumn<QString>("suffix");
        QTest::newRow("native ZIP unknown extension") << "zip" << "*" << ".unknown";
        QTest::newRow("native 7z explicit type") << "7z" << "7z" << ".unknown";
        QTest::newRow("native gzip AddExt") << "gzip" << "gzip" << ".tgz";
        QTest::newRow("native parser concatenated ZIP") << "zip" << "#" << ".unknown";
        QTest::newRow("native wrong forced handler") << "zip" << "7z" << ".unknown";
    }
    void fullSourceOpensActualHandlersAndParser() {
        QFETCH(QString, format);
        QFETCH(QString, openType);
        QFETCH(QString, suffix);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile source(fixture.filePath(QStringLiteral("источник 空.bin")));
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("native source opening"), qint64(21));
        source.close();
        const QString path = fixture.filePath(QStringLiteral("архив 空 with spaces") + suffix);
        QProcess creator;
        creator.start(engine_, {"a", "-t" + format, "--", path, source.fileName()});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        QFile archive(path);
        QVERIFY(archive.open(QIODevice::ReadOnly));
        const QByteArray original = archive.readAll();
        archive.close();
        if (openType == "#") {
            QVERIFY(archive.open(QIODevice::Append));
            QCOMPARE(archive.write(original), qint64(original.size()));
            archive.close();
        }
        auto *codecs = new CCodecs;
        CMyComPtr<ICompressCodecsInfo> lease = codecs;
        bool loaded = false;
        QCOMPARE(codecs->LoadDll(us2fs(UString(module_.toStdWString().c_str())), false, &loaded),
                 S_OK);
        QVERIFY(loaded);
        CObjectVector<COpenType> types;
        QVERIFY(ParseOpenTypes(*codecs, UString(openType.toStdWString().c_str()), types));
        COpenOptions options;
        CIntVector excludedFormats;
        options.codecs = codecs;
        options.types = &types;
        options.excludedFormats = &excludedFormats;
        options.props = nullptr;
        options.filePath = path.toStdWString().c_str();
        CNativeOpenUi callback;
        CArchiveLink link;
        const bool wrongType = format == "zip" && openType == "7z";
        QCOMPARE(link.Open3(options, &callback), wrongType ? S_FALSE : S_OK);
        QCOMPARE(callback.Finished, 1u);
        if (wrongType) {
            QVERIFY(link.Arcs.IsEmpty());
            return;
        }
        QVERIFY(!link.Arcs.IsEmpty());
        const CArc &arc = *link.GetArc();
        QVERIFY(arc.Archive);
        QCOMPARE(arc.IsParseArc, openType == "#");
        if (openType != "#") {
            QVERIFY(arc.FormatIndex >= 0);
            QCOMPARE(QString::fromWCharArray(codecs->Formats[unsigned(arc.FormatIndex)].Name.Ptr())
                         .toLower(),
                     format);
            if (format == "gzip")
                QCOMPARE(QString::fromWCharArray(arc.DefaultName.Ptr()),
                         QStringLiteral("архив 空 with spaces.tar"));
        }
        UInt32 count = 0;
        QCOMPARE(arc.Archive->GetNumberOfItems(&count), S_OK);
        QVERIFY(count >= 1);
        QCOMPARE(link.Close(), S_OK);
        link.Release();
        callback.Cancel = true;
        callback.BreakChecks = 0;
        COpenOptions cancelledOptions;
        cancelledOptions.codecs = codecs;
        cancelledOptions.types = &types;
        cancelledOptions.excludedFormats = &excludedFormats;
        cancelledOptions.props = nullptr;
        cancelledOptions.filePath = path.toStdWString().c_str();
        CArchiveLink cancelledLink;
        QCOMPARE(cancelledLink.Open3(cancelledOptions, &callback), E_ABORT);
        QVERIFY(callback.BreakChecks > 0);
        QCOMPARE(callback.Finished, 2u);
        cancelledLink.Release();
        QVERIFY(archive.open(QIODevice::ReadOnly));
        QCOMPARE(archive.readAll(), openType == "#" ? original + original : original);
    }
};
QTEST_GUILESS_MAIN(NativeArchiveOpeningTests)
#include "native_archive_opening_tests.moc"
