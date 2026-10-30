// SPDX-License-Identifier: MIT
#include "archive/backend/archivebackend.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <algorithm>

class ExtFscryptBackendTests final : public QObject {
    Q_OBJECT
  private slots:
    void keysListingAndOwnedExtraction() {
        const QDir fixtures(QString::fromUtf8(SEVENZIP_FSCRYPT_FIXTURE));
        QFile manifest(fixtures.filePath("native.json"));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        const auto record = QJsonDocument::fromJson(manifest.readAll()).object();
        QFile compressed(fixtures.filePath("native.ext4.zlib"));
        QVERIFY(compressed.open(QIODevice::ReadOnly));
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record["image_size"].toDouble()), packed.data());
        packed += compressed.readAll();
        const auto bytes = qUncompress(packed);
        QVERIFY(!bytes.isEmpty());
        QStringList keys;
        for (const auto &entry : record["cases"].toArray()) {
            const auto c = entry.toObject();
            keys << (c["key_type"].toInt() == 1 ? "v1:" + c["descriptor"].toString() + ":"
                                                : "v2:") +
                        c["raw_key"].toString();
        }
        const QString password = "fscrypt:" + keys.join(';');
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
        const QString path = runtime.filePath(QString::fromUtf8("образ fscrypt 空.ext4"));
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
        backend.list(path, password);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        const QString saved = runtime.filePath("selected.bin");
        for (const auto &entry : record["cases"].toArray()) {
            const auto c = entry.toObject();
            const QString member = c["name"].toString() + '/' + c["filename"].toString();
            QStringList observed;
            for (const auto &e : entries)
                observed << e.path;
            QVERIFY2(std::any_of(entries.cbegin(), entries.cend(),
                                 [&](const ArchiveEntry &e) {
                                     return QDir::fromNativeSeparators(e.path) == member;
                                 }),
                     qPrintable(QString("Missing %1; paths: %2").arg(member, observed.join('\n'))));
            finished.clear();
            QFile::remove(saved);
            backend.extractSourceEntry(path, member, saved, "Ext", password);
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            QFile output(saved);
            QVERIFY(output.open(QIODevice::ReadOnly));
            QCOMPARE(output.readAll(), QByteArray::fromHex(c["plain_first"].toString().toLatin1()) +
                                           QByteArray(4096, '\0') + "end");
        }
        finished.clear();
        backend.list(path, "fscrypt:none");
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        const auto first = record["cases"].toArray().first().toObject();
        const QString opaque = first["name"].toString() + '/' + first["nokey_name"].toString();
        QVERIFY(std::any_of(entries.cbegin(), entries.cend(), [&](const ArchiveEntry &e) {
            return QDir::fromNativeSeparators(e.path) == opaque;
        }));
        QFile output(saved);
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(output.write("keep"), qint64(4));
        output.close();
        finished.clear();
        backend.extractSourceEntry(path, opaque, saved, "Ext", "fscrypt:none");
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY(!finished[0][0].toBool());
        QVERIFY(output.open(QIODevice::ReadOnly));
        QCOMPARE(output.readAll(), QByteArray("keep"));
        QVERIFY(image.open(QIODevice::ReadOnly));
        QCOMPARE(image.readAll(), bytes);
    }
};
QTEST_GUILESS_MAIN(ExtFscryptBackendTests)
#include "ext_fscrypt_backend_tests.moc"
