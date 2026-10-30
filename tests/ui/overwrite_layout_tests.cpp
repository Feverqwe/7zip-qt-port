#include "dialogs/overwrite/OverwriteDialog.h"
#include "localization/LangUtils.h"
#include "localization/qtlangcontrols.h"
#include "settings/RegistryUtils.h"
#include "ui/support/layout_inspection.h"
#include <QLabel>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

class OverwriteLayoutTests final : public QObject {
    Q_OBJECT

  private slots:
    void sourceFooterAndFileRows_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<int>("width");
        QTest::addColumn<bool>("extra");
        for (const QString &language : {QString("-"), QString("ru")})
            for (int width : {700, 560})
                for (bool extra : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(language).arg(width).arg(extra)))
                        << language << width << extra;
    }

    void sourceFooterAndFileRows() {
        QFETCH(QString, language);
        QFETCH(int, width);
        QFETCH(bool, extra);
        QTemporaryDir settings;
        QVERIFY(settings.isValid());
        const QString ini = settings.filePath("language.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), ini);
        ReloadLang(ini);
        const auto restoreLanguage = qScopeGuard([&] {
            SaveRegLang(UString(L"-"), ini);
            ReloadLang(ini);
            QtNotifyLanguageChanged();
        });

        COverwriteDialog dialog;
        dialog.ShowExtraButtons = extra;
        dialog.DefaultButton_is_NO = true;
        dialog.OldFileInfo.Path = L"/destination/каталог 文件/old.txt";
        dialog.NewFileInfo.Path = L"/source/каталог 文件/new.txt";
        dialog.OldFileInfo.SetSize(12345);
        dialog.NewFileInfo.SetSize(67890);
        QVERIFY(dialog.OnInit());
        dialog.resize(width, 440);
        dialog.show();
        QApplication::processEvents();
        QVERIFY(UiInspection::writeReport(
            dialog, QString("overwrite-layout-%1-%2-%3").arg(language).arg(width).arg(extra)));

        auto *yes = dialog.findChild<QPushButton *>("overwriteYes");
        auto *no = dialog.findChild<QPushButton *>("overwriteNo");
        auto *yesAll = dialog.findChild<QPushButton *>("overwriteYesAll");
        auto *noAll = dialog.findChild<QPushButton *>("overwriteNoAll");
        auto *rename = dialog.findChild<QPushButton *>("overwriteRename");
        auto *cancel = dialog.findChild<QPushButton *>("overwriteCancel");
        QVERIFY(yes && no && yesAll && noAll && rename && cancel);
        QVERIFY(no->isDefault());
        const QRect yesBounds = UiInspection::bounds(*yes, dialog);
        const QRect noBounds = UiInspection::bounds(*no, dialog);
        const QRect cancelBounds = UiInspection::bounds(*cancel, dialog);
        if (extra) {
            QVERIFY(yesAll->isVisible() && noAll->isVisible() && rename->isVisible());
            QVERIFY(yesBounds.bottom() < noBounds.top());
            dialog.ShowExtraButtons = false;
            QVERIFY(dialog.OnInit());
            QApplication::processEvents();
            QVERIFY(UiInspection::writeReport(
                dialog,
                QString("overwrite-layout-%1-%2-%3-hidden").arg(language).arg(width).arg(extra)));
        } else {
            QVERIFY(yesBounds.right() < noBounds.left());
            QVERIFY(noBounds.right() < cancelBounds.left());
        }
        QVERIFY(!yesAll->isVisible() && !noAll->isVisible() && !rename->isVisible());
        for (auto *first : {yes, no, cancel})
            for (auto *second : {yes, no, cancel})
                if (first != second)
                    QVERIFY(UiInspection::separate(*first, *second, dialog));
        dialog.ShowExtraButtons = true;
        QVERIFY(dialog.OnInit());
        QApplication::processEvents();
        QVERIFY(yesAll->isEnabled() && noAll->isEnabled() && rename->isEnabled());
        for (const auto *button : {yes, no, yesAll, noAll, rename, cancel})
            if (button->isVisible()) {
                QVERIFY(UiInspection::fullyVisible(*button, dialog));
                QVERIFY(button->width() >= button->sizeHint().width());
                QVERIFY(button->height() >= button->sizeHint().height());
            }

        for (const QString &name : {QString("overwriteOldFile"), QString("overwriteNewFile")}) {
            auto *details = dialog.findChild<QLabel *>(name);
            auto *icon = dialog.findChild<QLabel *>(name + "Icon");
            QVERIFY(details && icon);
            const int flags = int(details->alignment()) | Qt::TextWordWrap;
            const QRect drawnText = details->fontMetrics().boundingRect(details->contentsRect(),
                                                                        flags, details->text());
            const int firstLineTop = details->mapTo(&dialog, drawnText.topLeft()).y();
            const int iconTop = icon->mapTo(&dialog, icon->contentsRect().topLeft()).y();
            QVERIFY(qAbs(firstLineTop - iconTop) <= 1);
            QVERIFY(UiInspection::fullyVisible(*details, dialog));
            QVERIFY(UiInspection::labelFits(*details));
        }
    }

    void initialFrameKeepsControlsReadableAtLargerFont() {
        const int points[] = {8, 12};
        for (int i = 0; i < 2; ++i) {
            COverwriteDialog dialog;
            QFont font = dialog.font();
            font.setPointSize(points[i]);
            dialog.setFont(font);
            dialog.OldFileInfo.Path = L"/old/file.txt";
            dialog.NewFileInfo.Path = L"/new/file.txt";
            dialog.OldFileInfo.SetSize(10);
            dialog.NewFileInfo.SetSize(20);
            QVERIFY(dialog.OnInit());
            dialog.show();
            QApplication::processEvents();
            for (auto *button : dialog.findChildren<QPushButton *>())
                if (button->isVisible()) {
                    QVERIFY(UiInspection::fullyVisible(*button, dialog));
                    QVERIFY(button->width() >= button->sizeHint().width());
                }
            for (auto *label : dialog.findChildren<QLabel *>())
                if (label->isVisible())
                    QVERIFY(UiInspection::labelFits(*label));
            QVERIFY(UiInspection::writeReport(dialog,
                                              QString("overwrite-initial-font-%1").arg(points[i])));
        }
    }
};

#include "ui/support/review_test_main.h"
SEVENZIP_UI_TEST_MAIN(OverwriteLayoutTests)
#include "overwrite_layout_tests.moc"
