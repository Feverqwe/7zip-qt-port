#include "7zip/UI/Common/Property.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class LanguageSourceUpdateTests final : public QObject {
    Q_OBJECT
    QTemporaryDir settings;
    void language(bool russian) {
        const QString path = settings.filePath("language.ini");
        SaveRegLang(UString(russian ? "ru" : "-"), path);
        ReloadLang(path);
    }
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }

  private slots:
    void cleanup() { language(false); }
    void compressionAndExtractionRejections_data() {
        QTest::addColumn<bool>("russian");
        QTest::newRow("en") << false;
        QTest::newRow("ru") << true;
    }
    void compressionAndExtractionRejections() {
        QFETCH(bool, russian);
        language(russian);
        CompressionOptions options;
        options.EncryptionMethod = "AES256";
        CObjectVector<CProperty> properties;
        QString error;
        QVERIFY(!ArchiveBackend::compressionProperties("zip", QString(100, 'x'), false, options,
                                                       &properties, &error));
        QCOMPARE(error, QString::fromWCharArray(LangString(IDS_PASSWORD_TOO_LONG).Ptr()));
        QVERIFY(!ArchiveBackend::compressionProperties("zip", QStringLiteral("密碼"), false,
                                                       options, &properties, &error));
        QCOMPARE(error, QString::fromWCharArray(LangString(IDS_PASSWORD_USE_ASCII).Ptr()));
        options.Method = "invalid";
        QVERIFY(
            !ArchiveBackend::compressionProperties("zip", {}, false, options, &properties, &error));
        QCOMPARE(error,
                 russian ? QStringLiteral("Недопустимые параметры сжатия для выбранного формата.")
                         : QString("Invalid compression parameters for the selected format."));
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("input 密碼 file.txt"));
        const QString target = directory.filePath("output.zip");
        QVERIFY(write(source, "input bytes"));
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.extractSourceEntry(target, source, directory.filePath("extracted"), "zip", {});
        QCOMPARE(finished.size(), 1);
        const auto result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 russian ? QStringLiteral("Недопустимый файл или поток извлечения.")
                         : QString("Invalid extraction file or stream."));
        QCOMPARE(backend.lastResult(), E_FAIL);
        QVERIFY(!QFile::exists(target));
        QCOMPARE(read(source), QByteArray("input bytes"));
    }
};
QTEST_GUILESS_MAIN(LanguageSourceUpdateTests)
#include "language_source_update_tests.moc"
