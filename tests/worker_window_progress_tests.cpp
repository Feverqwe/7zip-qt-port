#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "copydialog.h"
#include "window_state_support.h"
#include <QAbstractEventDispatcher>
#include <QCryptographicHash>
#include <QSemaphore>
#include <QTimerEvent>

namespace {
class MetaCallCounter final : public QObject {
  public:
    int calls = 0;

  protected:
    bool eventFilter(QObject *, QEvent *event) override {
        calls += event->type() == QEvent::MetaCall;
        return false;
    }
};
} // namespace

class WorkerWindowProgressTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static bool writeBlocks(const QString &path, int count) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        const QByteArray block(1 << 20, 'p');
        for (int i = 0; i < count; ++i)
            if (file.write(block) != block.size())
                return false;
        return file.flush();
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void nativeWorkerUpdatesVisibleProgressWithoutGuiQueue_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("destroyDuringCallback");
        QTest::newRow("copy") << 0 << false;
        QTest::newRow("split") << 1 << false;
        QTest::newRow("combine") << 2 << false;
        QTest::newRow("copy-close") << 0 << true;
        QTest::newRow("split-close") << 1 << true;
        QTest::newRow("combine-close") << 2 << true;
    }
    void nativeWorkerUpdatesVisibleProgressWithoutGuiQueue() {
        QFETCH(int, mode);
        QFETCH(bool, destroyDuringCallback);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("источник с пробелом.bin");
        const QString selected = name + (mode == 2 ? ".001" : "");
        QVERIFY(writeBlocks(fixture.filePath(selected), mode == 2 ? 32 : 64));
        if (mode == 2)
            QVERIFY(writeBlocks(fixture.filePath(name + ".002"), 32));
        const QString destination = fixture.filePath(QStringLiteral("папка назначения"));
        QVERIFY(QDir().mkpath(destination));
        ControlledBackend backend;
        auto owner = std::make_unique<MainWindow>(&backend);
        MainWindow &window = *owner;
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        const int row = filesystemRow(window, selected);
        QVERIFY(row >= 0);
        table(window)->selectRow(row);
        table(window)->setCurrentCell(row, 0);
        bool accepted = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>(mode == 1   ? "splitDialog"
                                                       : mode == 2 ? "combineDialog"
                                                                   : "copyDialog");
            if (!dialog)
                return;
            dialog->findChild<QComboBox *>(mode == 1 ? "volumeDestination" : "copyDestination")
                ->setCurrentText(destination);
            if (mode == 1)
                dialog->findChild<QComboBox *>("volumeSize")->setCurrentText("32M");
            accepted = true;
            dialog->accept();
        });
        action(window, mode == 1   ? "splitFileAction"
                       : mode == 2 ? "combineFilesAction"
                                   : "copyAction")
            ->trigger();
        QVERIFY(accepted);
        auto *worker = mode == 0
                           ? static_cast<QThread *>(window.findChild<FileTransferOperation *>())
                           : static_cast<QThread *>(window.findChild<VolumeOperation *>());
        QVERIFY(worker);
        auto *progress =
            dynamic_cast<CProgressDialog *>(window.findChild<QDialog *>("progressDialog"));
        QVERIFY(progress);
        if (destroyDuringCallback) {
            QSemaphore entered;
            if (auto *copy = qobject_cast<FileTransferOperation *>(worker))
                connect(
                    copy, &FileTransferOperation::advanced, copy,
                    [copy, &entered](qint64) {
                        entered.release();
                        while (!copy->isCanceled())
                            QThread::msleep(1);
                    },
                    Qt::DirectConnection);
            else {
                auto *volume = qobject_cast<VolumeOperation *>(worker);
                connect(
                    volume, &VolumeOperation::advanced, volume,
                    [volume, &entered](qint64) {
                        entered.release();
                        while (!volume->isCanceled())
                            QThread::msleep(1);
                    },
                    Qt::DirectConnection);
            }
            QVERIFY(entered.tryAcquire(1, 2000));
            const QPointer<QThread> retainedWorker(worker);
            const QPointer<QDialog> retainedProgress(progress);
            owner.reset();
            QVERIFY(!retainedWorker);
            QVERIFY(!retainedProgress);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
            QVERIFY(QFileInfo::exists(fixture.filePath(selected)));
            QVERIFY(QDir(destination)
                        .entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)
                        .isEmpty());
            return;
        }
        MetaCallCounter counter;
        window.installEventFilter(&counter);
        // Join the public worker without pumping GUI events. Its source progress
        // snapshot must already be usable by the dialog's real timer handler.
        QVERIFY(worker->wait(30000));
        bool timerDelivered = false;
        for (const auto &timer : QAbstractEventDispatcher::instance()->registeredTimers(progress)) {
            QTimerEvent event(timer.timerId);
            QCoreApplication::sendEvent(progress, &event);
            timerDelivered = true;
        }
        QVERIFY(timerDelivered);
        auto *processed = progress->findChild<QLabel *>("IDT_PROGRESS_PROCESSED_VAL");
        QVERIFY(processed);
        const QString visibleBytes = processed->text();
        QCoreApplication::sendPostedEvents(&window, QEvent::MetaCall);
        window.removeEventFilter(&counter);
        QCOMPARE(visibleBytes, QString("65536 KB"));
        QVERIFY2(counter.calls <= 4,
                 qPrintable(QString("%1 queued window callbacks").arg(counter.calls)));
        QTRY_VERIFY(action(window, "createFolderAction")->isEnabled());
        QStringList outputs;
        if (mode == 1)
            outputs = {QDir(destination).filePath(name + ".001"),
                       QDir(destination).filePath(name + ".002")};
        else
            outputs = {QDir(destination).filePath(name)};
        QCryptographicHash actual(QCryptographicHash::Sha256), expected(QCryptographicHash::Sha256);
        const QByteArray block(1 << 20, 'p');
        for (int i = 0; i < 64; ++i)
            expected.addData(block);
        qint64 total = 0;
        for (const QString &path : outputs) {
            QFile output(path);
            QVERIFY(output.open(QIODevice::ReadOnly));
            total += output.size();
            QVERIFY(actual.addData(&output));
        }
        QCOMPARE(total, qint64(64 << 20));
        QCOMPARE(actual.result(), expected.result());
        QVERIFY(QFileInfo::exists(fixture.filePath(selected)));
        QVERIFY(
            QDir(destination).entryList({"*.sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(WorkerWindowProgressTests)
#include "worker_window_progress_tests.moc"
