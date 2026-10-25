#include "archivereadonlyview.h"
#include "device_test_support.h"
#include "exfat_fixture.h"
#include "native_archive_fixture.h"
#include "qtarchivefileresource.h"
#include "qtarchivemountsession.h"
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QtTest>
#include <future>

namespace {
template <typename Work> auto Worker(Work work) {
    auto pending = std::async(std::launch::async, std::move(work));
    while (pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        QTest::qWait(1);
    return pending.get();
}
} // namespace

class ArchiveMountDeviceTests final : public QObject {
    Q_OBJECT
  private slots:
    void retainedDeviceRead_data() {
        QTest::addColumn<bool>("limited");
        QTest::addColumn<bool>("disconnect");
        QTest::newRow("retained read-only device") << false << false;
        QTest::newRow("device removed before first read") << false << true;
        QTest::newRow("explicit dirty-volume opening") << true << false;
    }
    void retainedDeviceRead() {
        QFETCH(bool, limited);
        QFETCH(bool, disconnect);
        ExfatFixture fixture;
        if (limited)
            fixture.image[106] = char(2); // Native exFAT dirty-volume flag.
        auto *raw = new CDeviceInput(fixture.image);
        CMyComPtr<IInStream> rawLease = raw, input;
        QCOMPARE(DeviceInput(raw, fixture.image.size(), &input, 512), S_OK);
        if (limited) {
            CMyComPtr<IInStream> consented;
            QCOMPARE(CreateDeviceInputAllowingIncompleteRead(input, &consented), S_OK);
            input = consented;
        }
        auto backend = std::make_unique<ArchiveBackend>();
        QCOMPARE(NativeArchiveFixture::Attach(*backend), S_OK);
        auto *resource = new QtArchiveFileResource("Synthetic read-only exFAT device");
        CMyComPtr<IUnknown> retained = resource;
        // The deployment catalog retains the external filesystem module;
        // NativeArchiveFixture::Attach's core-only backend is sufficient for
        // ordinary ZIP/TAR tests, but does not discover filesystem plugins.
        resource->AttachCodecCatalog(NativeArchiveFixture::Catalog());
        QString error;
        QCOMPARE(
            Worker([&] { return resource->OpenDeviceInput(*backend, input, nullptr, &error); }),
            S_OK);
        resource->UseNativeMetadata();
        std::shared_ptr<ArchiveReadOnlyView> view;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(
                         resource, 4096, {}, view, error, [&](const QString &path, quint64 bytes) {
                             return path == fixture.fragmentedName &&
                                    bytes == quint64(fixture.sectorSize + 188);
                         });
                 }),
                 S_OK);
        // The public mount warning must retain the selected device opening
        // mode independently of the original device/window services.
        if (limited)
            QVERIFY(!view->limitations().isEmpty());
        quint64 id = 0;
        for (const auto &entry : view->entries())
            if (entry.Name == fixture.fragmentedName)
                id = entry.Id;
        QVERIFY(id);
        retained.Release();
        backend.reset();
        rawLease.Release();
        input.Release();
        raw->Disconnected = disconnect;
        auto bytes = Worker([&] { return view->read(id, 500, 200); });
        if (disconnect) {
            QVERIFY(bytes.Result != S_OK);
            QVERIFY(bytes.Data.isEmpty());
            raw->Disconnected = false;
            bytes = Worker([&] { return view->read(id, 500, 200); });
        }
        QCOMPARE(bytes.Result, S_OK);
        QCOMPARE(bytes.Data, QByteArray(12, 'A') + QByteArray(188, 'B'));
    }
#ifdef Q_OS_MACOS
    void nativeDeviceRemovalAndRetry() {
        if (qEnvironmentVariableIntValue("SEVENZIP_NATIVE_SMB_TEST") != 1)
            QSKIP("Opt-in native mount of an isolated synthetic device.");
        ExfatFixture fixture;
        auto *raw = new CDeviceInput(fixture.image);
        CMyComPtr<IInStream> lease = raw, input;
        QCOMPARE(DeviceInput(raw, fixture.image.size(), &input, 512), S_OK);
        ArchiveBackend backend;
        QCOMPARE(NativeArchiveFixture::Attach(backend), S_OK);
        auto *resource = new QtArchiveFileResource("Synthetic removable exFAT device");
        CMyComPtr<IUnknown> retained = resource;
        resource->AttachCodecCatalog(NativeArchiveFixture::Catalog());
        QString error;
        QCOMPARE(Worker([&] { return resource->OpenDeviceInput(backend, input, nullptr, &error); }),
                 S_OK);
        resource->UseNativeMetadata();
        raw->Disconnected = true;
        QtArchiveMountSession session("Synthetic removable device");
        QSignalSpy issues(&session, &QtArchiveMountSession::entryIssue);
        connect(&session, &QtArchiveMountSession::extractionRequired, this,
                [](const QString &, quint64, const ArchiveMountDecisionPtr &decision) {
                    decision->answer(true);
                });
        const auto cleanup = qScopeGuard([&] {
            session.unmount(true);
            for (int attempt = 0;
                 attempt < 300 && session.status() != QtArchiveMountSession::Status::Stopped;
                 ++attempt)
                QTest::qWait(100);
        });
        session.start(resource, 4096, {}, QStringLiteral(SEVENZIP_SMB_SERVER));
        retained.Release();
        input.Release();
        QTRY_VERIFY_WITH_TIMEOUT(session.status() == QtArchiveMountSession::Status::Mounted ||
                                     session.status() == QtArchiveMountSession::Status::Stopped,
                                 30000);
        QCOMPARE(session.status(), QtArchiveMountSession::Status::Mounted);
        const auto path = QDir(session.path()).filePath(fixture.fragmentedName);
        const auto read = [&] {
            return Worker([&] {
                QFile file(path);
                return file.open(QIODevice::ReadOnly) && file.seek(500) ? file.read(200)
                                                                        : QByteArray();
            });
        };
        QVERIFY(read().isEmpty());
        QTRY_VERIFY(!issues.isEmpty());
        QVERIFY(issues.front()[1].toUInt() != quint32(S_OK));
        raw->Disconnected = false;
        QCOMPARE(read(), QByteArray(12, 'A') + QByteArray(188, 'B'));
        session.unmount();
        QTRY_COMPARE_WITH_TIMEOUT(session.status(), QtArchiveMountSession::Status::Stopped, 15000);
    }
#endif
};
QTEST_GUILESS_MAIN(ArchiveMountDeviceTests)
#include "archive_mount_device_tests.moc"
