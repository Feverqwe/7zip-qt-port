#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "qtcompressoptionsdialog.h"
#include "qtdialogunits.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTemporaryDir>
#include <QtTest>

class CompressOptionsPresentationTests final : public QObject {
    Q_OBJECT
    QTemporaryDir settings_;
    void data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("format");
        for (const QString language : {QString("en"), QString("ru")})
            for (const QString format :
                 {QString("7z"), QString("zip"), QString("wim"), QString("tar")})
                QTest::newRow(qPrintable(format + '-' + language)) << language << format;
    }
    CCompressDialog owner(NCompression::CFormatOptions &profile, const QString &format,
                          const QString &language) {
        const QString path = settings_.filePath("options.ini");
        SaveRegLang(language == "ru" ? UString(L"ru") : UString(L"-"), path);
        ReloadLang(path);
        CCompressDialog result;
        result.FormatOptions = &profile;
        result.ArcInfo.Name = UString(format.toStdWString().c_str());
        result.ArcInfo.Flags = NArcInfoFlags::kMTime | NArcInfoFlags::kMTime_Default |
                               NArcInfoFlags::kCTime | NArcInfoFlags::kATime;
        result.ArcInfo.TimeFlags = TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kWindows);
        if (format == "zip")
            result.ArcInfo.TimeFlags |= TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kUnix) |
                                        TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kDOS);
        if (format == "tar") {
            result.MethodID = NCompressDialog::kGnu;
            result.ArcInfo.Flags = NArcInfoFlags::kMTime | NArcInfoFlags::kMTime_Default;
            result.ArcInfo.TimeFlags = TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kWindows) |
                                       TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kUnix) |
                                       TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::k1ns) |
                                       TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT(NFileTimeType::kUnix);
        }
        result.SymLinks.Supported = result.HardLinks.Supported = format == "wim" || format == "tar";
#ifdef Q_OS_WIN
        result.AltStreams.Supported = result.NtSecurity.Supported = format == "wim";
#endif
        return result;
    }
    void show(COptionsDialog &dialog) {
        dialog.show();
        if (QGuiApplication::platformName() != "offscreen") {
            dialog.raise();
            dialog.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&dialog));
            QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        }
        QApplication::processEvents();
    }

  private slots:
    void initTestCase() { QVERIFY(settings_.isValid()); }
    void contentSizedInitialFrame_data() { data(); }
    void contentSizedInitialFrame() {
        QFETCH(QString, language);
        QFETCH(QString, format);
        NCompression::CFormatOptions profile;
        auto compression = owner(profile, format, language);
        COptionsDialog dialog(&compression);
        dialog.setProperty("uiInspection_language", language);
        dialog.setProperty("uiInspection_state", "compress-options-" + format + "-initial");
        show(dialog);
        QVERIFY(UiInspection::writeReport(
            dialog, QString("compress-options-source-%1-%2-initial").arg(format, language)));
        QVERIFY(dialog.height() <= dialog.sizeHint().height());
        if (!dialog.findChild<QGroupBox *>("IDG_COMPRESS_NTFS")->isVisible()) {
            const auto *info = dialog.findChild<QLabel *>("IDT_COMPRESS_TIME_INFO");
            QVERIFY(UiInspection::bounds(*info, dialog).top() <=
                    2 * info->fontMetrics().lineSpacing());
        }
#ifndef Q_OS_WIN
        QVERIFY(dialog.findChild<QCheckBox *>("IDX_COMPRESS_PRESERVE_ATIME")->isHidden());
#endif
        for (auto *group : dialog.findChildren<QGroupBox *>())
            if (group->isVisible())
                QVERIFY2(UiInspection::fullyVisible(*group, dialog),
                         qPrintable(group->objectName()));
        QVERIFY(UiInspection::separate(*dialog.findChild<QLabel *>("IDT_COMPRESS_TIME_PREC"),
                                       *dialog.findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC"),
                                       dialog));
        auto *precisionLabel = dialog.findChild<QLabel *>("IDT_COMPRESS_TIME_PREC");
        const QFontMetrics metrics(precisionLabel->font());
        if (precisionLabel->width() >=
            metrics.horizontalAdvance(precisionLabel->text().simplified())) {
            // A caption that fits must not retain a forced line break.
            QVERIFY(metrics.size(Qt::TextShowMnemonic, precisionLabel->text()).height() <=
                    metrics.height());
            QVERIFY(UiInspection::labelFits(*precisionLabel));
        }
        const char *first = (format == "wim" || format == "tar") ? "IDX_COMPRESS_NT_SYM_LINKS"
                            : format == "zip"                    ? "IDX_COMPRESS_PREC_SET"
                                                                 : "IDX_COMPRESS_CTIME_SET";
        const char *next = (format == "wim" || format == "tar") ? "IDX_COMPRESS_NT_HARD_LINKS"
                           : format == "zip"                    ? "IDX_COMPRESS_CTIME_SET"
                                                                : "IDX_COMPRESS_ATIME_SET";
        auto *firstControl = dialog.findChild<QCheckBox *>(first);
        auto *nextControl = dialog.findChild<QCheckBox *>(next);
        QCOMPARE(dialog.focusWidget(), firstControl);
        QTest::keyClick(firstControl, Qt::Key_Tab);
        QCOMPARE(dialog.focusWidget(), nextControl);
        QTest::keyClick(nextControl, Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(dialog.focusWidget(), firstControl);
    }
    void explicitValueKeyboardAndCompactFit_data() { data(); }
    void explicitValueKeyboardAndCompactFit() {
        QFETCH(QString, language);
        QFETCH(QString, format);
        NCompression::CFormatOptions profile;
        auto compression = owner(profile, format, language);
        COptionsDialog dialog(&compression);
        dialog.setProperty("uiInspection_language", language);
        show(dialog);
        for (const char *name : {"IDX_COMPRESS_PREC_SET", "IDX_COMPRESS_CTIME_SET",
                                 "IDX_COMPRESS_ATIME_SET", "IDX_COMPRESS_ZTIME_SET"}) {
            auto *box = dialog.findChild<QCheckBox *>(name);
            QVERIFY(box);
            if (box->isVisible()) {
                QVERIFY(!box->text().isEmpty() && box->text() != ":");
                QVERIFY(!box->accessibleName().isEmpty());
            }
        }
        if (format == "zip") {
            auto *explicitValue = dialog.findChild<QCheckBox *>("IDX_COMPRESS_PREC_SET");
            auto *precision = dialog.findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC");
            QVERIFY(!precision->isEnabled());
            explicitValue->setFocus();
            QTest::keyClick(explicitValue, Qt::Key_Space);
            QVERIFY(explicitValue->isChecked());
            QVERIFY(precision->isEnabled());
            precision->setFocus();
            QTest::keyClick(precision, Qt::Key_Down);
            QCOMPARE(precision->currentData().toUInt(), UInt32(NFileTimeType::kUnix));
            QVERIFY(!dialog.findChild<QCheckBox *>("IDX_COMPRESS_CTIME")->isVisible());
            QVERIFY(!dialog.findChild<QCheckBox *>("IDX_COMPRESS_ATIME")->isVisible());
            dialog.setProperty("uiInspection_state", "compress-options-zip-unix");
            QVERIFY(UiInspection::writeReport(
                dialog, QString("compress-options-source-zip-%1-unix").arg(language)));
        }
        dialog.setProperty("uiInspection_state", "compress-options-" + format + "-compact");
        dialog.resize(format == "wim"   ? QSize(480, 560)
                      : format == "tar" ? QSize(480, 430)
                                        : QSize(440, 340));
        QApplication::processEvents();
        QVERIFY(UiInspection::writeReport(
            dialog, QString("compress-options-source-%1-%2-compact").arg(format, language)));
        for (auto *label : dialog.findChildren<QLabel *>())
            if (label->isVisible()) {
                QVERIFY(UiInspection::fullyVisible(*label, dialog));
                QVERIFY(UiInspection::labelFits(*label));
            }
        for (auto *box : dialog.findChildren<QCheckBox *>())
            if (box->isVisible()) {
                QVERIFY2(UiInspection::fullyVisible(*box, dialog), qPrintable(box->objectName()));
                QStyleOptionButton option;
                option.initFrom(box);
                option.text = box->text();
                const QRect contents =
                    box->style()->subElementRect(QStyle::SE_CheckBoxContents, &option, box);
                const QSize text = box->fontMetrics().size(Qt::TextShowMnemonic, box->text());
                QVERIFY2(text.width() <= contents.width() && text.height() <= contents.height(),
                         qPrintable(box->objectName()));
                if (box->objectName().endsWith("_SET")) {
                    const QRect indicator =
                        box->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, box);
                    // A tall grid row must not separate its caption from the checkbox.
                    QVERIFY2(qAbs(indicator.center().y() - contents.center().y()) <=
                                 qMax(2, box->fontMetrics().height() / 4),
                             qPrintable(box->objectName()));
                }
            }
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::separate(*dialog.findChild<QGroupBox *>("IDG_COMPRESS_TIME"),
                                       *buttons, dialog));
        buttons->button(QDialogButtonBox::Cancel)->click();
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
        QVERIFY(!profile.CTime.Def);
        QCOMPARE(profile.TimePrec, UInt32(-1));

        COptionsDialog accepted(&compression);
        show(accepted);
        auto *setArchiveTime = accepted.findChild<QCheckBox *>("IDX_COMPRESS_ZTIME_SET");
        auto *archiveTime = accepted.findChild<QCheckBox *>("IDX_COMPRESS_ZTIME");
        setArchiveTime->setFocus();
        QTest::keyClick(setArchiveTime, Qt::Key_Space);
        QVERIFY(archiveTime->isEnabled());
        archiveTime->setFocus();
        QTest::keyClick(archiveTime, Qt::Key_Space);
        QVERIFY(archiveTime->isChecked());
        QTest::keyClick(archiveTime, Qt::Key_Return);
        QCOMPARE(accepted.result(), int(QDialog::Accepted));
        QVERIFY(profile.SetArcMTime.Def);
        QVERIFY(profile.SetArcMTime.Val);
    }
};
#include "review_test_main.h"
SEVENZIP_UI_TEST_MAIN(CompressOptionsPresentationTests)
#include "compress_options_presentation_tests.moc"
