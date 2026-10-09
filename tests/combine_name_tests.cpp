#include "window_state_support.h"
#include <QDialogButtonBox>
#include <QPushButton>

class CombineNameTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void rejectsInvalidSuffixAndUsesContiguousPrefix() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString error;
        for (const QString &name : {"part.1", "part.002", "part10", "part"}) {
            QVERIFY(VolumeOperation::discoverParts(dir.filePath(name), &error).isEmpty());
            QVERIFY(error.contains("Cannot detect file"));
        }
        for (const QString &name : {"part01", "part02", "part04"}) {
            QFile file(dir.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("data"), qint64(4));
        }
        QCOMPARE(VolumeOperation::discoverParts(dir.filePath("part01"), &error),
                 (QStringList{dir.filePath("part01"), dir.filePath("part02")}));
    }
    void upstreamFirstPartNames_data() {
        QTest::addColumn<QString>("first");
        QTest::addColumn<QString>("second");
        QTest::addColumn<QString>("output");
        QTest::newRow("without-dot") << "part01" << "part02" << "part";
        QTest::newRow("two-digits") << "part.01" << "part.02" << "part";
        QTest::newRow("four-digits") << "part0001" << "part0002" << "part";
        QTest::newRow("only-number") << ".001" << ".002" << "file";
        QTest::newRow("trailing-dots") << "part...001" << "part...002" << "part";
        QTest::newRow("unicode") << QStringLiteral("часть с пробелом01")
                                 << QStringLiteral("часть с пробелом02")
                                 << QStringLiteral("часть с пробелом");
    }
    void upstreamFirstPartNames() {
        QFETCH(QString, first);
        QFETCH(QString, second);
        QFETCH(QString, output);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        for (const QString &name : {first, second}) {
            QFile file(dir.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(name == first ? "first" : "second"),
                     name == first ? qint64(5) : qint64(6));
        }
        QString error;
        const auto parts = VolumeOperation::inspectParts(dir.filePath(first), &error);
        QCOMPARE(parts.paths, (QStringList{dir.filePath(first), dir.filePath(second)}));
        QCOMPARE(parts.totalSize, qint64(11));
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(dir.path());
        QTest::keyClick(address, Qt::Key_Return);
        int row = -1;
        for (int index = 0; index < table(window)->rowCount(); ++index)
            if (table(window)->item(index, 0)->text() == first)
                row = index;
        QVERIFY(row >= 0);
        table(window)->selectRow(row);
        auto *combine = action(window, "combineFilesAction");
        QVERIFY(combine->isEnabled());
        window.show();
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("combineDialog");
            if (!dialog)
                return;
            if (first == ".001" &&
                qEnvironmentVariableIntValue("SEVENZIP_CAPTURE_SCREENSHOTS") == 1) {
                const QString screenshots = QDir::currentPath() + "/ui-screenshots";
                QDir().mkpath(screenshots);
                for (int width : {540, 360}) {
                    dialog->resize(width, 340);
                    QApplication::processEvents();
                    QVERIFY(dialog->grab().save(
                        screenshots + QStringLiteral("/combine-names-%1.png").arg(width)));
                }
            }
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        });
        combine->trigger();
        const QString resultPath = dir.filePath(output);
        QTRY_VERIFY(QFileInfo::exists(resultPath));
        QFile result(resultPath);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("firstsecond"));
        QVERIFY(QFileInfo::exists(dir.filePath(first)));
        QVERIFY(QFileInfo::exists(dir.filePath(second)));
        QVERIFY(QDir(dir.path()).entryList({"*.sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CombineNameTests)
#include "combine_name_tests.moc"
