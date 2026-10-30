#pragma once
#include "archive/backend/archivebackend.h"
#include "archive/resources/qtarchivefoldermanager.h"
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QtTest>
#include <future>

inline HRESULT Worker(const std::function<HRESULT()> &action) {
    auto work = std::async(std::launch::async, action);
    while (work.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        QTest::qWait(1);
    return work.get();
}

class ArchiveReaderFixture : public QObject {
  protected:
    QString engine_;
    std::unique_ptr<ArchiveBackend> backend_;
    CMyComPtr<IFolderManager> manager_;
    bool Make(const QString &dir, const QStringList &args) {
        QProcess process;
        process.setWorkingDirectory(dir);
        process.start(engine_, args);
        return process.waitForFinished(30000) && process.exitCode() == 0;
    }
    static QByteArray Bytes(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static bool Put(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
    void InitializeRuntime() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native reading unverified");
#else
        const QString module = QStringLiteral(SEVENZIP_CODEC_MODULE);
        engine_ = QFileInfo(module).dir().filePath(
#ifdef Q_OS_WIN
            "7z.exe"
#else
            "7z"
#endif
        );
        backend_ = std::make_unique<ArchiveBackend>(nullptr, engine_);
        QSignalSpy ready(backend_.get(), &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend_->discoverCodecCatalog(module));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        manager_ = new QtArchiveFolderManager(*backend_);
#endif
    }
};
