#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/GUI/BenchmarkDialog.h"
#include "benchmark_services_fixture.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QtTest>

class BenchmarkLifecycleTests final : public QObject {
    Q_OBJECT
    static CObjectVector<CProperty> properties(bool total = false) {
        CObjectVector<CProperty> props;
        for (const auto &pair : {qMakePair(L"mt", L"1"), qMakePair(L"d18", L"")}) {
            CProperty &prop = props.AddNew();
            prop.Name = pair.first;
            prop.Value = pair.second;
        }
        if (total) {
            CProperty &prop = props.AddNew();
            prop.Name = "m";
            prop.Value = "*";
        }
        return props;
    }
    static QDialog *dialog() {
        auto *view = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        return view && view->objectName().startsWith("IDD_BENCH") ? view : nullptr;
    }

  private slots:
    void cleanup() {
        SaveRegLang(UString("-"), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
    void layoutAndLiveLanguage_data() {
        QTest::addColumn<bool>("total");
        QTest::newRow("normal") << false;
        QTest::newRow("total") << true;
    }
    void layoutAndLiveLanguage() {
        QFETCH(bool, total);
        FixtureBenchmarkServices service;
        service.Hold = true;
        bool inspected = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *view = dialog();
            if (!view || !service.Active || inspected)
                return;
            inspected = true;
            struct CloseOnExit {
                QDialog *View;
                ~CloseOnExit() { View->reject(); }
            } close{view};
            for (const char *language : {"-", "ru", "-"}) {
                SaveRegLang(UString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
                ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
                QtNotifyLanguageChanged();
                QCOMPARE(view->windowTitle(),
                         QString(language) == "ru"
                             ? QStringLiteral("Тестирование производительности")
                             : QString("Benchmark"));
                if (!total)
                    QCOMPARE(view->findChild<QLabel *>("IDT_BENCH_SPEED")->text(),
                             QString(language) == "ru" ? QStringLiteral("Скорость")
                                                       : QString("Speed"));
                for (const QSize size : {QSize(960, 650), QSize(640, 400)}) {
                    view->resize(size);
                    QApplication::processEvents();
                    for (const char *name : {"IDHELP", "IDCANCEL"}) {
                        auto *button = view->findChild<QPushButton *>(name);
                        QVERIFY(UiInspection::fullyVisible(*button, *view));
                        QVERIFY(button->width() >= button->sizeHint().width());
                        if (QString(name) == "IDCANCEL")
                            QCOMPARE(button->text(), QString(language) == "ru"
                                                         ? QStringLiteral("Отмена")
                                                         : QString("Cancel"));
                    }
                    if (total) {
                        QVERIFY(UiInspection::fullyVisible(
                            *view->findChild<QPlainTextEdit *>("IDE_BENCH2_EDIT"), *view));
                    } else {
                        auto *scroll = view->findChild<QScrollArea *>("benchmarkControlsScroll");
                        for (QWidget *control : scroll->widget()->findChildren<QWidget *>()) {
                            if (control->objectName().isEmpty() || !control->isVisible())
                                continue;
                            scroll->ensureWidgetVisible(control, 5, 10);
                            QApplication::processEvents();
                            QVERIFY2(UiInspection::fullyVisible(*control, *view),
                                     qPrintable(control->objectName()));
                            if (qobject_cast<QLabel *>(control)) {
                                if (!UiInspection::labelFits(*control))
                                    QVERIFY(UiInspection::writeReport(*view,
                                                                      "benchmark-clipped-label"));
                                QVERIFY2(UiInspection::labelFits(*control),
                                         qPrintable(control->objectName()));
                            }
                        }
                        scroll->verticalScrollBar()->setValue(0);
                    }
                    QVERIFY(UiInspection::writeReport(*view, QString("benchmark-%1-%2-%3")
                                                                 .arg(total ? "total" : "normal")
                                                                 .arg(language)
                                                                 .arg(size.width())));
                }
            }
            view->findChild<QPushButton *>("IDHELP")->click();
            QApplication::processEvents();
            QTextBrowser *help = nullptr;
            for (auto *window : QApplication::topLevelWidgets())
                if (auto *browser = window->findChild<QTextBrowser *>("helpBrowser")) {
                    help = browser;
                    break;
                }
            QVERIFY(help);
            QCOMPARE(help->source().path(), QString("/upstream/7zip/Help/fm/benchmark.htm"));
            help->window()->close();
            view->reject();
        });
        responder.start(1);
        QCOMPARE(Benchmark(nullptr, properties(total), 1, nullptr, &service), S_OK);
        QVERIFY(inspected);
        QCOMPARE(service.Active.load(), 0u);
    }
    void fixedSourceMetricsAndTotalConsole_data() {
        QTest::addColumn<bool>("total");
        QTest::newRow("normal") << false;
        QTest::newRow("total") << true;
    }
    void fixedSourceMetricsAndTotalConsole() {
        QFETCH(bool, total);
        FixtureBenchmarkServices service;
        QString size, speed, usage, console;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *view = dialog();
            if (!view || service.Active || service.Calls == 0)
                return;
            if (total) {
                console = view->findChild<QPlainTextEdit *>("IDE_BENCH2_EDIT")->toPlainText();
                if (!console.contains("Fixture native benchmark output"))
                    return; // Worker completion precedes its queued GUI delivery.
            } else {
                if (view->findChild<QPushButton *>("IDB_STOP")->isEnabled())
                    return;
                size = view->findChild<QLabel *>("benchmarkValue171")->text();
                speed = view->findChild<QLabel *>("benchmarkValue111")->text();
                usage = view->findChild<QLabel *>("IDT_BENCH_TOTAL_USAGE_VAL")->text();
            }
            view->reject();
        });
        responder.start(1);
        QCOMPARE(Benchmark(nullptr, properties(total), 1, nullptr, &service), S_OK);
        QCOMPARE(service.Calls.load(), 1u);
        QCOMPARE(service.Active.load(), 0u);
        if (total)
            QVERIFY(console.contains("Fixture native benchmark output"));
        else {
            QCOMPARE(size, QString("1 MB"));
            QCOMPARE(speed, QString("1024 KB/s"));
            QCOMPARE(usage, QString("50%"));
        }
    }
    void restartThenStopAndCloseJoinBothWorkers() {
        FixtureBenchmarkServices service;
        service.Hold = true;
        bool restarted = false, stopped = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *view = dialog();
            if (!view)
                return;
            if (service.Calls == 1 && !restarted) {
                restarted = true;
                view->findChild<QPushButton *>("IDB_RESTART")->click();
            } else if (service.Calls >= 2 && !stopped) {
                stopped = true;
                view->findChild<QPushButton *>("IDB_STOP")->click();
                view->reject();
            }
        });
        responder.start(1);
        QCOMPARE(Benchmark(nullptr, properties(), 1, nullptr, &service), S_OK);
        QVERIFY(restarted && stopped);
        QCOMPARE(service.Calls.load(), 2u);
        QCOMPARE(service.Canceled.load(), 2u);
        QCOMPARE(service.Active.load(), 0u);
        QVERIFY(QApplication::activeModalWidget() == nullptr);
    }
    void memoryRefusalAndEngineFailure_data() {
        QTest::addColumn<bool>("memory");
        QTest::newRow("memory-refusal") << true;
        QTest::newRow("engine-failure") << false;
    }
    void memoryRefusalAndEngineFailure() {
        QFETCH(bool, memory);
        FixtureBenchmarkServices service;
        if (memory)
            service.Ram = size_t(1) << 20;
        else
            service.Result = E_FAIL;
        QStringList errors;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors << box->text();
                box->accept();
            } else if (auto *view = dialog(); view && !errors.isEmpty())
                view->reject();
        });
        responder.start(1);
        QCOMPARE(Benchmark(nullptr, properties(), 1, nullptr, &service), S_OK);
        QVERIFY(!errors.isEmpty());
        QCOMPARE(service.Calls.load(), memory ? 0u : 1u);
        QCOMPARE(service.Active.load(), 0u);
    }
};
QTEST_MAIN(BenchmarkLifecycleTests)
#include "benchmark_lifecycle_tests.moc"
