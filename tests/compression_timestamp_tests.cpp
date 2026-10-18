#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "qtcompressoptionsdialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

class CompressionTimestampTests final : public QObject {
    Q_OBJECT
    QTemporaryDir fixture_;
    QString settings_;
    CCompressDialog owner(NCompression::CFormatOptions &profile, UInt32 flags) {
        CCompressDialog result;
        result.FormatOptions = &profile;
        result.ArcInfo.Name = L"TimestampFixture";
        result.ArcInfo.Flags =
            NArcInfoFlags::kMTime | NArcInfoFlags::kMTime_Default | NArcInfoFlags::kATime;
        result.ArcInfo.TimeFlags = flags;
        return result;
    }
    void language(const QString &name) {
        SaveRegLang(UString(name.toStdWString().c_str()), settings_);
        ReloadLang(settings_);
    }

  private slots:
    void initTestCase() {
        QVERIFY(fixture_.isValid());
        settings_ = fixture_.filePath("timestamp 空.ini");
    }
    void sourceComboSelectionAndErrorSentinel() {
        QComboBox view;
        CQtCompressCombo control;
        control.Attach(&view);
        QCOMPARE(control.GetItemData_of_CurSel(), quint32(-1));
        QCOMPARE(control.AddString_SetItemData(L"Unix", 1), 0);
        QCOMPARE(view.currentIndex(), -1);
        QCOMPARE(control.AddString_SetItemData(L"DOS", 2), 1);
        QCOMPARE(view.currentIndex(), -1);
        QCOMPARE(control.GetItemData(-1), quint32(-1));
        QCOMPARE(control.GetItemData(2), quint32(-1));
        control.SetCurSel(1);
        QCOMPARE(control.GetItemData_of_CurSel(), 2u);
        control.AddString_SetItemData(L"Windows", 0);
        QCOMPARE(view.currentIndex(), 1);
        control.SetCurSel(-1);
        QCOMPARE(control.GetItemData_of_CurSel(), quint32(-1));
        control.ResetContent();
        QCOMPARE(control.GetCount(), 0);
        QCOMPARE(control.GetItemData_of_CurSel(), quint32(-1));
    }
    void precisionSelection_data() {
        QTest::addColumn<UInt32>("flags");
        QTest::addColumn<UInt32>("stored");
        QTest::addColumn<int>("count");
        QTest::addColumn<int>("selected");
        QTest::addColumn<UInt32>("saved");
        QTest::addColumn<QString>("language");
        const UInt32 win = TIME_PREC_TO_ARC_FLAGS_MASK(0);
        const UInt32 unixTime = TIME_PREC_TO_ARC_FLAGS_MASK(1);
        const UInt32 dos = TIME_PREC_TO_ARC_FLAGS_MASK(2);
        for (const QString &language : {QString("-"), QString("ru")}) {
            const auto row = [&](const QString &name, UInt32 flags, UInt32 stored, int count,
                                 int selected, UInt32 saved) {
                QTest::newRow(qPrintable(name + '-' + language))
                    << flags << stored << count << selected << saved << language;
            };
            row("no-capabilities", 0, UInt32(-1), 0, -1, UInt32(-1));
            row("windows-default", win, UInt32(-1), 1, 0, UInt32(-1));
            row("one-without-default", unixTime, UInt32(-1), 1, -1, UInt32(-1));
            row("two-without-default", unixTime | dos, UInt32(-1), 2, -1, UInt32(-1));
            row("default-added", dos | TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT(1), UInt32(-1), 2, 0,
                UInt32(-1));
            row("explicit-dos", win | unixTime | dos, 2, 3, 2, 2);
            row("custom-precision", win, 3, 2, 1, 3);
            row("unsupported-low", win, 2, 1, 0, UInt32(-1));
        }
    }
    void precisionSelection() {
        QFETCH(UInt32, flags);
        QFETCH(UInt32, stored);
        QFETCH(int, count);
        QFETCH(int, selected);
        QFETCH(UInt32, saved);
        QFETCH(QString, language);
        this->language(language);
        NCompression::CFormatOptions profile;
        profile.FormatID = "TimestampFixture";
        profile.TimePrec = stored;
        profile.ATime.Def = true;
        profile.ATime.Val = true;
        auto compression = owner(profile, flags);
        for (bool cancel : {true, false}) {
            COptionsDialog dialog(&compression);
            dialog.show();
            QApplication::processEvents();
            auto *precision = dialog.findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC");
            QCOMPARE(precision->count(), count);
            QCOMPARE(precision->currentIndex(), selected);
            for (QSize size : {QSize(520, 400), QSize(440, 340)}) {
                dialog.resize(size);
                QApplication::processEvents();
                if (count)
                    QVERIFY(UiInspection::fullyVisible(*precision, dialog));
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("timestamp-%1-%2-%3-%4")
                                .arg(QString::fromLatin1(QTest::currentDataTag()))
                                .arg(cancel)
                                .arg(size.width())
                                .arg(count)));
            }
            dialog.findChild<QDialogButtonBox *>()
                ->button(cancel ? QDialogButtonBox::Cancel : QDialogButtonBox::Ok)
                ->click();
            QCOMPARE(dialog.result(), int(cancel ? QDialog::Rejected : QDialog::Accepted));
            QCOMPARE(profile.TimePrec, cancel ? stored : saved);
            QVERIFY(profile.ATime.Def && profile.ATime.Val);
        }
        NCompression::CInfo registry;
        registry.Formats.Add(profile);
        registry.Save(settings_);
        registry.Load(settings_);
        QCOMPARE(registry.Formats.Size(), 1u);
        QCOMPARE(registry.Formats[0].TimePrec, saved);
        QVERIFY(registry.Formats[0].ATime.Def && registry.Formats[0].ATime.Val);
    }
    void explicitChoiceAfterAbsentDefault_data() {
        QTest::addColumn<QString>("language");
        QTest::newRow("en") << QString("-");
        QTest::newRow("ru") << QString("ru");
    }
    void explicitChoiceAfterAbsentDefault() {
        QFETCH(QString, language);
        this->language(language);
        NCompression::CFormatOptions profile;
        auto compression =
            owner(profile, TIME_PREC_TO_ARC_FLAGS_MASK(1) | TIME_PREC_TO_ARC_FLAGS_MASK(2));
        COptionsDialog dialog(&compression);
        dialog.show();
        QApplication::processEvents();
        auto *precision = dialog.findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC");
        auto *set = dialog.findChild<QCheckBox *>("IDX_COMPRESS_PREC_SET");
        QCOMPARE(precision->currentIndex(), -1);
        QVERIFY(set->isVisible() && set->isEnabled() && !set->isChecked());
        QTest::mouseClick(set, Qt::LeftButton);
        QVERIFY(precision->isEnabled());
        precision->setFocus();
        QTest::keyClick(precision, Qt::Key_Down);
        QCOMPARE(precision->currentData().toUInt(), 1u);
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(profile.TimePrec, 1u);
        COptionsDialog restarted(&compression);
        restarted.show();
        QApplication::processEvents();
        QCOMPARE(restarted.findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC")->currentData().toUInt(),
                 1u);
        restarted.reject();
        QCOMPARE(profile.TimePrec, 1u);
    }
};
QTEST_MAIN(CompressionTimestampTests)
#include "compression_timestamp_tests.moc"
