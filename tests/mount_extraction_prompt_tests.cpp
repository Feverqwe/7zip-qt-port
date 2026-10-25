#include "layout_inspection.h"
#include "qtarchivemountmanager.h"
#include "qtmountextractionprompt.h"
#include "qtmountsettings.h"
#include <QLabel>
#include <QPushButton>
#include <QScopeGuard>
#include <QTranslator>
#include <QtTest>
#include <atomic>
#include <future>

class MountExtractionPromptTests final : public QObject {
    Q_OBJECT
  private slots:
    void managerActionsGeometry_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<QString>("language");
        for (const QString &language : {QStringLiteral("en"), QStringLiteral("ru")})
            for (int width : {640, 780})
                QTest::newRow(qPrintable(QStringLiteral("%1 %2").arg(language).arg(width)))
                    << width << language;
    }
    void managerActionsGeometry() {
        QFETCH(int, width);
        QFETCH(QString, language);
        QTranslator catalog;
        if (language == "ru") {
            QVERIFY(catalog.load(":/translations/sevenzip_ru.qm"));
            qApp->installTranslator(&catalog);
        }
        QWidget owner;
        QtArchiveMountManager manager(&owner);
        // Exercise the width needed when the busy-mount force action is
        // exposed, without requiring a privileged OS mount for layout checks.
        const auto buttons = manager.findChildren<QPushButton *>();
        for (auto *button : buttons)
            button->show();
        manager.resize(width, 400);
        manager.show();
        QTest::qWait(10);
        QVERIFY(manager.width() <= width);
        for (const auto *button : buttons) {
            QVERIFY(button->isVisible());
            QVERIFY(UiInspection::fullyVisible(*button, manager));
        }
        for (int first = 0; first < buttons.size(); ++first)
            for (int second = first + 1; second < buttons.size(); ++second)
                QVERIFY(UiInspection::separate(*buttons[first], *buttons[second], manager));
        QVERIFY(UiInspection::writeReport(
            manager, QStringLiteral("mount-manager-actions-%1-%2").arg(language).arg(width)));
    }
    void decision_data() {
        QTest::addColumn<int>("action");
        QTest::addColumn<bool>("unverified");
        for (bool unverified : {false, true}) {
            const QString mode = unverified ? "recovery " : "extraction ";
            QTest::newRow(qPrintable(mode + "Return cancels")) << 0 << unverified;
            QTest::newRow(qPrintable(mode + "Escape cancels")) << 1 << unverified;
            QTest::newRow(qPrintable(mode + "Close cancels")) << 2 << unverified;
            QTest::newRow(qPrintable(mode + "explicit consent")) << 3 << unverified;
        }
    }
    void decision() {
        QFETCH(int, action);
        QFETCH(bool, unverified);
        const auto decision = std::make_shared<ArchiveMountDecision>();
        std::atomic_bool cancelled{false};
        auto answer = std::async(std::launch::async, [&] { return decision->wait(cancelled); });
        QtMountExtractionPrompt prompt("Synthetic image", "folder/данные 空 <file>.bin", 200000,
                                       decision, nullptr, unverified);
        prompt.show();
        QTest::qWait(10);
        if (action == 0)
            QTest::keyClick(&prompt, Qt::Key_Return);
        else if (action == 1)
            QTest::keyClick(&prompt, Qt::Key_Escape);
        else if (action == 2)
            prompt.close();
        else
            QTest::mouseClick(prompt.findChild<QPushButton *>("extractForMountButton"),
                              Qt::LeftButton);
        QElapsedTimer timer;
        timer.start();
        while (answer.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready &&
               timer.elapsed() < 1000)
            QTest::qWait(5);
        // Ensure a broken UI signal cannot leave the test's worker blocked.
        if (answer.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            cancelled = true;
        QVERIFY(!cancelled);
        QCOMPARE(answer.get(), action == 3);
    }
    void destructionCancels() {
        const auto decision = std::make_shared<ArchiveMountDecision>();
        {
            QtMountExtractionPrompt prompt("Synthetic image", "file.bin", 10, decision);
        }
        const std::atomic_bool cancelled{false};
        QVERIFY(!decision->wait(cancelled));
    }
    void geometry_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("unverified");
        for (bool unverified : {false, true})
            for (const QString &language : {QStringLiteral("en"), QStringLiteral("ru")})
                for (int width : {640, 420})
                    QTest::newRow(qPrintable(
                        QStringLiteral("%1 %2 %3").arg(language).arg(width).arg(unverified)))
                        << width << language << unverified;
    }
    void geometry() {
        QFETCH(int, width);
        QFETCH(QString, language);
        QFETCH(bool, unverified);
        const QLocale previous;
        const auto restoreLocale = qScopeGuard([&] { QLocale::setDefault(previous); });
        QLocale::setDefault(QLocale(language));
        QTranslator catalog;
        if (language == "ru") {
            QVERIFY(catalog.load(":/translations/sevenzip_ru.qm"));
            qApp->installTranslator(&catalog);
        }
        auto decision = std::make_shared<ArchiveMountDecision>();
        QtMountExtractionPrompt prompt(
            "Synthetic image with spaces.img",
            "folder/длинное имя с пробелами/данные 空 для внешнего приложения.bin", 200000,
            decision, nullptr, unverified);
        prompt.resize(width, prompt.sizeHint().height());
        prompt.show();
        QTest::qWait(10);
        QVERIFY(prompt.width() <= width);
        for (const auto *label : prompt.findChildren<QLabel *>()) {
            if (label->isVisible()) {
                QVERIFY(UiInspection::fullyVisible(*label, prompt));
                QVERIFY(UiInspection::labelFits(*label));
            }
        }
        const auto buttons = prompt.findChildren<QPushButton *>();
        for (const auto *button : buttons)
            QVERIFY(UiInspection::fullyVisible(*button, prompt));
        for (int first = 0; first < buttons.size(); ++first)
            for (int second = first + 1; second < buttons.size(); ++second)
                QVERIFY(UiInspection::separate(*buttons[first], *buttons[second], prompt));
        QVERIFY(UiInspection::writeReport(prompt,
                                          QStringLiteral("mount-%1-%2-%3")
                                              .arg(unverified ? "recovery" : "extraction", language)
                                              .arg(width)));
        prompt.reject();
    }
    void setupGeometry_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<QString>("language");
        QTest::newRow("EN normal") << 640 << QStringLiteral("en");
        QTest::newRow("EN compact") << 420 << QStringLiteral("en");
        QTest::newRow("RU normal") << 640 << QStringLiteral("ru");
        QTest::newRow("RU compact") << 420 << QStringLiteral("ru");
    }
    void setupGeometry() {
        QFETCH(int, width);
        QFETCH(QString, language);
        QTranslator catalog;
        if (language == "ru") {
            QVERIFY(catalog.load(":/translations/sevenzip_ru.qm"));
            qApp->installTranslator(&catalog);
        }
        bool inspected = false, fits = true;
        QTimer inspect;
        connect(&inspect, &QTimer::timeout, this, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "archiveMountSettings")
                return;
            inspect.stop();
            dialog->resize(width, dialog->sizeHint().height());
            QTest::qWait(10);
            fits = dialog->width() <= width;
            for (const auto *label : dialog->findChildren<QLabel *>())
                if (label->isVisible())
                    fits = fits && UiInspection::fullyVisible(*label, *dialog) &&
                           UiInspection::labelFits(*label);
            for (const auto *button : dialog->findChildren<QPushButton *>())
                if (button->isVisible())
                    fits = fits && UiInspection::fullyVisible(*button, *dialog);
            inspected = UiInspection::writeReport(
                *dialog, QStringLiteral("mount-settings-%1-%2").arg(language).arg(width));
            dialog->reject();
        });
        inspect.start(5);
        const auto options = QtAskMountOptions(
            nullptr, "Synthetic image/длинное имя с пробелами/данные 空.img", "SyntheticOnly");
        QVERIFY(!options.Accepted);
        QVERIFY(inspected);
        QVERIFY(fits);
    }
};
QTEST_MAIN(MountExtractionPromptTests)
#include "mount_extraction_prompt_tests.moc"
