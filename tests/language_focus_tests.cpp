#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "createarchivedialog.h"
#include "qtlangcontrols.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class LanguageFocusTests : public QObject {
    Q_OBJECT
    QString settingsPath() const { return qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"); }

  private slots:
    void init() {
        QFile::remove(settingsPath());
        SaveRegLang(UString("-"), settingsPath());
        ReloadLang(settingsPath());
    }
    void cleanup() {
        SaveRegLang(UString("-"), settingsPath());
        ReloadLang(settingsPath());
    }
    void sourceRussianCollisionRetainsKeyboardReachability() {
        QTemporaryDir directory;
        SaveRegLang(UString("ru"), settingsPath());
        ReloadLang(settingsPath());
        CreateArchiveDialog dialog(directory.path());
        auto *show = dialog.findChild<QCheckBox *>("archiveShowPassword");
        auto *parameters = dialog.findChild<QLineEdit *>("archiveParameters");
        auto *password = dialog.findChild<QLineEdit *>("archivePassword");
        QLabel *parametersLabel = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (label->buddy() == parameters)
                parametersLabel = label;
        QVERIFY(show && parameters && password && parametersLabel);
        QCOMPARE(show->text(), QStringLiteral("&Показать пароль"));
        QCOMPARE(parametersLabel->text(), QStringLiteral("&Параметры:"));
        password->setText(QStringLiteral("pending secret 密碼"));
        parameters->setText("mt=2");
        dialog.show();
        dialog.activateWindow();
        QTRY_VERIFY(dialog.isActiveWindow());
        QSignalSpy toggles(show, &QCheckBox::toggled);
        password->setFocus();
        const QKeySequence mnemonic = QKeySequence::mnemonic(show->text());
        QCOMPARE(show->shortcut(), mnemonic);
        if (!mnemonic.isEmpty()) {
            const auto pressMnemonic = [&] {
                const auto key = mnemonic[0];
                // Deliver a complete Unicode key sequence through the window,
                // including Alt transitions and the shortcut override phase.
                QTest::sendKeyEvent(QTest::Click, dialog.windowHandle(), key.key(),
                                    QString(QChar(ushort(key.key()))), key.keyboardModifiers());
            };
            pressMnemonic();
            QWidget *first = dialog.focusWidget();
            QVERIFY(first == show || first == parameters);
            pressMnemonic();
            QWidget *second = dialog.focusWidget();
            QVERIFY(second == show || second == parameters);
            QVERIFY(first != second);
            QCOMPARE(toggles.count(), 0);
        }
        bool reachedShow = false, reachedParameters = false;
        for (int i = 0; i < 80 && (!reachedShow || !reachedParameters); ++i) {
            QTest::keyClick(dialog.focusWidget(), Qt::Key_Tab);
            reachedShow |= dialog.focusWidget() == show;
            reachedParameters |= dialog.focusWidget() == parameters;
        }
        QVERIFY(reachedShow && reachedParameters);
        QCOMPARE(toggles.count(), 0);
        QCOMPARE(password->text(), QStringLiteral("pending secret 密碼"));
        QCOMPARE(parameters->text(), QString("mt=2"));
    }
    void compositeRowLabelsFocusControls() {
        // Composite form rows must direct their label association to the input.
        QTemporaryDir directory;
        CreateArchiveDialog dialog(directory.path());
        dialog.show();
        dialog.activateWindow();
        QTRY_VERIFY(dialog.isActiveWindow());
        for (const char *lang : {"-", "ru", "-"}) {
            SaveRegLang(UString(lang), settingsPath());
            ReloadLang(settingsPath());
            QtNotifyLanguageChanged();
            for (const char *name : {"compressionThreads", "compressionMemoryLimit"}) {
                auto *combo = dialog.findChild<QComboBox *>(name);
                QVERIFY(combo && combo->isEnabled());
                QLabel *label = nullptr;
                for (auto *candidate : dialog.findChildren<QLabel *>())
                    if (candidate->buddy() == combo->parentWidget())
                        label = candidate;
                QVERIFY(label && !combo->accessibleName().isEmpty());
                QCOMPARE(combo->accessibleName(), QString(label->text()).remove('&'));
                dialog.findChild<QLineEdit *>("archiveName")->setFocus();
                label->buddy()->setFocus();
                QCOMPARE(dialog.focusWidget(), combo);
            }
        }
    }
    void actualFormLanguageFallbacksRetainPendingValues() {
        QTemporaryDir directory;
        CreateArchiveDialog dialog(directory.path());
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *name = dialog.findChild<QLineEdit *>("archiveName");
        auto *password = dialog.findChild<QLineEdit *>("archivePassword");
        level->setCurrentIndex(level->count() - 1);
        const QString englishLevel = level->currentText();
        const QVariant levelData = level->currentData();
        name->setText(QStringLiteral("pending name 密碼.7z"));
        password->setText(QStringLiteral("pending secret 密碼"));
        SaveRegLang(UString("ru"), settingsPath());
        ReloadLang(settingsPath());
        QtNotifyLanguageChanged();
        QVERIFY(level->currentText() != englishLevel);
        QFile partial(directory.filePath("partial.txt"));
        QVERIFY(partial.open(QIODevice::WriteOnly));
        QVERIFY(partial.write(";!@Lang2@!UTF-8!\n0\n7-Zip\nPartial\nPartial\n4000\nPartial Add\n") >
                0);
        partial.close();
        SaveRegLang(UString("partial"), settingsPath());
        ReloadLang(settingsPath(), directory.path());
        QtNotifyLanguageChanged();
        QCOMPARE(dialog.windowTitle(), QString("Partial Add"));
        QCOMPARE(level->currentText(), englishLevel);
        QVERIFY(partial.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(partial.write(";!@Lang2@!UTF-8!\n0\n7-Zip\nPartial\nPartial\n4000\n\n") > 0);
        partial.close();
        ReloadLang(settingsPath(), directory.path());
        QtNotifyLanguageChanged();
        QCOMPARE(dialog.windowTitle(), QString("Add to Archive"));
        QCOMPARE(level->currentText(), englishLevel);
        QVERIFY(partial.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(partial.write("malformed language"), qint64(18));
        partial.close();
        ReloadLang(settingsPath(), directory.path());
        QtNotifyLanguageChanged();
        QCOMPARE(dialog.windowTitle(), QString("Add to Archive"));
        QCOMPARE(level->currentText(), englishLevel);
        QCOMPARE(level->currentData(), levelData);
        QCOMPARE(name->text(), QStringLiteral("pending name 密碼.7z"));
        QCOMPARE(password->text(), QStringLiteral("pending secret 密碼"));
    }
};
QTEST_MAIN(LanguageFocusTests)
#include "language_focus_tests.moc"
