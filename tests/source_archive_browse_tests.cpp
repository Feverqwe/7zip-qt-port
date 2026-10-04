#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "archivecodeccatalog.h"
#include <QTemporaryDir>
#include <QtTest>

class SourceArchiveBrowseTests final : public QObject {
    Q_OBJECT
    std::shared_ptr<ArchiveCodecCatalog> catalog_;
    QTemporaryDir directory_;

    CCompressDialog model() const {
        CCompressDialog dialog;
        dialog.ArcFormats = &catalog_->nativeCodecs().Formats;
        for (const QString &format : {"7z", "zip", "gzip"})
            dialog.ArcIndices.Add(unsigned(catalog_->formatIndex(format)));
        dialog.Info.FormatIndex = catalog_->formatIndex("7z");
        dialog.DirPrefix = directory_.path();
        dialog.m_ArchivePath = QStringLiteral("данные 空.7z");
        return dialog;
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official codec module required for source archive browse");
#else
        catalog_ =
            std::make_shared<ArchiveCodecCatalog>(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(catalog_->result(), S_OK);
        QVERIFY(directory_.isValid());
#endif
    }
    void confirmedFilterControlsExtension_data() {
        QTest::addColumn<int>("filter");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("expectedName");
        QTest::addColumn<QString>("expectedFormat");
        QTest::newRow("zip-filter") << 1 << "output" << "output.zip" << "zip";
        QTest::newRow("zip-existing-extension") << 1 << "output.ZIP" << "output.ZIP" << "zip";
        QTest::newRow("zip-alias") << 1 << "output.docx" << "output.docx" << "zip";
        QTest::newRow("zip-trailing-dot") << 1 << "output." << "output.zip" << "zip";
        QTest::newRow("gzip-main-extension") << 2 << "output" << "output.gz" << "gzip";
        QTest::newRow("all-files-infers-zip") << 4 << "output.zip" << "output.zip" << "zip";
        QTest::newRow("all-files-unknown") << 4 << "output.dat" << "output.dat" << "7z";
    }
    void confirmedFilterControlsExtension() {
        QFETCH(int, filter);
        QFETCH(QString, name);
        QFETCH(QString, expectedName);
        QFETCH(QString, expectedFormat);
        auto dialog = model();
        bool shown = false;
        int initialFilter = -1;
        QString initialPath, zipDescription;
        unsigned count = 0;
        QStringList zipMasks;
        QVERIFY(dialog.OnButtonSetArchive(nullptr, [&](CBrowseInfo &info, const auto &filters) {
            shown = true;
            initialFilter = info.FilterIndex;
            initialPath = info.FilePath;
            count = filters.Size();
            zipDescription = QString::fromWCharArray(filters[1].Description.Ptr());
            FOR_VECTOR(i, filters[1].Masks)
            zipMasks.append(QString::fromWCharArray(filters[1].Masks[i].Ptr()));
            info.FilterIndex = filter;
            info.FilePath = directory_.filePath(name);
            return true;
        }));
        QVERIFY(shown);
        QCOMPARE(initialFilter, 0);
        QCOMPARE(initialPath, directory_.filePath(QStringLiteral("данные 空.7z")));
        QCOMPARE(count, 5u);
        QCOMPARE(zipDescription, QString("zip (zip z01 zipx jar epub ipa apk appx)"));
        for (const QString &mask : {"*.docx", "*.xlsx", "*.odt", "*.xpi"})
            QVERIFY(!zipMasks.contains(mask));
        QCOMPARE(dialog.Format, expectedFormat);
        QCOMPARE(dialog.m_ArchivePath, expectedName);
        QCOMPARE(QDir::fromNativeSeparators(dialog.DirPrefix), directory_.path() + '/');
    }
    void cancelPreservesPathAndFormat() {
        auto dialog = model();
        QVERIFY(!dialog.OnButtonSetArchive(nullptr, [](CBrowseInfo &info, const auto &) {
            info.FilterIndex = 1;
            info.FilePath = "changed.zip";
            return false;
        }));
        QCOMPARE(dialog.Format, QString("7z"));
        QCOMPARE(dialog.m_ArchivePath, QStringLiteral("данные 空.7z"));
        QCOMPARE(dialog.Info.FormatIndex, catalog_->formatIndex("7z"));
    }
    void sfxReplacesFinalExtension() {
        auto dialog = model();
        dialog.Info.SFXMode = true;
        unsigned count = 0;
        QVERIFY(dialog.OnButtonSetArchive(nullptr, [&](CBrowseInfo &info, const auto &filters) {
            count = filters.Size();
            info.FilePath = directory_.filePath("name.tar.zip");
            return true;
        }));
        QCOMPARE(count, 2u);
#ifdef _WIN32
        QCOMPARE(dialog.m_ArchivePath, QString("name.tar.exe"));
#else
        QCOMPARE(dialog.m_ArchivePath, QString("name.tar"));
#endif
        QCOMPARE(dialog.Format, QString("7z"));
    }
};
QTEST_MAIN(SourceArchiveBrowseTests)
#include "source_archive_browse_tests.moc"
