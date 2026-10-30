#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "Common/StringConvert.h"
#include "archive/codecs/archivecodeccatalog.h"
#include "archive/resources/qtarchivebatchextract.h"
#include "archive/resources/qtextractpathpolicy.h"
#include "dialogs/overwrite/OverwriteDialog.h"
#include "fileops/native/FileDir.h"
#include "platform/support/native_link_fixture.h"
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
FString native(const QString &path) { return us2fs(path.toStdWString().c_str()); }
bool makeLink(const QString &path, const QString &target) {
    return CreateNativeSymbolicLink(path, target);
}
class CExpectedErrorDismissal {
    QTimer timer_;

  public:
    CExpectedErrorDismissal() {
        QObject::connect(&timer_, &QTimer::timeout, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
                if (widget->objectName() == "progressDialog" && widget->isVisible()) {
                    auto *close = widget->findChild<QPushButton *>("IDCANCEL");
                    if (close && close->property("languageID").toUInt() == IDS_CLOSE)
                        close->click();
                }
            }
        });
        timer_.start(1);
    }
};
} // namespace

class SourceLinkExtractTests final : public QObject {
    Q_OBJECT
    bool createTar(const QString &directory, const QString &archive, const QStringList &names) {
        QProcess process;
        process.setWorkingDirectory(directory);
        process.start(QStandardPaths::findExecutable("7z"),
                      QStringList{"a", "-ttar", "-snl", "-snh", "--", archive} + names);
        return process.waitForFinished(10000) && process.exitCode() == 0;
    }
    bool loadNative(ArchiveBackend &backend) {
#ifdef SEVENZIP_CODEC_MODULE
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        if (!backend.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE)))
            return false;
        if (ready.isEmpty() && !ready.wait(10000))
            return false;
        return ready[0][0].toBool();
#else
        Q_UNUSED(backend)
        return false;
#endif
    }

  private slots:
    void nativeWimReparseLinks_data() {
        QTest::addColumn<bool>("absolute");
        QTest::newRow("relative") << false;
        QTest::newRow("absolute-rebased") << true;
    }
    void nativeWimReparseLinks() {
#if defined(Q_OS_WIN) && defined(SEVENZIP_CODEC_MODULE)
        QFETCH(bool, absolute);
        QTemporaryDir fixture;
        const QString rootName = QStringLiteral("корень δ with space");
        const QString root = fixture.filePath(rootName);
        QVERIFY(QDir().mkpath(root + "/sub"));
        QVERIFY(writeFile(root + "/payload.bin", "payload"));
        QVERIFY(writeFile(root + "/sub/inside.txt", "nested"));
        QVERIFY(makeLink(root + "/file link", absolute ? root + "/payload.bin" : "payload.bin"));
        QVERIFY(makeLink(root + "/directory link", absolute ? root + "/sub" : "sub"));
        const QString archive = fixture.filePath("native reparse.wim");
        QProcess creator;
        creator.setWorkingDirectory(fixture.path());
        creator.start(QStandardPaths::findExecutable("7z"),
                      {"a", "-twim", "-snl", "--", archive, rootName});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        const QByteArray original = readFile(archive);
        ArchiveBackend backend;
        QVERIFY(loadNative(backend));
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::OverwriteExisting;
        const QString destination = fixture.filePath("output");
        CExpectedErrorDismissal dismissal;
        const auto result =
            RunQtArchiveBatchExtract(backend, {archive}, destination, {}, options, nullptr);
        QCOMPARE(result.Result, S_OK);
        QVERIFY(result.CallbackOK);
        const QString output = QDir(destination).filePath(rootName);
        QVERIFY(QFileInfo(output + "/file link").isSymLink());
        QVERIFY(QFileInfo(output + "/directory link").isSymLink());
        QCOMPARE(QDir::cleanPath(QFileInfo(output + "/file link").symLinkTarget()),
                 output + "/payload.bin");
        QCOMPARE(QDir::cleanPath(QFileInfo(output + "/directory link").symLinkTarget()),
                 output + "/sub");
        QCOMPARE(readFile(output + "/file link"), QByteArray("payload"));
        QCOMPARE(readFile(output + "/directory link/inside.txt"), QByteArray("nested"));
        QCOMPARE(readFile(archive), original);
        QCOMPARE(readFile(root + "/payload.bin"), QByteArray("payload"));
#else
        QSKIP("Native Windows reparse representation requires Windows and the official module.");
#endif
    }
    void nativeSafeLinksAndRootElimination_data() {
        QTest::addColumn<bool>("elimDup");
        QTest::newRow("keep-root") << false;
        QTest::newRow("eliminate-root") << true;
    }
    void nativeSafeLinksAndRootElimination() {
#if !defined(SEVENZIP_CODEC_MODULE)
        QSKIP("Official module required.");
#else
        QFETCH(bool, elimDup);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("корень с пробелом");
        QVERIFY(QDir().mkpath(root + "/sub"));
        QVERIFY(writeFile(root + "/payload.bin", "payload"));
        QVERIFY(writeFile(root + "/sub/inside.txt", "nested"));
        QVERIFY(makeLink(root + "/file link", "payload.bin"));
        QVERIFY(makeLink(root + "/directory link", "sub"));
        QVERIFY(makeLink(root + "/dangling link", "missing"));
        QVERIFY(makeLink(root + "/cycle link", "cycle link"));
        QVERIFY(NWindows::NFile::NDir::MyCreateHardLink(native(root + "/hard alias.bin"),
                                                        native(root + "/payload.bin")));
        const QString archive = fixture.filePath("links.tar");
        QVERIFY(createTar(fixture.path(), archive, {QFileInfo(root).fileName()}));
        ArchiveBackend backend;
        QVERIFY(loadNative(backend));
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::OverwriteExisting;
        options.ElimDup.Def = true;
        options.ElimDup.Val = elimDup;
        const QString output = fixture.filePath("output/" + QFileInfo(root).fileName());
        const auto result =
            RunQtArchiveBatchExtract(backend, {archive}, output, {}, options, nullptr);
        QCOMPARE(result.Result, S_OK);
        QVERIFY(result.CallbackOK);
        const QString restored = elimDup ? output : output + "/" + QFileInfo(root).fileName();
        QCOMPARE(readFile(restored + "/payload.bin"), QByteArray("payload"));
        QCOMPARE(readFile(restored + "/directory link/inside.txt"), QByteArray("nested"));
        for (const QString &name : {QString("file link"), QString("directory link"),
                                    QString("dangling link"), QString("cycle link")})
            QVERIFY2(QFileInfo(restored + "/" + name).isSymLink(), qPrintable(name));
        QVERIFY(NativeFilesShareIdentity(restored + "/payload.bin", restored + "/hard alias.bin"));
        QCOMPARE(readFile(root + "/payload.bin"), QByteArray("payload"));
#endif
    }
    void refusesDangerousOrOccupiedLinkTargets_data() {
        QTest::addColumn<bool>("occupiedAncestor");
        QTest::addColumn<bool>("rebasedAbsolute");
        QTest::newRow("parent-traversal") << false << false;
        QTest::newRow("link-over-link") << true << false;
        QTest::newRow("absolute-rebased-with-eliminated-root") << false << true;
    }
    void refusesDangerousOrOccupiedLinkTargets() {
#if !defined(SEVENZIP_CODEC_MODULE)
        QSKIP("Official module required.");
#else
        QFETCH(bool, occupiedAncestor);
        QFETCH(bool, rebasedAbsolute);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("input");
        const QString output = fixture.filePath(rebasedAbsolute ? "output/input" : "output");
        const QString outside = fixture.filePath("outside");
        QVERIFY(QDir().mkpath(source) && QDir().mkpath(output) && QDir().mkpath(outside));
        QVERIFY(writeFile(outside + "/sentinel.txt", "preserve"));
        const QString absoluteTarget =
#ifdef Q_OS_WIN
            QDir::rootPath() + "outside/sentinel.txt";
#else
            "/outside/sentinel.txt";
#endif
        const QString target = rebasedAbsolute    ? absoluteTarget
                               : occupiedAncestor ? QString("occupied/sentinel.txt")
                                                  : QString("../outside/sentinel.txt");
        QVERIFY(makeLink(source + "/link", target));
        const QString archive = fixture.filePath("unsafe.tar");
        QVERIFY(createTar(rebasedAbsolute ? fixture.path() : source, archive,
                          {rebasedAbsolute ? QString("input") : QString("link")}));
        if (occupiedAncestor)
            QVERIFY(makeLink(output + "/occupied", outside));
        const QByteArray original = readFile(archive);
        ArchiveBackend backend;
        QVERIFY(loadNative(backend));
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::OverwriteExisting;
        if (rebasedAbsolute)
            options.ElimDup.SetTrueTrue();
        CExpectedErrorDismissal dismissal;
        const auto result =
            RunQtArchiveBatchExtract(backend, {archive}, output, {}, options, nullptr);
        // TAR symbolic targets use the original Unix-link representation.
        // Windows drive targets are refused by that source branch; WIM raw
        // reparse targets have a separate native representation.
#ifdef Q_OS_WIN
        const bool rebasedInside = false;
#else
        const bool rebasedInside = rebasedAbsolute;
#endif
        if (rebasedInside) {
            QCOMPARE(result.Result, S_OK);
            QVERIFY(result.CallbackOK);
            const QFileInfo link(output + "/link");
            QVERIFY(link.isSymLink());
            QCOMPARE(QDir::cleanPath(link.symLinkTarget()), output + "/outside/sentinel.txt");
        } else {
            QVERIFY(result.Result != S_OK || !result.CallbackOK);
            QVERIFY(!QFileInfo(output + "/link").isSymLink());
        }
        QCOMPARE(readFile(outside + "/sentinel.txt"), QByteArray("preserve"));
        QCOMPARE(readFile(archive), original);
        QVERIFY(!backend.busy());
#endif
    }
    void boundsFinalLinkTargets() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString output = fixture.filePath("output");
        QVERIFY(QDir().mkpath(output));
        CQtExtractPathPolicy policy(output);
        UString message;
        QCOMPARE(policy.CheckLink(native(output + "/link"), native(fixture.filePath("outside")),
                                  false, message),
                 E_INVALIDARG);
        QVERIFY(!message.IsEmpty());
        QCOMPARE(policy.CheckLink(native(output + "/link"), native("../outside"), true, message),
                 E_INVALIDARG);
        QCOMPARE(policy.CheckLink(native(output + "/link"), native("missing"), true, message),
                 S_OK);
        QVERIFY(!QFileInfo::exists(output + "/link"));
    }
    void cancellationKeepsPartialWritesAndExistingFile() {
#if !defined(SEVENZIP_CODEC_MODULE)
        QSKIP("Official module required.");
#else
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("input");
        const QString output = fixture.filePath("output");
        QVERIFY(QDir().mkpath(source) && QDir().mkpath(output));
        QVERIFY(writeFile(source + "/a completed.bin", "completed"));
        QVERIFY(writeFile(source + "/z existing.bin", "replacement"));
        QVERIFY(makeLink(source + "/zz deferred link", "a completed.bin"));
        const QString archive = fixture.filePath("cancel.tar");
        QVERIFY(
            createTar(source, archive, {"a completed.bin", "z existing.bin", "zz deferred link"}));
        const QByteArray original = readFile(archive);
        QVERIFY(writeFile(output + "/z existing.bin", "preserved"));
        ArchiveBackend backend;
        QVERIFY(loadNative(backend));
        bool answered = false;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &backend, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    answered = true;
                    dialog->findChild<QPushButton *>("overwriteCancel")->click();
                }
        });
        answer.start(1);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        const auto result =
            RunQtArchiveBatchExtract(backend, {archive}, output, {}, options, nullptr);
        QCOMPARE(result.Result, E_ABORT);
        QVERIFY(answered);
        QCOMPARE(readFile(output + "/a completed.bin"), QByteArray("completed"));
        QCOMPARE(readFile(output + "/z existing.bin"), QByteArray("preserved"));
        QVERIFY(!QFileInfo(output + "/zz deferred link").isSymLink());
        QCOMPARE(readFile(source + "/a completed.bin"), QByteArray("completed"));
        QCOMPARE(readFile(source + "/z existing.bin"), QByteArray("replacement"));
        QCOMPARE(readFile(archive), original);
        QVERIFY(!backend.busy());
#endif
    }
};
QTEST_MAIN(SourceLinkExtractTests)
#include "source_link_extract_tests.moc"
