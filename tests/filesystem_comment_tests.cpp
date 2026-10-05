#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/ComboDialog.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "Windows/PropVariant.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QScopeGuard>

namespace {
bool Write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray Read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
QString Comment(IFolderFolder *folder, UInt32 index) {
    NWindows::NCOM::CPropVariant value;
    if (folder->GetProperty(index, kpidComment, &value) != S_OK || value.vt != VT_BSTR)
        return {};
    return QString::fromWCharArray(value.bstrVal);
}
UInt32 Find(IFolderFolder *folder, const QString &name) {
    UInt32 count = 0;
    folder->GetNumberOfItems(&count);
    for (UInt32 i = 0; i < count; ++i) {
        NWindows::NCOM::CPropVariant value;
        folder->GetProperty(i, kpidName, &value);
        if (value.vt == VT_BSTR && QString::fromWCharArray(value.bstrVal) == name)
            return i;
    }
    return count;
}
} // namespace

class FilesystemCommentTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static int FsRow(MainWindow &window, const QString &name) {
        for (int row = 0; row < table(window)->rowCount(); ++row)
            if (table(window)->item(row, 0)->data(Qt::UserRole + 3).toString() == name)
                return row;
        return -1;
    }
  private slots:
    void sourceWriterPreservesOtherPairsAndClears() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("имя 空 with spaces.bin");
        const QByteArray payload("binary\0bytes", 12);
        QVERIFY(Write(fixture.filePath(name), payload));
        const QString description = fixture.filePath("descript.ion");
        QVERIFY(Write(description, "\"other name\" retained\r\n"));
        const auto permissions = QFile::permissions(description);
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(fixture.path());
        QCOMPARE(folder->LoadItems(), S_OK);
        const UInt32 index = Find(folder, name);
        QVERIFY(Comment(folder, index).isEmpty()); // Prime the original read cache.
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        NWindows::NCOM::CPropVariant value(L"  Комментарий 空  ");
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QCOMPARE(Comment(folder, index), QStringLiteral("Комментарий 空"));
        const QByteArray written = Read(description);
        QVERIFY(written.startsWith("\xEF\xBB\xBF\r\n"));
        QVERIFY(written.contains("\"other name\" retained\r\n"));
        QVERIFY(written.contains(('"' + name + "\" Комментарий 空\r\n").toUtf8()));
        QCOMPARE(QFile::permissions(description), permissions);
        QCOMPARE(Read(fixture.filePath(name)), payload);
        value = UInt32(3);
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), E_INVALIDARG);
        QCOMPARE(Read(description), written);
        QCOMPARE(operations->SetProperty(UInt32(-1), kpidComment, &value, nullptr), E_INVALIDARG);
        QCOMPARE(operations->SetProperty(index, kpidName, &value, nullptr), E_NOTIMPL);
        value = L"   ";
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QVERIFY(Comment(folder, index).isEmpty());
        QVERIFY(!Read(description).contains(name.toUtf8()));
        value = L"again";
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        value.Clear();
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QVERIFY(Comment(folder, index).isEmpty());
        QCOMPARE(Read(fixture.filePath(name)), payload);
    }
    void sourceSaveFailureKeepsOriginalReturnPolicy() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(Write(fixture.filePath("payload"), "unchanged"));
        QVERIFY(QDir(fixture.path()).mkdir("descript.ion"));
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(fixture.path());
        QCOMPARE(folder->LoadItems(), S_OK);
        const UInt32 index = Find(folder, "payload");
        QVERIFY(Comment(folder, index).isEmpty());
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        NWindows::NCOM::CPropVariant value(L"not saved");
        // Original SetProperty ignores SaveComments's false return.
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QVERIFY(QFileInfo(fixture.filePath("descript.ion")).isDir());
        QCOMPARE(folder->LoadItems(), S_OK);
        QVERIFY(Comment(folder, Find(folder, "payload")).isEmpty());
        QCOMPARE(Read(fixture.filePath("payload")), QByteArray("unchanged"));
    }
    void flatNestedItemRetainsSourceRefusal() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(QDir(fixture.path()).mkdir("sub"));
        QVERIFY(Write(fixture.filePath("sub/leaf.bin"), "unchanged"));
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(fixture.path());
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        NWindows::NCOM::CPropVariant value(L"refused");
        QCOMPARE(operations->SetProperty(Find(folder, "leaf.bin"), kpidComment, &value, nullptr),
                 E_NOTIMPL);
        QVERIFY(!QFileInfo::exists(fixture.filePath("descript.ion")));
        QVERIFY(!QFileInfo::exists(fixture.filePath("sub/descript.ion")));
        QCOMPARE(Read(fixture.filePath("sub/leaf.bin")), QByteArray("unchanged"));
    }
    void actualFilesystemCommand_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("cancel");
        for (const QString &language : {"-", "ru"})
            for (bool cancel : {false, true})
                QTest::newRow(qPrintable(language + QString::number(cancel))) << language << cancel;
    }
    void actualFilesystemCommand() {
        QFETCH(QString, language);
        QFETCH(bool, cancel);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        CFmSettings preferences;
        preferences.Load(settings);
        preferences.ShowDots = true;
        preferences.Save(settings);
        const auto resetLanguage = qScopeGuard([&] {
            SaveRegLang(L"-", settings);
            ReloadLang(settings);
        });
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("имя 空 with spaces.bin");
        QVERIFY(Write(fixture.filePath(name), "unchanged bytes"));
        QVERIFY(Write(fixture.filePath("other.bin"), "other bytes"));
        const QString description = fixture.filePath("descript.ion");
        const QByteArray initial = ('"' + name + "\" initial\r\nother.bin retained\r\n").toUtf8();
        QVERIFY(Write(description, initial));
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        window.openPath(fixture.path());
        QTRY_VERIFY(FsRow(window, name) >= 0);
        const int row = FsRow(window, name);
        table(window)->setCurrentCell(row, 0);
        table(window)->selectAll();
        const int parent = FsRow(window, "..");
        QVERIFY(parent >= 0);
        table(window)->selectionModel()->select(table(window)->model()->index(parent, 0),
                                                QItemSelectionModel::Deselect |
                                                    QItemSelectionModel::Rows);
        QTRY_VERIFY(action(window, "commentAction")->isEnabled());
        bool observed = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *dialog = dynamic_cast<CComboDialog *>(QApplication::activeModalWidget());
            if (!dialog || observed)
                return;
            observed = true;
            const auto close = qScopeGuard([&] {
                if (dialog->isVisible())
                    dialog->reject();
            });
            QVERIFY(dialog->windowTitle().contains(name));
            auto *edit = dialog->findChild<QComboBox *>("comboValue");
            QCOMPARE(edit->currentText(), QStringLiteral("initial"));
            QCOMPARE(dialog->findChild<QLabel *>("comboPrompt")->text(),
                     QString::fromWCharArray(LangString(IDS_COMMENT2)));
            for (int width : {360, 260}) {
                dialog->resize(width, 150);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*edit, *dialog));
                auto *buttons = dialog->findChild<QDialogButtonBox *>("comboButtons");
                QVERIFY(UiInspection::fullyVisible(*buttons, *dialog));
                QVERIFY(UiInspection::writeReport(*dialog, QString("filesystem-comment-%1-%2-%3")
                                                               .arg(language == "ru" ? "ru" : "en")
                                                               .arg(cancel)
                                                               .arg(width)));
            }
            edit->setEditText(QStringLiteral("  changed 空  "));
            if (cancel)
                QTest::keyClick(dialog, Qt::Key_Escape);
            else
                dialog->findChild<QDialogButtonBox *>("comboButtons")
                    ->button(QDialogButtonBox::Ok)
                    ->click();
        });
        responder.start(2);
        if (cancel) {
            window.activateWindow();
            table(window)->setFocus();
            QTRY_VERIFY(table(window)->hasFocus());
            QTest::keyClick(table(window), Qt::Key_Z, Qt::ControlModifier);
        } else
            action(window, "commentAction")->trigger();
        responder.stop();
        QVERIFY(observed);
        QCOMPARE(table(window)->currentRow(), FsRow(window, name));
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 3);
        QCOMPARE(Read(fixture.filePath(name)), QByteArray("unchanged bytes"));
        QCOMPARE(Read(fixture.filePath("other.bin")), QByteArray("other bytes"));
        if (cancel)
            QCOMPARE(Read(description), initial);
        else {
            CMyComPtr<IFolderFolder> reopened = new NFsFolder::CFSFolder(fixture.path());
            QCOMPARE(reopened->LoadItems(), S_OK);
            QCOMPARE(Comment(reopened, Find(reopened, name)), QStringLiteral("changed 空"));
            QCOMPARE(Comment(reopened, Find(reopened, "other.bin")), QStringLiteral("retained"));
        }
        table(window)->setCurrentCell(parent, 0);
        table(window)->selectRow(parent);
        QTRY_VERIFY(!action(window, "commentAction")->isEnabled());
    }
};
QTEST_MAIN(FilesystemCommentTests)
#include "filesystem_comment_tests.moc"
