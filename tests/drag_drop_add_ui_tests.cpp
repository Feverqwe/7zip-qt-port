#include "archivecodeccatalog.h"
#include "window_state_support.h"

class DragDropAddUiTests final : public QObject {
    Q_OBJECT
  private slots:
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
        QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
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
