#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QScrollArea>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class CompressionEncryptionTests final : public QObject {
    Q_OBJECT
    QTemporaryDir directory_;
    QString settings_;
    QString source_;
    QByteArray oldSettings_;

    static void submit(CreateArchiveDialog &dialog) {
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    }
    static void setPassword(CreateArchiveDialog &dialog, const QString &password) {
        dialog.findChild<QLineEdit *>("archivePassword")->setText(password);
        dialog.findChild<QLineEdit *>("archivePasswordConfirmation")->setText(password);
    }

  private slots:
    void initTestCase() {
        QVERIFY(directory_.isValid());
        oldSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        settings_ = directory_.filePath("encryption.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.toUtf8());
        source_ = directory_.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(source_);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(QByteArray(8192, 'z')), qint64(8192));
    }
    void init() { QFile::remove(settings_); }
    void cleanupTestCase() {
        if (oldSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", oldSettings_);
    }
    void upstreamPasswordBoundaries() {
        using P = CCompressDialog;
        QCOMPARE(P::CheckPassword("", "", false, u"zip", ""), P::kPasswordOK);
        QCOMPARE(P::CheckPassword("one", "two", false, u"7z", ""), P::kPasswordNotMatch);
        QCOMPARE(P::CheckPassword("one", "two", true, u"7z", ""), P::kPasswordOK);
        const QString unicode = QStringLiteral("пароль 密碼");
        QCOMPARE(P::CheckPassword(unicode, unicode, false, u"7z", ""), P::kPasswordOK);
        QCOMPARE(P::CheckPassword(unicode, unicode, true, u"zip", ""), P::kPasswordUseAscii);
        QCOMPARE(P::CheckPassword("line\n", "line\n", true, u"zip", "AES256"),
                 P::kPasswordUseAscii);
        QCOMPARE(P::CheckPassword(QString(QChar(0x1f)), {}, true, u"zip", ""),
                 P::kPasswordUseAscii);
        QCOMPARE(P::CheckPassword(QString(QChar(0x7f)), {}, true, u"zip", ""), P::kPasswordOK);
        QCOMPARE(P::CheckPassword(QString(99, 'x'), {}, true, u"zip", "aes256"), P::kPasswordOK);
        QCOMPARE(P::CheckPassword(QString(100, 'x'), {}, true, u"zip", "AES256"),
                 P::kPasswordTooLong);
        QCOMPARE(P::CheckPassword(QString(100, 'x'), {}, true, u"zip", ""), P::kPasswordOK);
    }
    void methodsAndShowPassword() {
        CreateArchiveDialog dialog(directory_.path(), {source_});
        auto *method = dialog.findChild<QComboBox *>("archiveEncryption");
        auto *headers = dialog.findChild<QCheckBox *>("archiveEncryptHeaders");
        auto *show = dialog.findChild<QCheckBox *>("archiveShowPassword");
        auto *repeat = dialog.findChild<QLineEdit *>("archivePasswordConfirmation");
        auto *form = qobject_cast<QFormLayout *>(
            dialog.findChild<QGroupBox *>("archiveEncryptionGroup")->layout());
        QWidget *repeatLabel = form->labelForField(repeat);
        QVERIFY(repeatLabel);
        dialog.show();
        QCOMPARE(method->count(), 1);
        QCOMPARE(method->currentText(), QString("AES-256"));
        QVERIFY(headers->isEnabled());
        QVERIFY(dialog.compressionOptions().EncryptionMethod.isEmpty());
        headers->setChecked(true);
        setPassword(dialog, "shown password");
        repeat->setText("different");
        show->setChecked(true);
        QVERIFY(!repeat->isVisible());
        QVERIFY(!repeatLabel->isVisible());
        QCOMPARE(repeat->echoMode(), QLineEdit::Normal);
        show->setChecked(false);
        QVERIFY(repeat->isVisible());
        QVERIFY(repeatLabel->isVisible());
        QCOMPARE(repeat->text(), QString("different"));
        QCOMPARE(repeat->echoMode(), QLineEdit::Password);
        show->setChecked(true);
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
        QVERIFY(!headers->isEnabled());
        QVERIFY(!dialog.encryptHeaders());
        QCOMPARE(method->count(), 2);
        QCOMPARE(method->currentText(), QString("ZipCrypto"));
        method->setCurrentText("AES-256");
        QCOMPARE(dialog.compressionOptions().EncryptionMethod, QString("AES256"));
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText("7z");
        QVERIFY(dialog.encryptHeaders());
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
        QCOMPARE(method->currentText(), QString("AES-256"));
        submit(dialog);
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        CreateArchiveDialog restored(directory_.path(), {source_});
        QCOMPARE(restored.findChild<QComboBox *>("archiveEncryption")->currentText(),
                 QString("AES-256"));
        QVERIFY(restored.findChild<QCheckBox *>("archiveShowPassword")->isChecked());
        QVERIFY(restored.password().isEmpty());
        restored.findChild<QComboBox *>("archiveFormat")->setCurrentText("7z");
        QVERIFY(restored.encryptHeaders());
        QFile settings(settings_);
        QVERIFY(settings.open(QIODevice::ReadOnly));
        QVERIFY(!settings.readAll().contains("shown password"));
        restored.reject();
    }
    void invalidAndCancelledDoNotSave() {
        CreateArchiveDialog dialog(directory_.path(), {source_});
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
        dialog.findChild<QComboBox *>("archiveEncryption")->setCurrentText("AES-256");
        setPassword(dialog, QString(100, 'x'));
        dialog.findChild<QCheckBox *>("archiveShowPassword")->setChecked(true);
        QTimer::singleShot(0, &dialog, [] {
            if (auto *warning = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                warning->accept();
        });
        submit(dialog);
        QCOMPARE(dialog.result(), 0);
        QVERIFY(!QFileInfo::exists(settings_));
        dialog.reject();
        CreateArchiveDialog restored(directory_.path());
        QCOMPARE(restored.findChild<QComboBox *>("archiveFormat")->currentText(), QString("7z"));
        QVERIFY(!restored.findChild<QCheckBox *>("archiveShowPassword")->isChecked());
    }
    void realArchives_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("encryption");
        QTest::addColumn<QString>("password");
        QTest::addColumn<bool>("headers");
        QTest::newRow("7z-unicode-headers")
            << QString("7z") << QString("AES-256") << QStringLiteral("пароль 密碼") << true;
        QTest::newRow("zip-crypto")
            << QString("zip") << QString("ZipCrypto") << QString(100, 'x') << false;
        QTest::newRow("zip-aes-boundary")
            << QString("zip") << QString("AES-256") << QString(99, 'x') << false;
        QTest::newRow("unencrypted") << QString("zip") << QString("AES-256") << QString() << false;
    }
    void realArchives() {
        QFETCH(QString, format);
        QFETCH(QString, encryption);
        QFETCH(QString, password);
        QFETCH(bool, headers);
        CreateArchiveDialog dialog(directory_.path(), {source_});
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText(format);
        dialog.findChild<QComboBox *>("archiveEncryption")->setCurrentText(encryption);
        dialog.findChild<QCheckBox *>("archiveEncryptHeaders")->setChecked(headers);
        setPassword(dialog, password);
        dialog.findChild<QLineEdit *>("archiveName")
            ->setText(QString::fromLatin1(QTest::currentDataTag()) + '.' + format);
        submit(dialog);
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        backend.create(dialog.target(), dialog.sources(), dialog.password(),
                       dialog.encryptHeaders(), dialog.compressionOptions());
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 30000);
        const auto result = done.takeFirst();
        QVERIFY2(result[0].toBool(), qPrintable(result[1].toString()));
        QProcess process;
        process.start("7zz", {"l", "-slt", "-p" + password, "--", dialog.target()});
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);
        const QByteArray listing = process.readAllStandardOutput();
        QVERIFY(listing.contains(password.isEmpty() ? "Encrypted = -" : "Encrypted = +"));
        if (format == "zip" && !password.isEmpty())
            QVERIFY(listing.contains(encryption == "ZipCrypto" ? "ZipCrypto" : "AES-256"));
        if (headers) {
            process.start("7zz", {"l", "-slt", "-pwrong", "--", dialog.target()});
            QVERIFY(process.waitForFinished(30000));
            QVERIFY(process.exitCode() != 0);
            QVERIFY(
                !process.readAllStandardOutput().contains(QFileInfo(source_).fileName().toUtf8()));
        }
        const QString output =
            directory_.filePath(QString::fromLatin1(QTest::currentDataTag()) + "-out");
        process.start("7zz", {"x", "-p" + password, "-o" + output, "--", dialog.target()});
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);
        QFile file(QDir(output).filePath(QFileInfo(source_).fileName()));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray(8192, 'z'));
    }
    void backendFailureAndCancel() {
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        CompressionOptions options;
        options.EncryptionMethod = "AES256";
        const QString target = directory_.filePath("rejected.zip");
        backend.create(target, {source_}, QString(100, 'x'), false, options);
        QCOMPARE(done.count(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(target));
        backend.create(target, {source_}, QStringLiteral("пароль"), false, options);
        QCOMPARE(done.count(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        options.EncryptionMethod = "unsupported";
        backend.create(target, {source_}, "abc", false, options);
        QCOMPARE(done.count(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        options.EncryptionMethod = "AES256";
        backend.create(directory_.filePath("invalid.7z"), {source_}, "abc", false, options);
        QCOMPARE(done.count(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        backend.create(target, {source_}, "abc", true, options);
        QCOMPARE(done.count(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        backend.create(target, {source_}, "abc", false, options);
        QTimer::singleShot(0, &backend, &ArchiveBackend::cancel);
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 30000);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(target));
        QVERIFY(QDir(directory_.path())
                    .entryList({"*.stage-*", "*.sevenzip-*"}, QDir::Files | QDir::Hidden)
                    .isEmpty());
    }
    void geometry() {
        NCompression::CInfo registry;
        registry.ArcPaths.Add(directory_.filePath(QString(400, QChar(0x044f)) + ".7z"));
        registry.Save(settings_);
        for (const QSize size : {QSize(760, 720), QSize(640, 480)}) {
            for (const bool shown : {false, true}) {
                CreateArchiveDialog dialog(directory_.path());
                dialog.resize(size);
                dialog.findChild<QCheckBox *>("archiveShowPassword")->setChecked(shown);
                dialog.show();
                auto *group = dialog.findChild<QGroupBox *>("archiveEncryptionGroup");
                dialog.findChild<QScrollArea *>()->ensureWidgetVisible(group);
                QApplication::processEvents();
                auto *method = dialog.findChild<QComboBox *>("archiveEncryption");
                auto *headers = dialog.findChild<QCheckBox *>("archiveEncryptHeaders");
                auto *password = dialog.findChild<QLineEdit *>("archivePassword");
                QVERIFY(UiInspection::fullyVisible(*group, dialog));
                QVERIFY(UiInspection::separate(*password, *method, dialog));
                QVERIFY(UiInspection::separate(*method, *headers, dialog));
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("encryption-%1-%2").arg(size.width()).arg(shown)));
            }
        }
    }
};
QTEST_MAIN(CompressionEncryptionTests)
#include "compression_encryption_tests.moc"
