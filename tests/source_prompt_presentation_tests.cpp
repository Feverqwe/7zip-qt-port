#include "CPP/7zip/UI/FileManager/EditDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/MemDialog.h"
#include "CPP/7zip/UI/FileManager/MessagesDialog.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "qtdialogunits.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

class SourcePromptPresentationTests final : public QObject {
    Q_OBJECT

    static void inspect(QDialog &view, const QSize &source, bool resizable, const QString &tag) {
        QApplication::processEvents();
        QVERIFY(UiInspection::writeReport(view, tag));
        QCOMPARE(view.size(),
                 QtDialogUnits(view.font()).pixels(source).expandedTo(view.minimumSizeHint()));
        QCOMPARE(view.windowFlags().testFlag(Qt::WindowMinMaxButtonsHint), resizable);
        QVERIFY(!view.windowFlags().testFlag(Qt::WindowContextHelpButtonHint));
        for (auto *button : view.findChildren<QPushButton *>()) {
            QVERIFY2(UiInspection::fullyVisible(*button, view), qPrintable(button->objectName()));
            QVERIFY(button->width() >= button->sizeHint().width());
        }
        for (auto *label : view.findChildren<QLabel *>()) {
            if (!label->isVisible())
                continue;
            QVERIFY2(UiInspection::fullyVisible(*label, view), qPrintable(label->objectName()));
            QVERIFY2(UiInspection::labelFits(*label), qPrintable(label->objectName()));
        }
    }

  private slots:
    void sourceInitialFrame_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<int>("kind");
        for (const char *language : {"-", "ru"})
            for (int kind = 0; kind < 4; ++kind)
                QTest::newRow(qPrintable(QString("%1-%2").arg(language).arg(kind)))
                    << QString::fromLatin1(language) << kind;
    }
    void sourceInitialFrame() {
        QFETCH(QString, language);
        QFETCH(int, kind);
        SaveRegLang(UString(language.toStdWString().c_str()),
                    qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        const QString tag = QString("source-prompt-initial-%1-%2").arg(language).arg(kind);
        if (kind == 0) {
            CEditDialog view;
            view.Title = "Source detail";
            view.Text = L"Literal <value> 文件\nsecond line";
            view.OnInit();
            view.show();
            QVERIFY(QTest::qWaitForWindowExposed(&view));
            inspect(view, QSize(336, 256), true, tag);
            auto *close = view.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close);
            QCOMPARE(view.focusWidget(), close);
            QVERIFY(close->isDefault());
            QVERIFY(UiInspection::fullyVisible(*view.findChild<QPlainTextEdit *>(), view));
        } else if (kind == 1) {
            CPasswordDialog view;
            view.OnInit();
            view.show();
            QVERIFY(QTest::qWaitForWindowExposed(&view));
            inspect(view, QSize(216, 88), false, tag);
            QCOMPARE(view.focusWidget(), view.findChild<QLineEdit *>("passwordEdit"));
            const QRect show =
                UiInspection::bounds(*view.findChild<QCheckBox *>("passwordShow"), view);
            // Native button-box optical margins can extend its backing widget
            // into the spacer; assert the source gap between layout items.
            const QRect footer = view.layout()->itemAt(view.layout()->count() - 1)->geometry();
            QVERIFY(footer.top() - show.bottom() - 1 >=
                    QtDialogUnits(view.font()).pixels(QSize(0, 12)).height());
        } else if (kind == 2) {
            CMemDialog view;
            view.FilePath = L"fixture 文件.bin";
            view.OnInit();
            view.show();
            QVERIFY(QTest::qWaitForWindowExposed(&view));
            inspect(view, QSize(336, 216), false, tag);
            QCOMPARE(view.focusWidget(), view.findChild<QCheckBox *>("memSaveLimit"));
            auto *pair = view.findChild<QWidget *>("memLimitPair");
            QVERIFY(pair);
            QCOMPARE(pair->width(), QtDialogUnits(view.font()).pixels(QSize(50, 0)).width());
            QCOMPARE(view.findChild<QLineEdit *>("memLimit")->alignment(),
                     Qt::Alignment(Qt::AlignCenter));
            auto *remember = view.findChild<QCheckBox *>("memRemember");
            auto *allow = view.findChild<QRadioButton *>("memAllow");
            QVERIFY(remember->mapTo(&view, QPoint()).x() - allow->mapTo(&view, QPoint()).x() >=
                    QtDialogUnits(view.font()).pixels(QSize(10, 0)).width());
            const QRect group = UiInspection::bounds(*view.findChild<QWidget *>("memAction"), view);
            const QRect footer = view.layout()->itemAt(view.layout()->count() - 1)->geometry();
            QVERIFY(footer.top() - group.bottom() - 1 >=
                    QtDialogUnits(view.font()).pixels(QSize(0, 10)).height());
        } else {
            QWidget owner;
            owner.show();
            UStringVector messages;
            messages.Add(L"Source diagnostic 文件\nsecond line");
            CMessagesDialog view;
            view.Messages = &messages;
            bool inspected = false;
            QTimer::singleShot(0, &view, [&] {
                const auto close = qScopeGuard([&] { view.reject(); });
                inspected = true;
                QCOMPARE(view.parentWidget(), &owner);
                inspect(view, QSize(456, 176), true, tag);
                QCOMPARE(view.focusWidget(), view.findChild<QPushButton *>("IDOK"));
                QVERIFY(UiInspection::fullyVisible(*view.findChild<QTreeWidget *>(), view));
            });
            QCOMPARE(view.Create(&owner), INT_PTR(IDCANCEL));
            QVERIFY(inspected);
        }
    }
    void cleanup() {
        SaveRegLang(UString("-"), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
};
QTEST_MAIN(SourcePromptPresentationTests)
#include "source_prompt_presentation_tests.moc"
