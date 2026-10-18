#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "layout_inspection.h"
#include "processpriority.h"
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>

namespace {
QString text(const CProgressDialog &dialog, const char *name) {
    return dialog.findChild<QLabel *>(name)->text();
}
QPushButton *pause(CProgressDialog &dialog) { return dialog.findChild<QPushButton *>("IDB_PAUSE"); }
QTreeWidget *messages(CProgressDialog &dialog) {
    return dialog.findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES");
}
} // namespace

class ProgressDialogTests : public QObject {
    Q_OBJECT

  private slots:
    void sourceIconIDAndEmbeddedFrames() {
        CProgressDialog normal;
        const auto originalIcon = normal.windowIcon().cacheKey();
        normal.show();
        QCOMPARE(normal.windowIcon().cacheKey(), originalIcon);
        CProgressDialog source;
        source.IconID = IDI_ICON;
        source.show();
        QVERIFY(!source.windowIcon().isNull());
        for (const int size : {16, 32, 48}) {
            QVERIFY(source.windowIcon().availableSizes().contains(QSize(size, size)));
            // Inspect the embedded 1x frame; the default overload can return
            // a Retina pixmap with twice the requested logical dimensions.
            const QPixmap frame = source.windowIcon().pixmap(QSize(size, size), 1.0);
            QVERIFY(!frame.isNull());
            QCOMPARE(frame.size(), QSize(size, size));
        }
        CProgressDialog invalid;
        invalid.IconID = 99999;
        invalid.show();
        QVERIFY(invalid.windowIcon().isNull());
        CProgressDialog second;
        second.IconID = IDI_ICON;
        second.show();
        QVERIFY(!second.windowIcon().isNull());
    }
    void initialUnknownStateAndSourceCommands() {
        CProgressDialog dialog(nullptr, L"Обновление");
        QVERIFY(dialog.windowFlags().testFlag(Qt::WindowMinimizeButtonHint));
        QVERIFY(dialog.windowFlags().testFlag(Qt::WindowMaximizeButtonHint));
        QVERIFY(!dialog.windowFlags().testFlag(Qt::WindowContextHelpButtonHint));
        dialog.show();
        QTRY_COMPARE(text(dialog, "IDT_PROGRESS_ELAPSED_VAL"), QString("00:00:00"));
        QCOMPARE(text(dialog, "IDT_PROGRESS_TOTAL_VAL"), QString());
        QCOMPARE(text(dialog, "IDT_PROGRESS_REMAINING_VAL"), QString());
        QCOMPARE(text(dialog, "IDT_PROGRESS_PACKED_VAL"), QString());
        QCOMPARE(text(dialog, "IDT_PROGRESS_FILES_VAL"), QString("0"));
        QCOMPARE(text(dialog, "IDT_PROGRESS_FILES_TOTAL"), QString());
        QCOMPARE(text(dialog, "IDT_PROGRESS_PROCESSED_VAL"), QString("0"));
        QVERIFY(!messages(dialog)->isVisible());
        bool background;
        QCOMPARE(dialog.findChild<QPushButton *>("IDB_PROGRESS_BACKGROUND")->isEnabled(),
                 NProcessPriority::GetBackground(0, background) == S_OK);
        QVERIFY(dialog.findChild<QPushButton *>("IDCANCEL")->isEnabled());
        QVERIFY(pause(dialog)->isEnabled());
        dialog.accept();
        QTimer::singleShot(0, &dialog, [] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                box->button(QMessageBox::No)->click();
        });
        dialog.reject();
        QVERIFY(dialog.isVisible());
        QVERIFY(!dialog.Sync.Get_Stopped());
    }
    void countersRatioTitleAndUnicodePath() {
        QWidget main;
        main.setWindowTitle("7-Zip");
        {
            CProgressDialog dialog(nullptr, L"Обновление");
            dialog.MainWindow = &main;
            dialog.MainTitle = L"7-Zip";
            dialog.MainAddTitle = L"Архив ";
            dialog.Sync.Set_NumBytesTotal(4000);
            QCOMPARE(dialog.Sync.Set_NumBytesCur(2000), S_OK);
            QCOMPARE(dialog.Sync.Set_NumFilesTotal(4), S_OK);
            dialog.Sync.Set_NumFilesCur(2);
            UInt64 in = 2000, out = 500;
            dialog.Sync.Set_Ratio(&in, &out);
            dialog.Sync.Set_TitleFileName(L"архив с пробелом.7z");
            dialog.Sync.Set_Status(L"Добавление");
            dialog.Sync.Set_FilePath(L"/папка с пробелом/файл.txt");
            dialog.show();
            QTRY_COMPARE(text(dialog, "IDT_PROGRESS_TOTAL_VAL"), QString("4000"));
            QTRY_COMPARE(text(dialog, "IDT_PROGRESS_RATIO_VAL"), QString("25%"));
            QCOMPARE(text(dialog, "IDT_PROGRESS_PROCESSED_VAL"), QString("2000"));
            QCOMPARE(text(dialog, "IDT_PROGRESS_PACKED_VAL"), QString("500"));
            QCOMPARE(text(dialog, "IDT_PROGRESS_FILES_VAL"), QString("2"));
            QCOMPARE(text(dialog, "IDT_PROGRESS_FILES_TOTAL"), QString(" / 4"));
            QCOMPARE(text(dialog, "IDT_PROGRESS_STATUS"), QStringLiteral("Добавление"));
            QCOMPARE(text(dialog, "IDT_PROGRESS_FILE_NAME"),
                     QStringLiteral("/папка с пробелом/\nфайл.txt"));
            QCOMPARE(dialog.windowTitle(), QStringLiteral("50% Обновление архив с пробелом.7z"));
            QCOMPARE(main.windowTitle(), QStringLiteral("50% Архив 7-Zip"));
            QVERIFY(UiInspection::writeReport(dialog, "progress-counters"));
            dialog.Sync.Set_FilePath(L"/папка с пробелом/", true);
            QTRY_COMPARE(text(dialog, "IDT_PROGRESS_FILE_NAME"),
                         QStringLiteral("/папка с пробелом/\n"));
            dialog.Sync.Set_NumBytesTotal(~UInt64(0));
            QTRY_COMPARE(text(dialog, "IDT_PROGRESS_REMAINING_VAL"), QString());
            QTRY_COMPARE(text(dialog, "IDT_PROGRESS_TOTAL_VAL"), QString());
            QTRY_VERIFY(dialog.windowTitle().startsWith("0% "));
        }
        QCOMPARE(main.windowTitle(), QString("7-Zip"));
    }
    void displayedSizeUnits_data() {
        QTest::addColumn<quint64>("bytes");
        QTest::addColumn<QString>("expected");
        QTest::newRow("zero") << quint64(0) << QString("0");
        QTest::newRow("last-byte-unit") << quint64(99999) << QString("99999");
        QTest::newRow("first-KB") << quint64(100000) << QString("97 KB");
        QTest::newRow("last-KB") << quint64((100000ULL << 10) - 1) << QString("99999 KB");
        QTest::newRow("first-MB") << quint64(100000ULL << 10) << QString("97 MB");
        QTest::newRow("last-MB") << quint64((100000ULL << 20) - 1) << QString("99999 MB");
        QTest::newRow("first-GB") << quint64(100000ULL << 20) << QString("97 GB");
        QTest::newRow("largest-defined") << quint64(~UInt64(0) - 1) << QString("17179869183 GB");
        QTest::newRow("unknown") << quint64(~UInt64(0)) << QString();
    }
    void displayedSizeUnits() {
        QFETCH(quint64, bytes);
        QFETCH(QString, expected);
        CProgressDialog dialog;
        QCOMPARE(dialog.Sync.Set_NumBytesCur(bytes), S_OK);
        dialog.show();
        QTRY_COMPARE(text(dialog, "IDT_PROGRESS_PROCESSED_VAL"), expected);
    }
    void decompressingAndFilesProgress() {
        CProgressDialog dialog;
        dialog.CompressingMode = false;
        dialog.Sync.Set_FilesProgressMode(true);
        QCOMPARE(dialog.Sync.Set_NumFilesTotal(8), S_OK);
        dialog.Sync.Set_NumFilesCur(2);
        dialog.Sync.Set_NumBytesTotal(1000);
        QCOMPARE(dialog.Sync.Set_NumBytesCur(750), S_OK);
        UInt64 in = 100, out = 400;
        dialog.Sync.Set_Ratio(&in, &out);
        dialog.show();
        QTRY_VERIFY(dialog.windowTitle().startsWith("25% "));
        QCOMPARE(text(dialog, "IDT_PROGRESS_PROCESSED_VAL"), QString("400"));
        QCOMPARE(text(dialog, "IDT_PROGRESS_PACKED_VAL"), QString("100"));
        QCOMPARE(text(dialog, "IDT_PROGRESS_RATIO_VAL"), QString("25%"));
        auto *bar = dialog.findChild<QProgressBar *>("IDC_PROGRESS1");
        QCOMPARE(bar->maximum(), 9);
        QCOMPARE(bar->value(), 2);
    }
    void sourceRangeConverterAndBackwardProgress_data() {
        QTest::addColumn<quint64>("total");
        QTest::newRow("zero") << quint64(0);
        QTest::newRow("small") << quint64(32767);
        QTest::newRow("larger") << quint64(32768);
        QTest::newRow("large-64-bit") << quint64(1ULL << 63);
        QTest::newRow("largest-defined") << quint64(~UInt64(0) - 1);
    }
    void sourceRangeConverterAndBackwardProgress() {
        QFETCH(quint64, total);
        CProgressDialog dialog;
        dialog.Sync.Set_NumBytesTotal(total);
        QCOMPARE(dialog.Sync.Set_NumBytesCur(total / 2), S_OK);
        dialog.show();
        auto *bar = dialog.findChild<QProgressBar *>("IDC_PROGRESS1");
        QTRY_VERIFY(bar->maximum() > 0);
        if (total)
            QTRY_VERIFY(qAbs(double(bar->value()) / bar->maximum() - 0.5) < 0.001);
        else
            QTRY_COMPARE(bar->value(), bar->maximum());
        QCOMPARE(dialog.Sync.Set_NumBytesCur(total), S_OK);
        QTRY_COMPARE(bar->value(), bar->maximum());
        QCOMPARE(dialog.Sync.Set_NumBytesCur(UInt64(0)), S_OK);
        if (total != 0)
            QTRY_COMPARE(bar->value(), 0);
    }
    void pauseStopsRenderingAndPreservesElapsedTime() {
        CProgressDialog dialog(nullptr, L"Обновление");
        dialog.Sync.Set_NumBytesTotal(100);
        QCOMPARE(dialog.Sync.Set_NumBytesCur(25), S_OK);
        dialog.show();
        QTRY_VERIFY(dialog.windowTitle().startsWith("25% "));
        const QString elapsed = text(dialog, "IDT_PROGRESS_ELAPSED_VAL");
        pause(dialog)->click();
        QVERIFY(dialog.Sync.Get_Paused());
        QCOMPARE(pause(dialog)->text(), QStringLiteral("&Continue"));
        QVERIFY(dialog.windowTitle().startsWith(QStringLiteral("Paused 25% ")));
        dialog.Sync.Set_Status(L"Пока приостановлено");
        // Counter setters that wait on pause belong to the worker; status does not wait.
        QTest::qWait(1100);
        QCOMPARE(text(dialog, "IDT_PROGRESS_STATUS"), QString());
        QCOMPARE(text(dialog, "IDT_PROGRESS_ELAPSED_VAL"), elapsed);
        pause(dialog)->click();
        QVERIFY(!dialog.Sync.Get_Paused());
        QCOMPARE(pause(dialog)->text(), QStringLiteral("&Pause"));
        QTRY_COMPARE(text(dialog, "IDT_PROGRESS_STATUS"), QStringLiteral("Пока приостановлено"));
        QCOMPARE(text(dialog, "IDT_PROGRESS_ELAPSED_VAL"), elapsed);
    }
    void nativeProgressControlClampsOutOfRangePosition() {
        CProgressDialog dialog;
        dialog.Sync.Set_NumBytesTotal(100);
        QCOMPARE(dialog.Sync.Set_NumBytesCur(25), S_OK);
        dialog.show();
        auto *bar = dialog.findChild<QProgressBar *>("IDC_PROGRESS1");
        QTRY_COMPARE(bar->value(), 25);
        QCOMPARE(dialog.Sync.Set_NumBytesCur(150), S_OK);
        QTRY_COMPARE(bar->value(), bar->maximum());
        QCOMPARE(dialog.Sync.Set_NumBytesCur(UInt64(0)), S_OK);
        QTRY_COMPARE(bar->value(), 0);
    }
    void errorLinesAppendNumberingAndClipboard() {
        CProgressDialog dialog;
        dialog.Sync.AddError_Message_Name(L"Ошибка\nПодробность", L"файл с пробелом.txt");
        dialog.Sync.AddError_Message(L"");
        dialog.show();
        auto *list = messages(dialog);
        QTRY_COMPARE(list->topLevelItemCount(), 4);
        QVERIFY(list->isVisible());
        QCOMPARE(text(dialog, "IDT_PROGRESS_ERRORS_VAL"), QString("2"));
        QCOMPARE(list->topLevelItem(0)->text(0), QString("1"));
        QCOMPARE(list->topLevelItem(0)->text(1), QStringLiteral("файл с пробелом.txt"));
        QCOMPARE(list->topLevelItem(1)->text(0), QString());
        QCOMPARE(list->topLevelItem(2)->text(1), QStringLiteral("Подробность"));
        QCOMPARE(list->topLevelItem(3)->text(0), QString("2"));
        list->setFocus();
        list->clearSelection();
        const QString newline =
#ifdef _WIN32
            "\r\n";
#else
            "\n";
#endif
        QTest::keyClick(list, Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->text(),
                 QStringLiteral("файл с пробелом.txt") + newline + QStringLiteral("Ошибка") +
                     newline + QStringLiteral("Подробность") + newline + newline);
        list->topLevelItem(1)->setSelected(true);
        QTest::keyClick(list, Qt::Key_Insert, Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("Ошибка") + newline);
        QTest::keyClick(list, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(list->selectedItems().size(), 4);
        dialog.Sync.AddError_Message(L"Ещё ошибка\n");
        QTRY_COMPARE(list->topLevelItemCount(), 6);
        QCOMPARE(text(dialog, "IDT_PROGRESS_ERRORS_VAL"), QString("3"));
        QCOMPARE(list->topLevelItem(4)->text(0), QString("3"));
        QCOMPARE(list->topLevelItem(5)->text(1), QString());
    }
    void sourceLayoutStates_data() {
        QTest::addColumn<QSize>("size");
        QTest::addColumn<bool>("compression");
        QTest::addColumn<bool>("errors");
        for (const int width : {760, 640})
            for (const bool compression : {true, false})
                for (const bool errors : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(width).arg(compression).arg(errors)))
                        << QSize(width, width == 760 ? 450 : 380) << compression << errors;
    }
    void sourceLayoutStates() {
        QFETCH(QSize, size);
        QFETCH(bool, compression);
        QFETCH(bool, errors);
        CProgressDialog dialog(nullptr, L"Обновление");
        dialog.ShowCompressionInfo = compression;
        if (errors)
            dialog.Sync.AddError_Message(L"Ошибка файла с пробелом");
        dialog.resize(size);
        dialog.show();
        if (errors)
            QTRY_VERIFY(messages(dialog)->isVisible());
        QApplication::processEvents();
        QCOMPARE(dialog.size(), size.expandedTo(dialog.minimumSizeHint()));
        const QString tag =
            QString("progress-dialog-%1-%2-%3").arg(size.width()).arg(compression).arg(errors);
        QVERIFY(UiInspection::writeReport(dialog, tag + "-collapsed"));
        auto *details = dialog.findChild<QToolButton *>("progressDetailsToggle");
        auto *total = dialog.findChild<QLabel *>("IDT_PROGRESS_TOTAL_VAL");
        QVERIFY(!total->isVisible());
        details->click();
        QApplication::processEvents();
        auto *scroll = dialog.findChild<QScrollArea *>("progressControlsScroll");
        for (QWidget *widget : dialog.findChildren<QWidget *>()) {
            if (!widget->isVisible() || (widget->objectName().isEmpty()))
                continue;
            if (!qobject_cast<const QLabel *>(widget) && !qobject_cast<const QPushButton *>(widget))
                continue;
            scroll->ensureWidgetVisible(widget);
            QApplication::processEvents();
            QVERIFY2(UiInspection::fullyVisible(*widget, dialog), qPrintable(widget->objectName()));
            if (scroll->isAncestorOf(widget) && scroll->verticalScrollBar()->isVisible())
                QVERIFY2(UiInspection::separate(*widget, *scroll->verticalScrollBar(), dialog),
                         qPrintable(widget->objectName()));
            if (qobject_cast<const QLabel *>(widget) &&
                !text(dialog, qPrintable(widget->objectName())).isEmpty())
                QVERIFY2(UiInspection::labelFits(*widget), qPrintable(widget->objectName()));
            if (qobject_cast<const QPushButton *>(widget)) {
                QVERIFY2(widget->sizeHint().width() <= widget->width(),
                         qPrintable(widget->objectName()));
                QVERIFY2(widget->sizeHint().height() <= widget->height(),
                         qPrintable(widget->objectName()));
            }
        }
        auto *bar = dialog.findChild<QProgressBar *>("IDC_PROGRESS1");
        auto *file = dialog.findChild<QLabel *>("IDT_PROGRESS_FILE_NAME");
        QVERIFY(bar->geometry().bottom() < file->geometry().top());
        auto *background = dialog.findChild<QPushButton *>("IDB_PROGRESS_BACKGROUND");
        auto *cancel = dialog.findChild<QPushButton *>("IDCANCEL");
        QVERIFY(background->geometry().right() < pause(dialog)->geometry().left());
        QVERIFY(pause(dialog)->geometry().right() < cancel->geometry().left());
        if (errors)
            QVERIFY(UiInspection::bounds(*messages(dialog), dialog).bottom() <
                    UiInspection::bounds(*cancel, dialog).top());
        QCOMPARE(dialog.findChild<QLabel *>("IDT_PROGRESS_PACKED")->isVisible(), compression);
        QCOMPARE(dialog.findChild<QLabel *>("IDT_PROGRESS_RATIO")->isVisible(), compression);
        QCOMPARE(dialog.findChild<QLabel *>("IDT_PROGRESS_ERRORS")->isVisible(), errors);
        QVERIFY(UiInspection::writeReport(dialog, tag));
        pause(dialog)->click();
        QCOMPARE(pause(dialog)->text(), QStringLiteral("&Continue"));
        QTRY_VERIFY(pause(dialog)->sizeHint().width() <= pause(dialog)->width());
        QVERIFY(dialog.windowTitle().startsWith(QStringLiteral("Paused ")));
        QVERIFY(UiInspection::writeReport(dialog, tag + "-paused"));
        details->click();
        QVERIFY(!total->isVisible());
        pause(dialog)->click();
        dialog.Sync.Set_NumBytesTotal(2048);
        QTRY_COMPARE(total->text(), QString("2048"));
        details->click();
        QVERIFY(total->isVisible());
        QCOMPARE(total->text(), QString("2048"));
    }
};

#include "review_test_main.h"
SEVENZIP_UI_TEST_MAIN(ProgressDialogTests)
#include "progress_dialog_tests.moc"
