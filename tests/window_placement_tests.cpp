#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "layout_inspection.h"
#include "qtfilemanagerinitservices.h"
#include "window_state_support.h"
#include <QScopeGuard>
#include <QSplitter>

namespace {
struct RecordingInitWindow final : IQtFileManagerInitServices {
    bool createResult = true, placementAvailable = true;
    bool created = false, placed = false, panelsDefined = false;
    mutable bool readPlacement = false;
    int shown = -1;
    int createdX = 0;
    UInt32 splitter = 0;
    QRect createdRect;
    QtFileManagerPlacement placement{{10, 20, 510, 420}};
    bool Create(int x, int y, int width, int height, bool panels, UInt32 position) override {
        created = true;
        createdX = x;
        if (width != NQtFileManager::WindowUseDefault)
            createdRect = QRect(x, y, width, height);
        panelsDefined = panels;
        splitter = position;
        return createResult;
    }
    bool GetPlacement(QtFileManagerPlacement &value) const override {
        readPlacement = true;
        value = placement;
        return placementAvailable;
    }
    void SetPlacement(const QtFileManagerPlacement &value) override {
        placed = true;
        placement = value;
    }
    void Show(int command) override { shown = command; }
};
} // namespace

class WindowPlacementTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void sourceShowCommands_data() {
        QTest::addColumn<int>("command");
        QTest::addColumn<bool>("savedMaximized");
        QTest::addColumn<unsigned>("expectedCommand");
        using namespace NQtFileManager::NShowCommand;
        QTest::newRow("normal-restored") << Normal << false << unsigned(Normal);
        QTest::newRow("normal-maximized") << Normal << true << unsigned(Maximized);
        QTest::newRow("show-maximized") << Show << true << unsigned(Maximized);
        QTest::newRow("default-maximized") << Default << true << unsigned(Maximized);
        QTest::newRow("explicit-maximized") << Maximized << false << unsigned(Maximized);
        QTest::newRow("explicit-minimized") << Minimized << true << unsigned(Minimized);
    }

    void sourceShowCommands() {
        QFETCH(int, command);
        QFETCH(bool, savedMaximized);
        QFETCH(unsigned, expectedCommand);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("settings.ini");
        CWindowInfo info{{-40, 20, 940, 640}, savedMaximized, 2, 1, 370};
        info.Save(settings);
        CApp app;
        RecordingInitWindow services;
        QVERIFY(NQtFileManager::InitInstance(command, app, services, settings));
        QVERIFY(services.created && services.readPlacement && services.placed);
        QCOMPARE(services.shown, -1);
        QCOMPARE(services.createdRect, QRect(-40, 20, 980, 620));
        QVERIFY(services.panelsDefined);
        QCOMPARE(services.splitter, 370u);
        QCOMPARE(app.NumPanels, 2u);
        QCOMPARE(app.LastFocusedPanel, 1u);
        QCOMPARE(services.placement.showCmd, expectedCommand);
        QCOMPARE(services.placement.normalPosition.left, -40);
        QCOMPARE(services.placement.normalPosition.right, 940);
    }

    void sourceDefaultsRefusalAndPlacementFallback() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("settings.ini");
        CApp app;
        RecordingInitWindow missing;
        QVERIFY(NQtFileManager::InitInstance(1, app, missing, settings));
        QVERIFY(!missing.panelsDefined);
        QCOMPARE(missing.createdX, NQtFileManager::WindowUseDefault);
        QCOMPARE(app.NumPanels, 1u);
        QCOMPARE(app.LastFocusedPanel, 0u);
        QCOMPARE(missing.placement.normalPosition.left, 10);
        QCOMPARE(missing.placement.showCmd, 1u);
        CWindowInfo info{{42, 54, 1022, 674}, true, 99, 88, 310};
        info.Save(settings);
        RecordingInitWindow refusal;
        refusal.createResult = false;
        QVERIFY(!NQtFileManager::InitInstance(1, app, refusal, settings));
        QVERIFY(refusal.created && !refusal.readPlacement && !refusal.placed);
        QCOMPARE(refusal.shown, -1);
        QCOMPARE(app.NumPanels, 1u);
        QCOMPARE(app.LastFocusedPanel, 0u);
        RecordingInitWindow fallback;
        fallback.placementAvailable = false;
        QVERIFY(NQtFileManager::InitInstance(1, app, fallback, settings));
        QVERIFY(fallback.created && fallback.readPlacement && !fallback.placed);
        QCOMPARE(fallback.shown, 3);
        QSettings key(settings, QSettings::IniFormat);
        key.setValue("Position", QByteArray(19, '\0'));
        key.sync();
        RecordingInitWindow malformed;
        QVERIFY(NQtFileManager::InitInstance(1, app, malformed, settings));
        QVERIFY(malformed.panelsDefined);
        QCOMPARE(malformed.placement.normalPosition.left, 10);
        QCOMPARE(malformed.placement.showCmd, 1u);
    }

    void savedPlacementAndRestart_data() {
        QTest::addColumn<QSize>("size");
        QTest::addColumn<bool>("maximized");
        QTest::addColumn<unsigned>("panels");
        QTest::newRow("normal-one") << QSize(980, 620) << false << 1u;
        QTest::newRow("compact-two") << QSize(640, 400) << false << 2u;
        QTest::newRow("maximized-two") << QSize(980, 620) << true << 2u;
        QTest::newRow("maximized-one") << QSize(640, 400) << true << 1u;
    }

    void splitterCoordinateAndResize_data() {
        QTest::addColumn<int>("initialWidth");
        QTest::newRow("normal") << 980;
        QTest::newRow("compact") << 640;
    }

    void splitterBounds_data() {
        QTest::addColumn<bool>("defined");
        QTest::addColumn<unsigned>("position");
        QTest::addColumn<int>("expectedLeft");
        QTest::newRow("minimum") << true << 0u << 120;
        QTest::newRow("maximum") << true << 999u << 516;
        QTest::newRow("default") << false << 0u << 318;
    }

    void splitterBounds() {
        QFETCH(bool, defined);
        QFETCH(unsigned, position);
        QFETCH(int, expectedLeft);
        QtFileManagerSplitter splitter(nullptr);
        QWidget left, right;
        splitter.addWidget(&left);
        splitter.addWidget(&right);
        splitter.resize(640, 200);
        splitter.show();
        splitter.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&splitter));
        splitter.Initialize(defined, position);
        QCOMPARE(splitter.sizes().value(0), expectedLeft);
    }

    void splitterCompactResizeAndHandleInput() {
        QtFileManagerSplitter splitter(nullptr);
        QWidget left, right;
        splitter.addWidget(&left);
        splitter.addWidget(&right);
        splitter.resize(640, 200);
        splitter.show();
        splitter.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&splitter));
        splitter.Initialize(false, 0);
        QCOMPARE(splitter.sizes().value(0), 318);
        splitter.resize(200, 200);
        QTRY_COMPARE_WITH_TIMEOUT(splitter.sizes().value(0), 98, 1000);
        splitter.resize(640, 200);
        QTRY_COMPARE_WITH_TIMEOUT(splitter.sizes().value(0), 318, 1000);
        auto *handle = splitter.handle(1);
        QSignalSpy moved(&splitter, &QSplitter::splitterMoved);
        const QPoint point = handle->rect().center();
        QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, point);
        QTest::mouseMove(handle, point + QPoint(50, 0));
        QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier, point + QPoint(50, 0));
        QVERIFY(!moved.isEmpty());
        QCOMPARE(splitter.sizes().value(0), 368);
        QCOMPARE(splitter.SourcePosition(), 368);
    }

    void splitterCoordinateAndResize() {
        QFETCH(int, initialWidth);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString previous = QDir::currentPath();
        const auto restore = qScopeGuard([&] { QDir::setCurrent(previous); });
        QVERIFY(QDir::setCurrent(fixture.path()));
        for (unsigned i = 0; i < 2; ++i)
            SavePanelPath(i, ToUString(fixture.path()), settings);
        CWindowInfo info{{42, 54, 42 + initialWidth, 674}, false, 2, 0, 300};
        info.Save(settings);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *splitter = window.findChild<QSplitter *>("filePanelSplitter");
        QVERIFY(splitter);
        QTRY_COMPARE_WITH_TIMEOUT(splitter->sizes().value(0), 300, 1000);
        const int ratio = (300 + 2) * (1 << 16) / initialWidth;
        window.resize(1600, 620);
        const int expandedPosition = 1600 * ratio / (1 << 16) - 2;
        QTRY_COMPARE_WITH_TIMEOUT(splitter->sizes().value(0), expandedPosition, 1000);
        window.resize(initialWidth, 620);
        const int restoredPosition = initialWidth * ratio / (1 << 16) - 2;
        QTRY_COMPARE_WITH_TIMEOUT(splitter->sizes().value(0), restoredPosition, 1000);
        auto *twoPanels = action(window, "twoPanelsAction");
        QTRY_VERIFY(twoPanels->isEnabled());
        twoPanels->trigger();
        QVERIFY(!twoPanels->isChecked());
        window.resize(1600, 620);
        twoPanels->trigger();
        QVERIFY(twoPanels->isChecked());
        QTRY_COMPARE_WITH_TIMEOUT(splitter->sizes().value(0), expandedPosition, 1000);
        window.resize(initialWidth, 620);
        QTRY_COMPARE_WITH_TIMEOUT(splitter->sizes().value(0), restoredPosition, 1000);
        QVERIFY(UiInspection::writeReport(window, QString("splitter-source-%1").arg(initialWidth)));
        QVERIFY(window.close());
        bool position = false, panels = false;
        info.Read(position, panels, settings);
        QVERIFY(position && panels);
        QCOMPARE(info.splitterPos, unsigned(restoredPosition));
    }

    void savedPlacementAndRestart() {
        QFETCH(QSize, size);
        QFETCH(bool, maximized);
        QFETCH(unsigned, panels);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString previous = QDir::currentPath();
        const auto restore = qScopeGuard([&] { QDir::setCurrent(previous); });
        QVERIFY(QDir::setCurrent(fixture.path()));
        const QString payload = fixture.filePath(QStringLiteral("protected 空.txt"));
        QFile file(payload);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("window fixture"), qint64(14));
        file.close();
        for (unsigned i = 0; i < 2; ++i)
            SavePanelPath(i, ToUString(fixture.path()), settings);
        const QRect expected(QPoint(42, 54), size);
        const unsigned focused = panels - 1;
        CWindowInfo info{{expected.x(), expected.y(), expected.x() + expected.width(),
                          expected.y() + expected.height()},
                         maximized,
                         panels,
                         focused,
                         unsigned(size.width() / 3)};
        info.Save(settings);
        for (int launch = 0; launch < 2; ++launch) {
            ControlledBackend backend;
            MainWindow window(&backend);
            QVERIFY(!window.isVisible());
            QCOMPARE(window.geometry(), expected);
            QCOMPARE(window.isMaximized(), maximized);
            QCOMPARE(action(window, "twoPanelsAction")->isChecked(), panels == 2);
            window.show();
            window.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(focused));
            QVERIFY(panel);
            QTRY_VERIFY(panel->isAncestorOf(QApplication::focusWidget()));
            QVERIFY(UiInspection::fullyVisible(*panel, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("window-placement-%1-%2").arg(QTest::currentDataTag()).arg(launch)));
            const QRect normal = maximized ? window.normalGeometry() : window.geometry();
            QCOMPARE(normal, expected);
            auto *splitter = window.findChild<QSplitter *>("filePanelSplitter");
            QVERIFY(splitter);
            const int leftWidth = splitter->sizes().value(0);
            if (panels == 2) {
                QVERIFY(leftWidth > 0);
                QVERIFY(leftWidth < splitter->width() / 2);
            }
            QVERIFY(window.close());
            bool position = false, panelInfo = false;
            info.Read(position, panelInfo, settings);
            QVERIFY(position && panelInfo);
            QCOMPARE(info.maximized, maximized);
            QCOMPARE(info.numPanels, panels);
            QCOMPARE(info.currentPanel, focused);
            if (panels == 2)
                QCOMPARE(info.splitterPos, unsigned(leftWidth));
            QCOMPARE(QRect(info.rect.left, info.rect.top, info.rect.right - info.rect.left,
                           info.rect.bottom - info.rect.top),
                     expected);
        }
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("window fixture"));
    }
};

QTEST_MAIN(WindowPlacementTests)
#include "window_placement_tests.moc"
