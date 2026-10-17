#include "CPP/7zip/UI/FileManager/SystemPage.h"
#include "Common/MyCom.h"
#include "Common/StringConvert.h"
#include "qtassociationplatform.h"
#include "qtfmoptionsdialog.h"
#include <QDialogButtonBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QUuid>
#include <QtTest>
#include <ShlObj.h>

namespace {
class FormatFixture final : public IFolderManager, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderManager)
    const UString extension_;

  public:
    explicit FormatFixture(const QString &extension)
        : extension_(extension.toStdWString().c_str()) {}
    Z7_COM7F_IMF(OpenFolderFile(IInStream *, const wchar_t *, const wchar_t *, IFolderFolder **,
                                IProgress *))
    override { return E_NOTIMPL; }
    Z7_COM7F_IMF(GetExtensions(BSTR *extensions)) override {
        return StringToBstr(extension_, extensions);
    }
    Z7_COM7F_IMF(GetIconPath(const wchar_t *, BSTR *path, Int32 *index)) override {
        *index = 0;
        return StringToBstr(UString(QCoreApplication::applicationFilePath().toStdWString().c_str()),
                            path);
    }
};

LONG keyStatus(HKEY root, const QString &path) {
    HKEY key = nullptr;
    const LONG result =
        RegOpenKeyExW(root, reinterpret_cast<LPCWSTR>(path.utf16()), 0, KEY_READ, &key);
    if (key)
        RegCloseKey(key);
    return result;
}
} // namespace

class WindowsSystemRegistrationTests final : public QObject {
    Q_OBJECT
    void verifyShellRecipient(const QTemporaryDir &fixture, const QString &extension,
                              const QString &programKey) {
        // Ask Windows to resolve the registered command, rather than execute a
        // copied command string. The child verifies the original Unicode bytes.
        const QString file =
            fixture.filePath(QString::fromUtf8("документ 空 with spaces.") + extension);
        const QByteArray payload("native association\0bytes", 24);
        QFile input(file);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write(payload), qint64(payload.size()));
        input.close();
        const QString receipt = fixture.filePath("recipient.json");
        const QByteArray oldReceipt = qgetenv("SEVENZIP_REGISTRATION_RECEIPT");
        const bool wasSet = qEnvironmentVariableIsSet("SEVENZIP_REGISTRATION_RECEIPT");
        const auto restore = qScopeGuard([&] {
            if (wasSet)
                qputenv("SEVENZIP_REGISTRATION_RECEIPT", oldReceipt);
            else
                qunsetenv("SEVENZIP_REGISTRATION_RECEIPT");
        });
        qputenv("SEVENZIP_REGISTRATION_RECEIPT", receipt.toUtf8());
        const QString nativeFile = QDir::toNativeSeparators(file);
        SHELLEXECUTEINFOW launch = {};
        launch.cbSize = sizeof(launch);
        launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        launch.lpVerb = L"open";
        launch.lpFile = reinterpret_cast<LPCWSTR>(nativeFile.utf16());
        launch.nShow = SW_SHOWNORMAL;
        const BOOL launched = ShellExecuteExW(&launch);
        const DWORD launchError = GetLastError();
        QSettings registration("HKEY_CURRENT_USER\\" + programKey + "\\shell\\open\\command",
                               QSettings::NativeFormat);
        const QString launchDiagnostic = QString("error=%1 shell=%2 command=%3")
                                             .arg(launchError)
                                             .arg(quintptr(launch.hInstApp))
                                             .arg(registration.value(".").toString());
        QVERIFY2(launched, qPrintable(launchDiagnostic));
        const auto closeProcess = qScopeGuard([&] {
            if (launch.hProcess) {
                if (WaitForSingleObject(launch.hProcess, 0) == WAIT_TIMEOUT) {
                    TerminateProcess(launch.hProcess, 1);
                    WaitForSingleObject(launch.hProcess, 5000);
                }
                CloseHandle(launch.hProcess);
            }
        });
        QVERIFY(launch.hProcess);
        QTRY_COMPARE(WaitForSingleObject(launch.hProcess, 0), DWORD(WAIT_OBJECT_0));
        DWORD exitCode = 1;
        QVERIFY(GetExitCodeProcess(launch.hProcess, &exitCode));
        QCOMPARE(exitCode, DWORD(0));
        QFile received(receipt);
        QVERIFY(received.open(QIODevice::ReadOnly));
        const auto actual = QJsonDocument::fromJson(received.readAll()).object();
        QCOMPARE(QFileInfo(actual["path"].toString()).canonicalFilePath(),
                 QFileInfo(file).canonicalFilePath());
        QCOMPARE(QByteArray::fromBase64(actual["bytes"].toString().toLatin1()), payload);
        QVERIFY(input.open(QIODevice::ReadOnly));
        QCOMPARE(input.readAll(), payload);
    }
  private slots:
    void sourceApplyRegistersShellRecipientAndPreservesExternalChanges() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString extension = "sevenzip-fixture-" + QUuid::createUuid().toString(QUuid::Id128);
        const QStringList keys = {
            "Software\\Classes\\." + extension, "Software\\Classes\\7-Zip." + extension,
            "Software\\Classes\\Fixture.Other." + extension,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\." + extension};
        for (const auto &key : keys) {
            QCOMPARE(keyStatus(HKEY_CURRENT_USER, key), LONG(ERROR_FILE_NOT_FOUND));
            QCOMPARE(keyStatus(HKEY_LOCAL_MACHINE, key), LONG(ERROR_FILE_NOT_FOUND));
        }
        const auto cleanup = qScopeGuard([&] {
            for (const auto &key : keys)
                RegDeleteTreeW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(key.utf16()));
            SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        });
        CMyComPtr<IFolderManager> manager = new FormatFixture(extension);
        const auto service = CreateQtAssociationServices(CreateNativeAssociationPlatform());
        const auto options = [&] {
            return std::make_unique<QtFmOptionsDialog>(fixture.filePath("settings.ini"), nullptr,
                                                       manager, service);
        };
        {
            auto dialog = options();
            dialog->show();
            QVERIFY(QTest::qWaitForWindowActive(dialog.get()));
            auto *list = dialog->findChild<QTreeWidget *>("systemAssociations");
            QCOMPARE(list->topLevelItemCount(), 1);
            QCOMPARE(list->topLevelItem(0)->text(1), QString());
            dialog->findChild<QPushButton *>("systemCurrent")->click();
            QCOMPARE(list->topLevelItem(0)->text(1), QString("7-Zip"));
            dialog->findChild<QDialogButtonBox *>("settingsButtons")
                ->button(QDialogButtonBox::Cancel)
                ->click();
        }
        QCOMPARE(keyStatus(HKEY_CURRENT_USER, keys[0]), LONG(ERROR_FILE_NOT_FOUND));
        auto dialog = options();
        dialog->show();
        QVERIFY(QTest::qWaitForWindowActive(dialog.get()));
        auto *list = dialog->findChild<QTreeWidget *>("systemAssociations");
        auto *page = dialog->findChild<CSystemPage *>();
        auto *change = dialog->findChild<QPushButton *>("systemCurrent");
        auto *apply = dialog->findChild<QDialogButtonBox *>("settingsButtons")
                          ->button(QDialogButtonBox::Apply);
        change->click();
        QVERIFY(apply->isEnabled());
        QSignalSpy errors(page, &CSystemPage::ApplyError);
        apply->click();
        QCOMPARE(errors.count(), 0);
        QCOMPARE(list->topLevelItem(0)->text(1), QString("7-Zip"));
        QVERIFY(!apply->isEnabled());

        verifyShellRecipient(fixture, extension, keys[1]);
        QVERIFY(!QTest::currentTestFailed());

        change->click();
        apply->click();
        QCOMPARE(errors.count(), 0);
        QCOMPARE(list->topLevelItem(0)->text(1), QString());
        QCOMPARE(keyStatus(HKEY_CURRENT_USER, keys[0]), LONG(ERROR_FILE_NOT_FOUND));
        QCOMPARE(keyStatus(HKEY_CURRENT_USER, keys[1]), LONG(ERROR_FILE_NOT_FOUND));
        change->click();
        QSettings external("HKEY_CURRENT_USER\\" + keys[0], QSettings::NativeFormat);
        const QString other = "Fixture.Other." + extension;
        external.setValue(".", other);
        external.sync();
        QCOMPARE(external.status(), QSettings::NoError);
        QCOMPARE(page->OnApply(), LONG(ERROR_RETRY));
        QCOMPARE(errors.count(), 1);
        QCOMPARE(list->topLevelItem(0)->text(1), other);
        external.sync();
        QCOMPARE(external.value(".").toString(), other);
        QCOMPARE(keyStatus(HKEY_CURRENT_USER, keys[1]), LONG(ERROR_FILE_NOT_FOUND));
        for (const auto &key : keys)
            QCOMPARE(keyStatus(HKEY_LOCAL_MACHINE, key), LONG(ERROR_FILE_NOT_FOUND));
        dialog->reject();
        for (const auto &key : keys) {
            const LONG removed =
                RegDeleteTreeW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(key.utf16()));
            QVERIFY(removed == ERROR_SUCCESS || removed == ERROR_FILE_NOT_FOUND);
            QCOMPARE(keyStatus(HKEY_CURRENT_USER, key), LONG(ERROR_FILE_NOT_FOUND));
        }
    }
};

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    const QString receipt = qEnvironmentVariable("SEVENZIP_REGISTRATION_RECEIPT");
    if (!receipt.isEmpty() && application.arguments().size() == 2) {
        const QString path = application.arguments()[1];
        QFile input(path), output(receipt);
        if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))
            return 2;
        const QJsonDocument data(QJsonObject{
            {"path", path}, {"bytes", QString::fromLatin1(input.readAll().toBase64())}});
        return output.write(data.toJson()) > 0 ? 0 : 3;
    }
    WindowsSystemRegistrationTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "windows_system_registration_tests.moc"
