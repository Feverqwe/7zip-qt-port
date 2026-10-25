#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivenativeopen.h"
#include "archivereadonlyview.h"
#include "qtarchivefileresource.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <algorithm>
#include <future>

class ExtXattrBackendTests final : public QObject {
    Q_OBJECT
  private slots:
    void selectedMetadataValues() {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_EXT_PYTHON),
                        {QString::fromUtf8(SEVENZIP_EXT_FIXTURE), "xattrs", "--bundle"});
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        const auto record = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record["image"].toString().toLatin1());
        const QByteArray bytes = qUncompress(packed);
        QVERIFY(!bytes.isEmpty());
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
        const QDir prepared(QString::fromUtf8(SEVENZIP_EXT_RUNTIME));
#ifdef Q_OS_WIN
        const QString executable = "7z.exe", core = "7z.dll";
#else
        const QString executable = "7z", core = "7z.so";
#endif
        QVERIFY(QFile::copy(prepared.filePath(executable), runtime.filePath(executable)));
        QVERIFY(QFile::copy(prepared.filePath(core), runtime.filePath(core)));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        const QString module = QString::fromUtf8(SEVENZIP_EXT_CORE);
        QVERIFY(QFile::copy(module, runtime.filePath("Formats/" + QFileInfo(module).fileName())));
        const QString path = runtime.filePath(QString::fromUtf8("образ xattr 空.ext4"));
        QFile image(path);
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(bytes), bytes.size());
        image.close();
        ArchiveBackend backend(nullptr, runtime.filePath(executable));
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(runtime.filePath(core)));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QVector<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, &backend,
                [&](const QVector<ArchiveEntry> &result) { entries = result; });
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString mountError;
        bool cacheRequired = true;
        QByteArray mountedBytes;
        auto probe = std::async(std::launch::async, [&]() -> HRESULT {
            std::shared_ptr<ArchiveNativeOpen> reader;
            RINOK(ArchiveBackend::openNativeArchive(backend.codecCatalog(), path, "Ext", nullptr,
                                                    reader, nullptr))
            auto *owner = new QtArchiveFileResource(path);
            resource = owner;
            owner->AttachNativeOpen(reader);
            owner->UseNativeMetadata();
            RINOK(ArchiveBackend::createReadOnlyView(resource, 0, {}, view, mountError))
            RINOK(view->needsCache(cacheRequired))
            for (const auto &entry : view->entries())
                if (!entry.Directory && entry.Name == "data.bin") {
                    const auto read = view->read(entry.Id, 0, 1024);
                    mountedBytes = read.Data;
                    return read.Result;
                }
            return E_FAIL;
        });
        while (probe.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            QTest::qWait(1);
        QCOMPARE(probe.get(), S_OK);
        QVERIFY(!cacheRequired);
        QCOMPARE(mountedBytes,
                 QByteArray::fromHex(record["files"].toObject()["data.bin"].toString().toLatin1()));
        for (const QString &entry : {QString("data.bin:$XATTR.user.large"),
                                     QString("data.bin:$XATTR.system.posix_acl_access"),
                                     QString("nested:$XATTR.system.posix_acl_default"),
                                     QString("[ROOT]:$XATTR.user.root")}) {
            QVERIFY(std::any_of(entries.cbegin(), entries.cend(), [&](const ArchiveEntry &value) {
                return value.path == entry && value.properties.value("Alternate Stream") == "+" &&
                       !value.directory;
            }));
            finished.clear();
            const QString saved = runtime.filePath("selected.bin");
            QFile::remove(saved);
            backend.extractSourceEntry(path, entry, saved, "Ext", {}, true);
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            QFile output(saved);
            QVERIFY(output.open(QIODevice::ReadOnly));
            QCOMPARE(output.readAll(),
                     QByteArray::fromHex(record["files"].toObject()[entry].toString().toLatin1()));
        }
        QVERIFY(image.open(QIODevice::ReadOnly));
        QCOMPARE(image.readAll(), bytes);
    }
};
QTEST_GUILESS_MAIN(ExtXattrBackendTests)
#include "ext_xattr_backend_tests.moc"
