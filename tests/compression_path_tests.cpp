#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QScrollArea>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class CompressionPathTests final : public QObject {
    Q_OBJECT
    QTemporaryDir dir_;
    QByteArray previousSettings_;
    QString settings_;
    QString source_;

    static void submit(CreateArchiveDialog &dialog) {
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    }

  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        previousSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        settings_ = dir_.filePath("preferences.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.toUtf8());
        source_ = dir_.filePath(QStringLiteral("источник с пробелом.txt"));
        QFile file(source_);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("archive path regression"), qint64(23));
    }
    void init() { QFile::remove(settings_); }
    void cleanupTestCase() {
        if (previousSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", previousSettings_);
    }
    void namesAndFormats() {
        CCompressDialog path;
        path.SetArchiveName("report.tar.gz");
        QCOMPARE(path.m_ArchivePath, QString("report.tar.7z"));
        path.Format = "zip";
        path.SetArchiveName2();
        QCOMPARE(path.m_ArchivePath, QString("report.tar.zip"));
        path.SetArchiveName(".hidden");
        QCOMPARE(path.m_ArchivePath, QString(".hidden.zip"));
        path.SetArchiveName("folder.with.dot/plain");
        QCOMPARE(path.m_ArchivePath, QString("folder.with.dot/plain.zip"));
        path.SetArchiveName(QStringLiteral("папка/.скрытый"));
        QCOMPARE(path.m_ArchivePath, QStringLiteral("папка/.скрытый.zip"));
        path.ArcPath_WasChanged("NAME.7Z");
        QCOMPARE(path.Format, QString("7z"));
        path.ArcPath_WasChanged("unknown.rar");
        QCOMPARE(path.Format, QString("7z"));
        path.ArcPath_WasChanged(".zip");
        QCOMPARE(path.Format, QString("7z"));
    }
    void fullPathsAndFailure() {
        CCompressDialog path;
        path.StartDirPrefix = dir_.path();
        path.m_ArchivePath = QStringLiteral("  вложенная/архив с точкой.7z  ");
        QString resolved;
        QVERIFY(path.GetFinalPath_Smart(resolved));
        QCOMPARE(resolved, dir_.filePath(QStringLiteral("вложенная/архив с точкой.7z")));
        path.DirPrefix = dir_.filePath("other");
        QVERIFY(path.GetFinalPath_Smart(resolved));
        QCOMPARE(resolved, dir_.filePath(QStringLiteral("other/вложенная/архив с точкой.7z")));
        path.m_ArchivePath = source_;
        QVERIFY(path.GetFinalPath_Smart(resolved));
        QCOMPARE(resolved, source_);
        QVERIFY(path.SetArcPathFields(source_));
        QCOMPARE(path.DirPrefix, dir_.path() + '/');
        QCOMPARE(path.m_ArchivePath, QFileInfo(source_).fileName());
        QString name = "unchanged";
        QVERIFY(!path.SetArcPathFields(QString(), name, false));
        QCOMPARE(name, QString("unchanged"));
        path.m_ArchivePath = QString("bad") + QChar::Null + "name.7z";
        QVERIFY(!path.GetFinalPath_Smart(resolved));
        QVERIFY(resolved.isEmpty());
    }
    void boundedHistoryAndSettings() {
        NCompression::CInfo registry;
        for (int i = 0; i < 25; i++)
            registry.ArcPaths.Add(dir_.filePath(QString("old %1.zip").arg(i)));
        registry.ArcPaths.Insert(0, dir_.filePath("NEW.ZIP"));
        CCompressDialog path;
        path.m_ArchivePath = dir_.filePath("new.zip");
        path.Format = "zip";
        path.OnOK(registry);
        QCOMPARE(registry.ArcPaths.Size(), unsigned(20));
        QCOMPARE(registry.ArcPaths[0], path.m_ArchivePath);
        QCOMPARE(registry.ArcPaths[1], dir_.filePath("old 0.zip"));
        registry.Save(settings_);
        NCompression::CInfo loaded;
        loaded.Load(settings_);
        QCOMPARE(loaded.ArcType, QString("zip"));
        QCOMPARE(loaded.ArcPaths.Size(), unsigned(20));
        QCOMPARE(loaded.ArcPaths[0], path.m_ArchivePath);
        loaded.Load(dir_.filePath("absent.ini"));
        QCOMPARE(loaded.ArcType, QString("7z"));
        QVERIFY(loaded.ArcPaths.IsEmpty());
    }
    void historySelectionAndCancel() {
        NCompression::CInfo registry;
        registry.ArcPaths.Add(dir_.filePath(QStringLiteral("готовый архив.ZIP")));
        registry.Save(settings_);
        CreateArchiveDialog dialog(dir_.path(), {source_});
        auto *combo = dialog.findChild<QComboBox *>("archivePath");
        QVERIFY(combo);
        QCOMPARE(combo->count(), 1);
        QCOMPARE(combo->currentText(), QString("archive.7z"));
        combo->setCurrentIndex(0);
        QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, 0)));
        QCOMPARE(dialog.findChild<QComboBox *>("archiveFormat")->currentText(), QString("zip"));
        QCOMPARE(combo->currentText(), QStringLiteral("готовый архив.ZIP"));
        QCOMPARE(dialog.findChild<QLineEdit *>("archiveDestination")->text(), dir_.path() + '/');
        dialog.reject();
        NCompression::CInfo loaded;
        loaded.Load(settings_);
        QCOMPARE(loaded.ArcType, QString("7z"));
        QCOMPARE(loaded.ArcPaths[0], registry.ArcPaths[0]);
    }
    void sourceNavigationKeepsInitialPrefix() {
        QVERIFY(QDir(dir_.path()).mkpath("source-navigation"));
        CreateArchiveDialog dialog(dir_.path(), {source_});
        auto *sourceDirectory = dialog.findChild<QLineEdit *>("sourceDirectory");
        sourceDirectory->setText(dir_.filePath("source-navigation"));
        QVERIFY(QMetaObject::invokeMethod(sourceDirectory, "returnPressed"));
        dialog.findChild<QLineEdit *>("archiveDestination")->clear();
        submit(dialog);
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(dialog.target(), dir_.filePath("archive.7z"));
    }
    void realArchiveAndValidation() {
        CreateArchiveDialog dialog(dir_.path(), {source_});
        auto *name = dialog.findChild<QLineEdit *>("archiveName");
        name->setText(QStringLiteral("готовый архив.7z"));
        submit(dialog);
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(dialog.target(), dir_.filePath(QStringLiteral("готовый архив.7z")));
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(dialog.target(), dialog.sources(), {}, false, dialog.compressionOptions());
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        const QList<QVariant> created = finished.takeFirst();
        QVERIFY2(created.at(0).toBool(), qPrintable(created.at(1).toString()));
        QProcess process;
        process.start("7zz", {"t", "--", dialog.target()});
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);
        const QString output = dir_.filePath("extracted");
        process.start("7zz", {"x", "-o" + output, "--", dialog.target()});
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);
        QFile extracted(QDir(output).filePath(QFileInfo(source_).fileName()));
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), QByteArray("archive path regression"));
        CreateArchiveDialog invalid(dir_.path(), {source_});
        invalid.findChild<QLineEdit *>("archiveName")->setText("absent/invalid.7z");
        QTimer::singleShot(0, &invalid, [] {
            if (auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                message->accept();
        });
        submit(invalid);
        QCOMPARE(invalid.result(), 0);
        NCompression::CInfo registry;
        registry.Load(settings_);
        QCOMPARE(registry.ArcPaths.Size(), unsigned(1));
        QCOMPARE(registry.ArcPaths[0], dialog.target());
        QVERIFY(!QFileInfo::exists(dir_.filePath("absent")));
    }
    void geometry() {
        for (const QSize size : {QSize(760, 720), QSize(640, 480)}) {
            CreateArchiveDialog dialog(dir_.path());
            dialog.resize(size);
            dialog.show();
            QApplication::processEvents();
            auto *combo = dialog.findChild<QComboBox *>("archivePath");
            auto *format = dialog.findChild<QComboBox *>("archiveFormat");
            dialog.findChild<QScrollArea *>()->ensureWidgetVisible(combo);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*combo, dialog));
            QVERIFY(UiInspection::separate(*combo, *format, dialog));
            QVERIFY(
                UiInspection::writeReport(dialog, QString("archive-path-%1").arg(size.width())));
        }
    }
};
QTEST_MAIN(CompressionPathTests)
#include "compression_path_tests.moc"
