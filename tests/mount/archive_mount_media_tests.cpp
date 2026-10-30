#include "archive/agent/Agent.h"
#include "archive/resources/archivenativeopen.h"
#include "archive/resources/archivereadstream.h"
#include "archive/resources/qtarchivefileresource.h"
#include "archive/resources/qtarchivefoldermanager.h"
#include "archive/support/native_archive_fixture.h"
#include "mount/qtarchivemountsession.h"
#include "support/native_media_consumer.h"
#include <QTemporaryDir>
#include <QtTest>
#include <future>

namespace {
template <typename Work> auto Worker(Work work) {
    auto pending = std::async(std::launch::async, std::move(work));
    while (pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        QTest::qWait(1);
    return pending.get();
}
HRESULT NestedResource(const QString &outer, CMyComPtr<IUnknown> &resource) {
    ArchiveBackend backend;
    RINOK(NativeArchiveFixture::Attach(backend))
    CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
    CMyComPtr<IFolderFolder> parent;
    const auto path = outer.toStdWString();
    RINOK(Worker(
        [&] { return manager->OpenFolderFile(nullptr, path.c_str(), L"tar", &parent, nullptr); }))
    CMyComPtr<IArchiveGetStreamWithRequest> source;
    RINOK(parent->QueryInterface(IID_IArchiveGetStreamWithRequest, (void **)&source))
    CMyComPtr<ISequentialInStream> sequential;
    RINOK(Worker([&] { return source->GetStreamWithRequest(0, &sequential, nullptr); }))
    if (!sequential)
        return E_NOINTERFACE;
    CMyComPtr<IInStream> input;
    RINOK(sequential.QueryInterface(IID_IInStream, &input))
    CMyComPtr<IFolderFolder> child;
    RINOK(Worker(
        [&] { return manager->OpenFolderFile(input, L"inner.tar", L"tar", &child, nullptr); }))
    auto *agent = dynamic_cast<CAgentFolder *>(child.Interface());
    if (!agent || !agent->GetEngineResource())
        return E_NOINTERFACE;
    resource = agent->GetEngineResource();
    // Both folder/window services disappear before the mount starts.
    return S_OK;
}
} // namespace

class ArchiveMountMediaTests final : public QObject {
    Q_OBJECT
  private slots:
    void nativePdfVideo_data() {
        QTest::addColumn<bool>("nested");
        QTest::newRow("root TAR") << false;
        QTest::newRow("nested TAR after folders close") << true;
    }
    void nativePdfVideo() {
        QFETCH(bool, nested);
        const QString inputs = qEnvironmentVariable("SEVENZIP_NATIVE_MEDIA_FIXTURES");
        if (inputs.isEmpty())
            QSKIP("Opt-in PDFKit/AVPlayer native mount; requires generated synthetic media "
                  "fixtures.");
        QList<NativeArchiveFixture::Item> items;
        for (const auto &name :
             {QStringLiteral("sample.pdf"), QStringLiteral("video with spaces.mp4")}) {
            QFile file(QDir(inputs).filePath(name));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QVERIFY(file.size() > 0 && file.size() < 10 * 1024 * 1024);
            items.append({name, file.readAll()});
        }
        QTemporaryDir fixture;
        const auto path = fixture.filePath("native-media.tar");
        QVERIFY(NativeArchiveFixture::Write(path, items, "tar"));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(ArchiveBackend::openNativeArchive(NativeArchiveFixture::Catalog(), path, "tar",
                                                   nullptr, reader, nullptr),
                 S_OK);
        CMyComPtr<IUnknown> retained;
        if (nested) {
            QFile inner(path);
            QVERIFY(inner.open(QIODevice::ReadOnly));
            const auto outer = fixture.filePath("outer.tar");
            QVERIFY(NativeArchiveFixture::Write(outer, {{"inner.tar", inner.readAll()}}, "tar"));
            QCOMPARE(NestedResource(outer, retained), S_OK);
        } else {
            auto *resource = new QtArchiveFileResource(path);
            retained = resource;
            resource->AttachNativeOpen(reader);
            resource->UseNativeMetadata();
        }
        QtArchiveMountSession session("Synthetic native PDF and video");
        int extractions = 0;
        connect(&session, &QtArchiveMountSession::extractionRequired, this,
                [&](const QString &, quint64, const ArchiveMountDecisionPtr &decision) {
                    ++extractions;
                    decision->answer(false);
                });
        const auto deployed = qEnvironmentVariable("SEVENZIP_TEST_MOUNT_SERVER");
        session.start(retained, 16 * 1024 * 1024, {},
                      deployed.isEmpty() ? QStringLiteral(SEVENZIP_SMB_SERVER) : deployed);
        retained.Release();
        reader.reset();
        QTRY_VERIFY_WITH_TIMEOUT(session.status() == QtArchiveMountSession::Status::Mounted ||
                                     session.status() == QtArchiveMountSession::Status::Stopped,
                                 30000);
        QVERIFY2(session.status() == QtArchiveMountSession::Status::Mounted,
                 qPrintable(session.error()));
        QString error;
        const QString mounted = session.path();
        auto consuming =
            std::async(std::launch::async, [&] { return ReadNativeMedia(mounted, error); });
        while (consuming.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            QTest::qWait(5);
        const bool read = consuming.get();
        session.unmount();
        QTRY_VERIFY_WITH_TIMEOUT(session.status() == QtArchiveMountSession::Status::Stopped ||
                                     session.status() == QtArchiveMountSession::Status::Mounted,
                                 15000);
        // AVFoundation releases file handles asynchronously after playback.
        // Keep the server available during that interval and retry normal
        // unmount, just as the manager permits after a busy result.
        for (int attempt = 0;
             attempt < 10 && session.status() == QtArchiveMountSession::Status::Mounted;
             ++attempt) {
            QTest::qWait(500);
            session.unmount();
            QTRY_VERIFY_WITH_TIMEOUT(session.status() == QtArchiveMountSession::Status::Stopped ||
                                         session.status() == QtArchiveMountSession::Status::Mounted,
                                     15000);
        }
        if (session.status() != QtArchiveMountSession::Status::Stopped) {
            session.unmount(true);
            QTRY_COMPARE_WITH_TIMEOUT(session.status(), QtArchiveMountSession::Status::Stopped,
                                      15000);
            QFAIL(
                "Media consumer retained the native mount; force-detached the synthetic fixture.");
        }
        QCOMPARE(extractions, 0);
        QVERIFY2(read, qPrintable(error));
    }
};
QTEST_MAIN(ArchiveMountMediaTests)
#include "archive_mount_media_tests.moc"
