#include "CPP/7zip/UI/FileManager/CopyDialog.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "layout_inspection.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

class VolumeDialogTests : public QObject {
    Q_OBJECT
  private slots:
    void splitInitAndConfirmation_data() {
        QTest::addColumn<bool>("viaButton");
        QTest::newRow("accept") << false;
        QTest::newRow("button") << true;
    }
    void splitInitAndConfirmation() {
        QFETCH(bool, viaButton);
        QTemporaryDir temp;
        CSplitDialog dialog(QStringLiteral("файл с пробелом.bin"), temp.path());
        QVERIFY(dialog.windowTitle().endsWith("файл с пробелом.bin"));
        auto *path = dialog.findChild<QComboBox *>("volumeDestination");
        auto *volume = dialog.findChild<QComboBox *>("volumeSize");
        QVERIFY(path && volume);
        QCOMPARE(path->currentText(), temp.path());
        QCOMPARE(volume->currentText(), QStringLiteral("10M"));
        QCOMPARE(volume->count(), 9);
        path->setCurrentText(temp.filePath(QStringLiteral("новая папка")));
        volume->setCurrentText(" 10M 20M ");
        if (viaButton)
            dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        else
            dialog.accept();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(dialog.Path, path->currentText());
        QCOMPARE(dialog.VolumeSizes.Size(), 2u);
        QCOMPARE(dialog.VolumeSizes[0], UInt64(10) << 20);
        QCOMPARE(dialog.VolumeSizes[1], UInt64(20) << 20);
        QVERIFY(!QFileInfo::exists(dialog.Path));
    }

    void splitRejectsInvalidSizesAndCancelDoesNotCreateDirectory() {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("не создана"));
        CSplitDialog dialog("source.bin", path);
        dialog.show();
        auto *volume = dialog.findChild<QComboBox *>("volumeSize");
        for (const QString &bad : {QString(), QString("0"), QString("10M 0M"),
                                   QString("9223372036854775808"), QString("wrong")}) {
            volume->setCurrentText(bad);
            dialog.accept();
            QVERIFY(dialog.isVisible());
            QVERIFY(dialog.findChild<QLabel *>("volumeError")->isVisible());
            QVERIFY(!QFileInfo::exists(path));
        }
        dialog.reject();
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
        QVERIFY(!QFileInfo::exists(path));
    }

    void splitGeometry_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<bool>("invalid");
        for (int width : {480, 340})
            for (bool invalid : {false, true})
                QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(width).arg(invalid)))
                    << width << invalid;
    }
    void splitGeometry() {
        QFETCH(int, width);
        QFETCH(bool, invalid);
        CSplitDialog dialog("source.bin", "/temporary/destination");
        auto *path = dialog.findChild<QComboBox *>("volumeDestination");
        auto *volume = dialog.findChild<QComboBox *>("volumeSize");
        auto *browse = dialog.findChild<QPushButton *>("volumeBrowse");
        auto *pathLabel = dialog.findChild<QLabel *>("splitPathLabel");
        auto *volumeLabel = dialog.findChild<QLabel *>("splitVolumeLabel");
        auto *buttons = dialog.findChild<QDialogButtonBox *>("splitButtons");
        if (invalid) {
            volume->setCurrentText("wrong");
            dialog.accept();
        }
        dialog.resize(width, 240);
        dialog.show();
        QApplication::processEvents();
        QVERIFY(UiInspection::fullyVisible(*path, dialog));
        QVERIFY(UiInspection::fullyVisible(*volume, dialog));
        QVERIFY(UiInspection::fullyVisible(*browse, dialog));
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::labelFits(*pathLabel));
        QVERIFY(UiInspection::labelFits(*volumeLabel));
        QVERIFY(UiInspection::bounds(*pathLabel, dialog).bottom() <
                UiInspection::bounds(*path, dialog).top());
        QVERIFY(UiInspection::bounds(*path, dialog).bottom() <
                UiInspection::bounds(*volumeLabel, dialog).top());
        QVERIFY(UiInspection::bounds(*volumeLabel, dialog).bottom() <
                UiInspection::bounds(*volume, dialog).top());
        QVERIFY(UiInspection::separate(*path, *browse, dialog));
        QVERIFY(UiInspection::separate(*buttons->button(QDialogButtonBox::Ok),
                                       *buttons->button(QDialogButtonBox::Cancel), dialog));
        if (invalid) {
            auto *error = dialog.findChild<QLabel *>("volumeError");
            QVERIFY(UiInspection::fullyVisible(*error, dialog));
            QVERIFY(UiInspection::labelFits(*error));
            QVERIFY(UiInspection::separate(*error, *buttons, dialog));
        }
        QVERIFY(UiInspection::writeReport(
            dialog, QStringLiteral("split-%1-%2").arg(width).arg(invalid ? "error" : "normal")));
    }

    void combineUsesCopyDialogAndUpstreamSummary_data() {
        QTest::addColumn<int>("count");
        QTest::newRow("two") << 2;
        QTest::newRow("three") << 3;
        QTest::newRow("many") << 5;
    }
    void combineUsesCopyDialogAndUpstreamSummary() {
        QFETCH(int, count);
        CThreadCombine combiner;
        combiner.InputDirPrefix = "/parts/";
        combiner.TotalSize = 1234;
        for (int i = 1; i <= count; ++i)
            combiner.Names << QStringLiteral("часть с пробелом.%1").arg(i, 3, 10, QLatin1Char('0'));
        const QString info = combiner.GetInfo();
        QVERIFY(info.contains(QStringLiteral("Files: %1").arg(count)));
        QVERIFY(info.contains("1 234 bytes"));
        QVERIFY(info.contains("\n/parts/"));
        QVERIFY(info.contains(combiner.Names.first()));
        QVERIFY(info.contains(combiner.Names[1]));
        QVERIFY(info.contains(combiner.Names.last()));
        QCOMPARE(info.contains("..."), count > 3);
        if (count == 5)
            QVERIFY(!info.contains(combiner.Names[2]));
        CCopyDialog dialog("Объединить файлы часть.001", "Объединить в:", "/output", info);
        auto *summary = dialog.findChild<QPlainTextEdit *>("copySelectionInfo");
        QCOMPARE(summary->toPlainText(), info);
        QVERIFY(summary->isReadOnly());
        for (int width : {480, 340}) {
            dialog.resize(width, 250);
            dialog.show();
            QApplication::processEvents();
            auto *buttons = dialog.findChild<QDialogButtonBox *>("copyButtons");
            QVERIFY(UiInspection::fullyVisible(*summary, dialog));
            QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
            QVERIFY(UiInspection::separate(*summary, *buttons, dialog));
            QVERIFY(UiInspection::writeReport(
                dialog, QStringLiteral("combine-%1-%2").arg(count).arg(width)));
        }
        dialog.findChild<QComboBox *>("copyDestination")->setCurrentText("/new destination");
        dialog.accept();
        QCOMPARE(dialog.Value, QStringLiteral("/new destination"));
    }
};
QTEST_MAIN(VolumeDialogTests)
#include "volume_dialog_tests.moc"
