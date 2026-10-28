#include "archivecodecimage.h"
#include "fixtures/scoped_environment.h"
#include "native_link_fixture.h"
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

class CodecImageTests final : public QObject {
    Q_OBJECT
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

  private slots:
    void snapshotRetainsBytesAndReleasesFilesWithItsOwner() {
        QTemporaryDir source, registry;
        QVERIFY(source.isValid() && registry.isValid());
        ScopedTestEnvironment environment("SEVENZIP_OWNED_TEMP_REGISTRY", registry.path());
        const QString module = source.filePath(QStringLiteral("кодек 空.module"));
        const QString asset = source.filePath("Formats/resources/asset.bin");
        QVERIFY(QDir().mkpath(QFileInfo(asset).absolutePath()));
        QVERIFY(write(module, "module before replacement"));
        QVERIFY(write(asset, "resource before replacement"));
        QString snapshot;
        {
            auto image = ArchiveCodecImage::Create(module);
            QVERIFY(image);
            snapshot = (*image)->baseFolder();
            QVERIFY(write(module, "new module"));
            QVERIFY(write(asset, "new resource"));
            QCOMPARE(read((*image)->module()), QByteArray("module before replacement"));
            const QString copiedAsset = QDir(snapshot).filePath("Formats/resources/asset.bin");
            QCOMPARE(read(copiedAsset), QByteArray("resource before replacement"));
            QCOMPARE((*image)->originalPath(copiedAsset), asset);
            const auto entries = QtOwnedTempRegistry().Entries();
            QCOMPARE(entries.size(), 1);
            QVERIFY(entries.first().active);
        }
        QVERIFY(!QFileInfo::exists(snapshot));
        QVERIFY(QtOwnedTempRegistry().Entries().isEmpty());
        QCOMPARE(read(module), QByteArray("new module"));
        QCOMPARE(read(asset), QByteArray("new resource"));
    }
    void failedCopyReleasesPartialSnapshot() {
        QTemporaryDir source, registry;
        QVERIFY(source.isValid() && registry.isValid());
        ScopedTestEnvironment environment("SEVENZIP_OWNED_TEMP_REGISTRY", registry.path());
        const QString module = source.filePath("runtime.module");
        const QString formats = source.filePath("Formats");
        QVERIFY(write(module, "unchanged module"));
        QVERIFY(QDir().mkpath(formats));
        const QString cycle = QDir(formats).filePath("cycle");
        QVERIFY(CreateNativeSymbolicLink(cycle, formats));
        const auto unlink = qScopeGuard([&] { QFile::remove(cycle); });
        const auto failed = ArchiveCodecImage::Create(module);
        QVERIFY(!failed);
        QCOMPARE(failed.error(), ArchiveCodecImage::Error::RuntimeCopyFailed);
        QVERIFY(QtOwnedTempRegistry().Entries().isEmpty());
        QCOMPARE(read(module), QByteArray("unchanged module"));
    }
};

QTEST_GUILESS_MAIN(CodecImageTests)
#include "codec_image_tests.moc"
