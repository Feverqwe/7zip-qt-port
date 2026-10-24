#include "archivecodeccatalog.h"
#include "fixtures/drag_event_position.h"
#include "native_archive_fixture.h"
#include "window_state_support.h"

class DragDropAddUiTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void externalArchiveDrop_data() {
        QTest::addColumn<bool>("valid");
        QTest::addColumn<bool>("multiple");
        QTest::addColumn<bool>("peer");
        QTest::newRow("open-archive") << true << false << false;
        QTest::newRow("open-in-other-panel") << true << false << true;
        QTest::newRow("damaged-archive") << false << false << false;
        QTest::newRow("multiple-files-add") << true << true << false;
    }
    void externalArchiveDrop() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Requires the official native module");
#endif
        QFETCH(bool, valid);
        QFETCH(bool, multiple);
        QFETCH(bool, peer);
        QTemporaryDir root;
        QFile input(root.filePath(QStringLiteral("содержимое.txt")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        input.write("archive drop content");
        input.close();
        const QString archive = root.filePath(QStringLiteral("мой архив.ZIP"));
        ArchiveBackend backend;
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog probe(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(
            backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), probe.version()),
            S_OK);
#endif
        QSignalSpy created(&backend, &ArchiveBackend::finished);
        QVERIFY(NativeArchiveFixture::WriteFiles(archive, {input.fileName()}));
        if (!valid) {
            QFile damaged(archive);
            QVERIFY(damaged.open(QIODevice::WriteOnly | QIODevice::Truncate));
            damaged.write("damaged archive");
        }
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(root.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(table->isEnabled());
        QLineEdit *otherAddress = nullptr;
        if (peer) {
            auto *two = window.findChild<QAction *>("twoPanelsAction");
            if (!two->isChecked())
                two->trigger();
            auto *right = window.findChild<QWidget *>("filePanel1");
            QVERIFY(right && right->isVisible());
            otherAddress = address;
            address = right->findChild<QLineEdit *>("archivePath");
            table = right->findChild<QTableWidget *>("archiveTable");
            address->setFocus();
            address->setText(root.path());
            QTest::keyClick(address, Qt::Key_Return);
            QTRY_VERIFY(table->isEnabled());
            QTRY_VERIFY(table->rowCount() >= 2);
            QTRY_VERIFY((otherAddress->setFocus(), otherAddress->hasFocus()));
        }
        QMimeData mime;
        QList<QUrl> urls{QUrl::fromLocalFile(archive)};
        if (multiple)
            urls.append(QUrl::fromLocalFile(input.fileName()));
        mime.setUrls(urls);
        QDragEnterEvent enter(DragEventPosition(QPoint(5, 5)), Qt::CopyAction | Qt::MoveAction,
                              &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        bool addShown = false, errorShown = false;
        QTimer respond;
        connect(&respond, &QTimer::timeout, &window, [&] {
            if (auto *dialog =
                    dynamic_cast<CreateArchiveDialog *>(QApplication::activeModalWidget())) {
                addShown = true;
                dialog->reject();
            } else if (auto *error =
                           qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errorShown = true;
                error->accept();
            }
        });
        respond.start(5);
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction | Qt::MoveAction, &mime, Qt::NoButton,
                        Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &drop);
        respond.stop();
        QCOMPARE(addShown, multiple);
        QCOMPARE(errorShown, !valid);
        QVERIFY(QFileInfo::exists(archive));
        if (valid && !multiple) {
            QVERIFY(drop.isAccepted());
            QCOMPARE(drop.dropAction(), Qt::CopyAction);
            QVERIFY(address->text().contains(QFileInfo(archive).fileName()));
            bool found = false;
            for (int row = 0; row < table->rowCount(); ++row)
                found |= table->item(row, 0) &&
                         table->item(row, 0)->text() == QFileInfo(input.fileName()).fileName();
            QVERIFY(found);
            if (otherAddress)
                QCOMPARE(QDir::cleanPath(otherAddress->text()), QDir::cleanPath(root.path()));
        } else {
            QCOMPARE(QDir::cleanPath(address->text()), QDir::cleanPath(root.path()));
        }
    }
    void externalFilesystemDropCreatesArchiveWithNativeName() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Source drop Add requires the official native module");
#endif
        QTemporaryDir root;
        QVERIFY(QDir().mkpath(root.filePath("input")));
        QVERIFY(QDir().mkpath(root.filePath("output")));
        const QString name = QStringLiteral("добавленный файл.bin");
        QFile input(QDir(root.filePath("input")).filePath(name));
        QVERIFY(input.open(QIODevice::WriteOnly));
        const QByteArray bytes("Add\0native bytes", 16);
        QCOMPARE(input.write(bytes), qint64(bytes.size()));
        input.close();
        ArchiveBackend backend;
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog probe(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(
            backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), probe.version()),
            S_OK);
#endif
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(root.filePath("output"));
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(table->isEnabled());
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(input.fileName())});
        QDragEnterEvent enter(DragEventPosition(QPoint(5, 5)), Qt::CopyAction, &mime,
                              Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QString archiveName, format, error;
        QTimer accept;
        accept.setInterval(5);
        connect(&accept, &QTimer::timeout, &window, [&] {
            if (auto *dialog =
                    dynamic_cast<CreateArchiveDialog *>(QApplication::activeModalWidget())) {
                accept.stop();
                archiveName = dialog->findChild<QLineEdit *>("archiveName")->text();
                format = dialog->findChild<QComboBox *>("archiveFormat")->currentText();
                if (archiveName != QFileInfo(name).completeBaseName() + "." + format) {
                    error = "Native archive name or selected format suffix is missing";
                    dialog->reject();
                    return;
                }
                dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            }
        });
        accept.start();
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &drop);
        accept.stop();
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY(!archiveName.isEmpty());
        QVERIFY(drop.isAccepted());
        const QString archive = QDir(root.filePath("output")).filePath(archiveName);
        QVERIFY(QFileInfo::exists(archive));
        QVERIFY(!QFileInfo::exists(QDir(root.filePath("output")).filePath(name)));
        QVERIFY(QFileInfo::exists(input.fileName()));
        QVERIFY(!backend.busy());
        ArchiveBackend verifier;
        QSignalSpy finished(&verifier, &ArchiveBackend::finished);
        verifier.list(archive);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        const QString output = root.filePath("verify");
        verifier.extract(archive, output);
        if (finished.isEmpty())
            QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QFile result(QDir(output).filePath(name));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), bytes);
    }
};
QTEST_MAIN(DragDropAddUiTests)
#include "drag_drop_add_ui_tests.moc"
