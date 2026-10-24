#include "CPP/Common/Wildcard.h"
#include "extractdialog.h"
#include "native_archive_fixture.h"
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class SplitDestinationTests final : public QObject {
    Q_OBJECT
    QTemporaryDir dir_;
    QByteArray oldSettings_;
    QString settings_;
    QComboBox *path(CExtractDialog &d) { return d.findChild<QComboBox *>("extractDestination"); }
    QLineEdit *name(CExtractDialog &d) { return d.findChild<QLineEdit *>("extractPathName"); }
    QCheckBox *enabled(CExtractDialog &d) { return d.findChild<QCheckBox *>("extractNameEnable"); }
  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        oldSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        settings_ = dir_.filePath("split-dest.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.toUtf8());
    }
    void init() { QFile::remove(settings_); }
    void cleanupTestCase() {
        if (oldSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", oldSettings_);
    }
    void smartSplit_data() {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("prefix");
        QTest::addColumn<QString>("leaf");
        QTest::newRow("empty") << QString() << QString() << QString();
        QTest::newRow("root") << QString("/") << QString() << QString("/");
        QTest::newRow("relative") << QString("name") << QString() << QString("name");
        QTest::newRow("relative-slash") << QString("name/") << QString() << QString("name/");
        QTest::newRow("full") << QString("/a/b") << QString("/a/") << QString("b");
        QTest::newRow("trailing") << QString("/a/b/") << QString("/a/") << QString("b/");
        QTest::newRow("double-trailing") << QString("/a/b//") << QString("/a/b/") << QString("/");
        QTest::newRow("unicode") << QStringLiteral("/папка с пробелом/δ файл/")
                                 << QStringLiteral("/папка с пробелом/")
                                 << QStringLiteral("δ файл/");
#ifdef Q_OS_WIN
        QTest::newRow("native") << QString("C:\\folder\\name\\") << QString("C:\\folder\\")
                                << QString("name\\");
#else
        QTest::newRow("literal-backslash")
            << QString("folder\\name") << QString() << QString("folder\\name");
#endif
    }
    void smartSplit() {
        QFETCH(QString, input);
        QFETCH(QString, prefix);
        QFETCH(QString, leaf);
        QString actualPrefix, actualLeaf;
        SplitPathToParts_Smart(input, actualPrefix, actualLeaf);
        QCOMPARE(actualPrefix, prefix);
        QCOMPARE(actualLeaf, leaf);
    }
    void originalSplitControlsAndPersistence() {
        const QString initial = QStringLiteral("/папка с пробелом/вложенная/");
        {
            CExtractDialog dialog(initial, {});
            QVERIFY(enabled(dialog)->isChecked());
            QCOMPARE(path(dialog)->currentText(), QStringLiteral("/папка с пробелом/"));
            QCOMPARE(name(dialog)->text(), QStringLiteral("вложенная/"));
            QVERIFY(!name(dialog)->isHidden());
            name(dialog)->setText(QStringLiteral("  δ результат  "));
            QCOMPARE(dialog.destination(), QStringLiteral("/папка с пробелом/δ результат"));
            dialog.accept();
            QCOMPARE(QDir::fromNativeSeparators(dialog.destination()),
                     QStringLiteral("/папка с пробелом/δ результат/"));
        }
        NExtract::CInfo saved;
        saved.Load(settings_);
        QCOMPARE(QDir::fromNativeSeparators(saved.Paths[0]),
                 QStringLiteral("/папка с пробелом/δ результат/"));
        {
            CExtractDialog dialog(initial, {});
            enabled(dialog)->setChecked(false);
            QVERIFY(!name(dialog)->isHidden());
            QVERIFY(!name(dialog)->isEnabled());
            path(dialog)->setEditText(QStringLiteral("/другое место"));
            dialog.accept();
            QCOMPARE(QDir::fromNativeSeparators(dialog.destination()),
                     QStringLiteral("/другое место/"));
        }
        {
            CExtractDialog restarted(initial, {});
            QVERIFY(!enabled(restarted)->isChecked());
            QCOMPARE(path(restarted)->currentText(), initial);
            enabled(restarted)->setChecked(true);
            QVERIFY(!name(restarted)->isHidden());
            QVERIFY(name(restarted)->isEnabled());
            restarted.reject();
        }
        CExtractDialog afterCancel(initial, {});
        QVERIFY(!enabled(afterCancel)->isChecked());
    }
    void rootAndLeafDoNotDuplicate() {
        for (QString initial : {QString("/"), QString("name"), QString("name/")}) {
            QFile::remove(settings_);
            CExtractDialog dialog(initial, {});
            QCOMPARE(path(dialog)->currentText(), initial);
            QVERIFY(name(dialog)->text().isEmpty());
            dialog.accept();
            QCOMPARE(QDir::fromNativeSeparators(dialog.destination()),
                     initial.endsWith('/') ? initial : initial + "/");
        }
    }
    void extractionSuccessFailureAndCancel() {
        QTemporaryDir fixture;
        const QString source = fixture.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("source bytes"), qint64(12));
        file.close();
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString archive = fixture.filePath("split-dest.zip");
        QVERIFY(NativeArchiveFixture::WriteFiles(archive, {source}));
        done.clear();
        backend.list(archive);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        done.clear();
        const QString base = fixture.filePath(QStringLiteral("вывод"));
        CExtractDialog dialog(base + "/initial", {});
        // This fixture exercises the basic process transport's destination
        // path; advanced ElimDup is covered by source_extract_advanced.
        dialog.findChild<QCheckBox *>("extractElimDup")->setChecked(false);
        name(dialog)->setText(QStringLiteral("  δ результат  "));
        dialog.accept();
        QCOMPARE(QDir::fromNativeSeparators(dialog.destination()),
                 base + QStringLiteral("/δ результат/"));
        backend.extract(archive, dialog.destination(), {}, {}, dialog.options());
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QFile restored(QDir(dialog.destination()).filePath(QFileInfo(source).fileName()));
        QVERIFY(restored.open(QIODevice::ReadOnly));
        QCOMPARE(restored.readAll(), QByteArray("source bytes"));
        done.clear();
        CExtractDialog invalid(base + "/initial", {});
        invalid.findChild<QCheckBox *>("extractElimDup")->setChecked(false);
        name(invalid)->setText("occupied");
        QFile occupied(base + "/occupied");
        QVERIFY(occupied.open(QIODevice::WriteOnly));
        QCOMPARE(occupied.write("preserve"), qint64(8));
        occupied.close();
        invalid.accept();
        backend.extract(archive, invalid.destination(), {}, {}, invalid.options());
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.first()[0].toBool());
        QVERIFY(occupied.open(QIODevice::ReadOnly));
        QCOMPARE(occupied.readAll(), QByteArray("preserve"));
        const QString cancelledPath = base + "/cancelled";
        CExtractDialog cancelled(cancelledPath, {});
        cancelled.reject();
        QCOMPARE(cancelled.result(), int(QDialog::Rejected));
        QVERIFY(!QFileInfo::exists(cancelledPath));
    }
};
QTEST_MAIN(SplitDestinationTests)
#include "split_destination_tests.moc"
