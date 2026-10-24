#include "7zip/UI/FileManager/LinkDialogRes.h"
#include "panel_model_support.h"
#include "window_state_support.h"
#include <QRadioButton>

class LinkCommandTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void sourceMenuCreatesLinkAndKeepsSelection_data() {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("create") << false;
        QTest::newRow("cancel") << true;
    }
    void sourceMenuCreatesLinkAndKeepsSelection() {
        QFETCH(bool, cancel);
        QTemporaryDir temp;
        const QString name = QStringLiteral("исходный файл 空.txt");
        QFile input(temp.filePath(name));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("source bytes"), qint64(12));
        input.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY(filesystemRow(window, name) >= 0);
        table(window)->selectRow(filesystemRow(window, name));
        auto *link = action(window, "linkAction");
        QTRY_VERIFY(link->isEnabled());
        const QString output = temp.filePath(QStringLiteral("новая ссылка.txt"));
        bool opened = false;
        QStringList errors;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors << box->text();
                if (auto *parent = qobject_cast<QDialog *>(box->parentWidget()))
                    QTimer::singleShot(0, parent, &QDialog::reject);
                box->accept();
            } else if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                       dialog && dialog->objectName() == "IDD_LINK" && !opened) {
                opened = true;
                struct RejectOnFailure {
                    QDialog &dialog;
                    bool armed = true;
                    ~RejectOnFailure() {
                        if (armed)
                            dialog.reject();
                    }
                } guard{*dialog};
                auto *from = dialog->findChild<QComboBox *>("IDC_LINK_PATH_FROM");
                auto *to = dialog->findChild<QComboBox *>("IDC_LINK_PATH_TO");
                QCOMPARE(QDir::fromNativeSeparators(to->currentText())
                             .normalized(QString::NormalizationForm_C),
                         temp.filePath(name).normalized(QString::NormalizationForm_C));
                from->setEditText(output);
                dialog->findChild<QRadioButton *>("IDR_LINK_TYPE_HARD")->setChecked(true);
                if (cancel)
                    dialog->reject();
                else
                    QTimer::singleShot(0, dialog->findChild<QPushButton *>("IDB_LINK_LINK"),
                                       &QPushButton::click);
                guard.armed = false;
            }
        });
        responder.start(1);
        link->trigger();
        responder.stop();
        QVERIFY(opened);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join(';')));
        QCOMPARE(QFileInfo::exists(output), !cancel);
        QTRY_VERIFY(link->isEnabled());
        QTRY_VERIFY(filesystemRow(window, name) >= 0);
        QVERIFY(PanelIndexSelected(table(window)->model()->index(filesystemRow(window, name), 0)));
        if (!cancel) {
            QFile appended(output);
            QVERIFY(appended.open(QIODevice::Append));
            QCOMPARE(appended.write("!"), qint64(1));
            appended.close();
            QVERIFY(input.open(QIODevice::ReadOnly));
            QCOMPARE(input.readAll(), QByteArray("source bytes!"));
        }
        QCOMPARE(backend.listCalls, 0);
        QVERIFY(!QApplication::activeModalWidget());
    }
    void unavailableForMultipleSelectionAndArchive() {
        QTemporaryDir temp;
        for (const char *name : {"one", "two"}) {
            QFile input(temp.filePath(name));
            QVERIFY(input.open(QIODevice::WriteOnly));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(table(window)->rowCount(), 2);
        table(window)->selectAll();
        QVERIFY(!action(window, "linkAction")->isEnabled());
        openListedArchive(window, backend, temp.filePath("fixture.7z"),
                          {ArchiveEntry{"one", 1, 1}});
        table(window)->selectRow(0);
        QVERIFY(!action(window, "linkAction")->isEnabled());
    }
};
QTEST_MAIN(LinkCommandTests)
#include "link_command_tests.moc"
