#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "deviceselectiondialog.h"
#include "layout_inspection.h"
#include "mainwindow.h"
#include "qtlanguageservices.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHeaderView>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>
#include <cstring>

namespace {
class CDeviceBytes final : public DeviceReadHandle {
  public:
    QByteArray Bytes;
    explicit CDeviceBytes(QByteArray bytes) : Bytes(std::move(bytes)) {}
    qint32 readAt(quint64 offset, void *buffer, quint32 size, quint32 &read) override {
        read = 0;
        if (offset > quint64(Bytes.size()) || size > quint64(Bytes.size()) - offset)
            return E_FAIL;
        memcpy(buffer, Bytes.constData() + offset, size);
        read = size;
        return S_OK;
    }
};
ExternalDevice Device(const QByteArray &bytes) {
    return {"private-device",
            QStringLiteral("USB диск 空"),
            "fixture",
            quint64(bytes.size()),
            4096,
            false};
}
DeviceSelectionServices Services(const std::shared_ptr<CDeviceBytes> &bytes) {
    auto device = Device(bytes->Bytes);
    return {[device](QString &) {
                auto busy = device;
                busy.id = "busy-fixture";
                busy.mounted = true;
                return QList<ExternalDevice>{busy, device};
            },
            [bytes](const ExternalDevice &, const auto &) {
                return DeviceAccessResult{DeviceAccessStatus::Ready, {}, bytes};
            }};
}
QTreeWidget *Items(DeviceSelectionDialog &dialog) {
    return dialog.findChild<QTreeWidget *>("deviceItems");
}
QPushButton *Open(DeviceSelectionDialog &dialog) {
    return dialog.findChild<QPushButton *>("deviceOpenButton");
}
int Row(QTableWidget &table, const QString &name) {
    for (int row = 0; row < table.rowCount(); ++row)
        if (table.item(row, 0) && table.item(row, 0)->text() == name)
            return row;
    return -1;
}
} // namespace

class DeviceUiTests final : public QObject {
    Q_OBJECT
    QString engine_, module_;
    QByteArray fixture_;
    QTemporaryDir settings_;
    QByteArray previousSettings_;
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
  private slots:
    void initTestCase() {
        previousSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        QVERIFY(settings_.isValid());
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.filePath("settings.ini").toUtf8());
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; device UI integration unverified");
#else
        module_ = QStringLiteral(SEVENZIP_CODEC_MODULE);
        engine_ = QFileInfo(module_).dir().filePath(
#ifdef Q_OS_WIN
            "7z.exe"
#else
            "7z"
#endif
        );
        ArchiveCodecProbe probe;
        bool done = false;
        QVERIFY(probe.Start(engine_, module_, [&](auto result) {
            catalog_ = result.catalog;
            done = true;
        }));
        QTRY_VERIFY(done);
        QVERIFY(catalog_);
        QCOMPARE(catalog_->result(), S_OK);
        QProcess gzip;
        gzip.start(engine_, {"x", "-so", "--", QStringLiteral(DEVICE_FIXTURE_DIR "/gpt.img.gz")});
        QVERIFY(gzip.waitForFinished(10000));
        QCOMPARE(gzip.exitCode(), 0);
        fixture_ = gzip.readAllStandardOutput();
        QVERIFY(!fixture_.isEmpty());
#endif
    }
    void cleanupTestCase() {
        if (previousSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", previousSettings_);
    }
    void cleanup() {
        QtLoadTranslation("-");
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
    void geometryAndPartitionSelection_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &language : {"en", "ru"}) {
            QTest::newRow(qPrintable(language + "-normal")) << language << QSize(620, 390);
            QTest::newRow(qPrintable(language + "-compact")) << language << QSize(420, 320);
        }
    }
    void geometryAndPartitionSelection() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QtLoadTranslation(language == "en" ? "-" : language);
        auto bytes = std::make_shared<CDeviceBytes>(fixture_);
        auto services = Services(bytes);
        const auto open = services.open;
        const QString denied = QCoreApplication::translate(
            "deviceaccess", "macOS denied device access after administrator authorization. "
                            "Enable Full Disk Access for Sevenzip in System Settings > "
                            "Privacy & Security, then restart Sevenzip and try again.");
        std::atomic<unsigned> attempts{0};
        services.open = [&](const ExternalDevice &device, const auto &cancelled) {
            if (!attempts++)
                return DeviceAccessResult{DeviceAccessStatus::Failed, denied, {}};
            return open(device, cancelled);
        };
        DeviceSelectionDialog dialog(catalog_, nullptr, services);
        dialog.resize(size);
        dialog.show();
        QTRY_VERIFY(Items(dialog)->isEnabled());
        QCOMPARE(Items(dialog)->topLevelItemCount(), 2);
        QVERIFY(Items(dialog)->topLevelItem(0)->isDisabled());
        QVERIFY(!Open(dialog)->isEnabled());
        Items(dialog)->setCurrentItem(Items(dialog)->topLevelItem(1));
        QVERIFY(Open(dialog)->isEnabled());
        auto inspect = [&](const QString &state) {
            dialog.layout()->activate();
            QCoreApplication::processEvents();
            auto *buttons = dialog.findChild<QDialogButtonBox *>();
            QVERIFY(UiInspection::fullyVisible(*Items(dialog), dialog));
            QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
            QVERIFY(UiInspection::separate(*Items(dialog), *buttons, dialog));
            for (auto *button : buttons->findChildren<QPushButton *>())
                if (button->isVisible()) {
                    QVERIFY(UiInspection::fullyVisible(*button, dialog));
                    QVERIFY(button->width() >= button->sizeHint().width());
                }
            for (auto *label : dialog.findChildren<QLabel *>())
                if (label->isVisible()) {
                    QVERIFY(UiInspection::fullyVisible(*label, dialog));
                    QVERIFY2(UiInspection::labelFits(*label), qPrintable(label->text()));
                    QVERIFY(UiInspection::separate(*label, *Items(dialog), dialog));
                    QVERIFY(UiInspection::separate(*label, *buttons, dialog));
                }
            dialog.setProperty("uiInspection_language", language);
            dialog.setProperty("uiInspection_state", state);
            QVERIFY(UiInspection::writeReport(
                dialog, "devices-" + language + '-' + QString::number(size.width()) + '-' + state));
        };
        inspect("devices");
        Open(dialog)->click();
        QTRY_VERIFY(Open(dialog)->isEnabled());
        QCOMPARE(dialog.findChild<QLabel *>("deviceMessage")->text(), denied);
        QVERIFY(!dialog.selectedStream());
        inspect("denied");
        Open(dialog)->click();
        auto *back = dialog.findChild<QPushButton *>("deviceBackButton");
        QTRY_VERIFY(back->isVisible() && back->isEnabled());
        QCOMPARE(Items(dialog)->topLevelItemCount(), 2);
        inspect("partitions");
        back->click();
        QVERIFY(!back->isVisible());
        Items(dialog)->setCurrentItem(Items(dialog)->topLevelItem(1));
        Open(dialog)->click();
        QTRY_VERIFY(back->isVisible() && back->isEnabled());
        Items(dialog)->setCurrentItem(Items(dialog)->topLevelItem(0));
        QSignalSpy accepted(&dialog, &QDialog::accepted);
        QTest::keyClick(Items(dialog), Qt::Key_Return);
        QCOMPARE(accepted.size(), 1);
        QVERIFY(dynamic_cast<DirectDeviceInput *>(dialog.selectedStream()));
    }
    void emptyListRefreshAndDeniedAuthorizationAllowRetry() {
        auto services = Services(std::make_shared<CDeviceBytes>(fixture_));
        auto enumerate = services.enumerate;
        auto open = services.open;
        std::atomic<unsigned> enumerations{0}, attempts{0};
        services.enumerate = [&](QString &error) {
            return enumerations++ ? enumerate(error) : QList<ExternalDevice>{};
        };
        services.open = [&](const ExternalDevice &device, const auto &cancelled) {
            if (!attempts++)
                return DeviceAccessResult{DeviceAccessStatus::PermissionRequired, {}, {}};
            return open(device, cancelled);
        };
        DeviceSelectionDialog dialog(catalog_, nullptr, services);
        dialog.show();
        QTRY_VERIFY(Items(dialog)->isEnabled());
        QCOMPARE(Items(dialog)->topLevelItemCount(), 0);
        QVERIFY(!Open(dialog)->isEnabled());
        dialog.findChild<QPushButton *>("deviceRefreshButton")->click();
        QTRY_VERIFY(Items(dialog)->isEnabled());
        QCOMPARE(Items(dialog)->topLevelItemCount(), 2);
        Items(dialog)->setCurrentItem(Items(dialog)->topLevelItem(1));
        Open(dialog)->click();
        QTRY_VERIFY(Open(dialog)->isEnabled());
        QVERIFY(!dialog.selectedStream());
        QVERIFY(!dialog.findChild<QLabel *>("deviceMessage")->text().isEmpty());
        Open(dialog)->click();
        QTRY_VERIFY(dialog.findChild<QPushButton *>("deviceBackButton")->isVisible());
        dialog.reject();
        QVERIFY(!dialog.selectedStream());
    }
    void backReleasesSourceBeforeAnotherExclusiveOpen() {
        auto services = Services(std::make_shared<CDeviceBytes>(fixture_));
        std::weak_ptr<DeviceReadHandle> active;
        services.open = [&](const ExternalDevice &, const auto &) {
            if (!active.expired())
                return DeviceAccessResult{DeviceAccessStatus::Busy, {}, {}};
            auto handle = std::make_shared<CDeviceBytes>(fixture_);
            active = handle;
            return DeviceAccessResult{DeviceAccessStatus::Ready, {}, handle};
        };
        DeviceSelectionDialog dialog(catalog_, nullptr, services);
        dialog.show();
        QTRY_VERIFY(Items(dialog)->isEnabled());
        auto *back = dialog.findChild<QPushButton *>("deviceBackButton");
        for (unsigned attempt = 0; attempt < 2; ++attempt) {
            Items(dialog)->setCurrentItem(Items(dialog)->topLevelItem(1));
            Open(dialog)->click();
            QTRY_VERIFY(back->isVisible() && back->isEnabled());
            QVERIFY(!active.expired());
            back->click();
            QVERIFY(active.expired());
        }
        dialog.reject();
    }
    void pendingAuthorizationCanBeCanceledWithoutPublishingStream() {
        std::atomic<bool> entered{false};
        auto services = Services(std::make_shared<CDeviceBytes>(fixture_));
        services.open = [&](const ExternalDevice &, const auto &cancelled) {
            entered = true;
            while (!cancelled())
                QThread::msleep(1);
            return DeviceAccessResult{DeviceAccessStatus::Cancelled, {}, {}};
        };
        DeviceSelectionDialog dialog(catalog_, nullptr, services);
        dialog.show();
        QTRY_VERIFY(Items(dialog)->isEnabled());
        Items(dialog)->setCurrentItem(Items(dialog)->topLevelItem(1));
        Open(dialog)->click();
        QTRY_VERIFY(entered.load());
        QSignalSpy rejected(&dialog, &QDialog::rejected);
        QTest::keyClick(&dialog, Qt::Key_Escape);
        QTRY_COMPARE(rejected.size(), 1);
        QVERIFY(!dialog.selectedStream());
    }
    void menuToSourcePanelCopyRefreshAndParent_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("needsRecovery");
        QTest::newRow("en") << QString("en") << false;
        QTest::newRow("ru") << QString("ru") << false;
        QTest::newRow("en-recovery") << QString("en") << true;
        QTest::newRow("ru-recovery") << QString("ru") << true;
    }
    void menuToSourcePanelCopyRefreshAndParent() {
        QFETCH(QString, language);
        QFETCH(bool, needsRecovery);
        SaveRegLang(UString(language == "en" ? L"-" : L"ru"),
                    qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        auto bytes = std::make_shared<CDeviceBytes>(fixture_);
        if (needsRecovery) {
            // Controlled Ext declarations inside the first GPT partition.
            const int superblock = 1024 * 1024 + 1024;
            bytes->Bytes[superblock + 0x65] = char(quint8(bytes->Bytes[superblock + 0x65]) & ~4u);
            bytes->Bytes[superblock + 0x60] = char(quint8(bytes->Bytes[superblock + 0x60]) | 4u);
        }
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        MainWindow window(&backend, {}, nullptr, nullptr, Services(bytes));
        window.openPath(directory.path());
        window.show();
        auto *action = window.findChild<QAction *>("openDeviceAction");
        QVERIFY(action);
        QTRY_VERIFY(action->isEnabled());
        QTimer automation;
        unsigned stage = 0;
        QString openingError;
        bool errorGeometry = false;
        connect(&automation, &QTimer::timeout, &window, [&] {
            if (needsRecovery && stage == 2) {
                if (auto *message = window.findChild<QMessageBox *>()) {
                    openingError = message->text();
                    message->layout()->activate();
                    errorGeometry = true;
                    for (auto *label : message->findChildren<QLabel *>())
                        if (label->isVisible())
                            errorGeometry &= UiInspection::fullyVisible(*label, *message) &&
                                             UiInspection::labelFits(*label);
                    message->setProperty("uiInspection_language", language);
                    message->setProperty("uiInspection_state", "journal-recovery");
                    errorGeometry &=
                        UiInspection::writeReport(*message, "device-recovery-" + language);
                    ++stage;
                    automation.stop();
                    message->accept();
                }
                return;
            }
            auto *dialog = window.findChild<DeviceSelectionDialog *>();
            if (!dialog || !Items(*dialog)->isEnabled())
                return;
            auto *back = dialog->findChild<QPushButton *>("deviceBackButton");
            if (stage == 0 && Items(*dialog)->topLevelItemCount() == 2) {
                Items(*dialog)->setCurrentItem(Items(*dialog)->topLevelItem(1));
                ++stage;
                Open(*dialog)->click();
            } else if (stage == 1 && back->isVisible()) {
                Items(*dialog)->setCurrentItem(Items(*dialog)->topLevelItem(0));
                ++stage;
                if (!needsRecovery)
                    automation.stop();
                Open(*dialog)->click();
            }
        });
        automation.start(10);
        action->trigger();
        if (needsRecovery) {
            QCOMPARE(stage, 3u);
            QVERIFY2(openingError.contains("RECOVER"), qPrintable(openingError));
            QVERIFY(errorGeometry);
            QVERIFY(!window.findChild<QLabel *>("deviceReadOnlyLabel")->isVisible());
            QVERIFY(action->isEnabled());
            return;
        }
        QCOMPARE(stage, 2u);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QVERIFY(table);
        const QString folder = QStringLiteral("папка 空");
        QTRY_VERIFY(Row(*table, folder) >= 0);
        auto *badge = window.findChild<QLabel *>("deviceReadOnlyLabel");
        QVERIFY(badge && badge->isVisible());
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            window.layout()->activate();
            QCoreApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*badge, window));
            QVERIFY(UiInspection::labelFits(*badge));
            window.setProperty("uiInspection_language", language);
            window.setProperty("uiInspection_state", "direct-device");
            QVERIFY(UiInspection::writeReport(window, "device-panel-" + language + '-' +
                                                          QString::number(size.width())));
        }
        const int row = Row(*table, folder);
        QVERIFY(QMetaObject::invokeMethod(table, "cellDoubleClicked", Qt::DirectConnection,
                                          Q_ARG(int, row), Q_ARG(int, 0)));
        const QString name = QStringLiteral("данные с пробелом.txt");
        QTRY_VERIFY(Row(*table, name) >= 0);
        table->selectRow(Row(*table, name));
        table->setFocus();
        QCoreApplication::processEvents();
        QVERIFY(window.findChild<QAction *>("copyAction")->isEnabled());
        for (const char *command : {"deleteAction", "renameAction", "moveAction"})
            QVERIFY(!window.findChild<QAction *>(command)->isEnabled());
        const QString output = directory.filePath("copied");
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::OverwriteExisting;
        window.extractTo(output, true, {}, options);
        QFile copied(output + '/' + folder + '/' + name);
        QVERIFY2(copied.open(QIODevice::ReadOnly), qPrintable(copied.fileName()));
        QCOMPARE(copied.readAll(), QByteArray("device ext4 payload\n"));
        window.findChild<QAction *>("refreshAction")->trigger();
        QTRY_VERIFY(Row(*table, name) >= 0);
        window.findChild<QAction *>("parentFolderAction")->trigger();
        QTRY_VERIFY(Row(*table, folder) >= 0);
        const QString allOutput = directory.filePath("whole-filesystem");
        window.extractTo(allOutput, false, {}, options);
        QFile allCopied(allOutput + '/' + folder + '/' + name);
        QVERIFY(allCopied.open(QIODevice::ReadOnly));
        QCOMPARE(allCopied.readAll(), QByteArray("device ext4 payload\n"));
        window.findChild<QAction *>("parentFolderAction")->trigger();
        QTRY_VERIFY(!badge->isVisible());
        QVERIFY(QFileInfo::exists(copied.fileName()));
        QCOMPARE(bytes->Bytes, fixture_);
    }
};
#include "review_test_main.h"
SEVENZIP_UI_TEST_MAIN(DeviceUiTests)
#include "device_ui_tests.moc"
