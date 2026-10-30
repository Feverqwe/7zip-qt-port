#include "Common/MyCom.h"
#include "Common/StringConvert.h"
#include "localization/LangUtils.h"
#include "platform/associations/qtassociationplatform.h"
#include "settings/SystemPage.h"
#include "settings/ViewSettings.h"
#include "settings/qtfmoptionsdialog.h"
#include "ui/support/layout_inspection.h"
#include <QFileIconProvider>
#include <QPainter>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>

namespace {
class FormatFixture final : public IFolderManager, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderManager)
  public:
    Z7_COM7F_IMF(OpenFolderFile(IInStream *, const wchar_t *, const wchar_t *, IFolderFolder **,
                                IProgress *))
    override { return E_NOTIMPL; }
    Z7_COM7F_IMF(GetExtensions(BSTR *extensions)) override {
        return StringToBstr(UString(L"zip"), extensions);
    }
    Z7_COM7F_IMF(GetIconPath(const wchar_t *, BSTR *path, Int32 *index)) override {
        *index = 0;
        // A real codec manager supplies a module filename, not an app bundle.
        return StringToBstr(ToUString(QCoreApplication::applicationFilePath()), path);
    }
};
class StateFixture final : public IQtAssociationPlatform {
    const std::shared_ptr<IQtAssociationPlatform> native_ = CreateNativeAssociationPlatform();

  public:
    int writes = 0;
    bool CanApply(unsigned group) const override { return group == 0; }
    QString UnavailableReason(unsigned) const override { return "Current user only"; }
    UString OwnIconPath() const override { return native_->OwnIconPath(); }
    QIcon OwnIcon() const override { return native_->OwnIcon(); }
    LONG Read(unsigned group, const UString &, QtAssociationRecord &record) override {
        if (group != 0)
            return 50;
        record.info.ProgramKey = "Fixture.Other";
        record.info.IconPath = ToUString(QCoreApplication::applicationFilePath());
        record.info.IconIndex = 0;
        record.revision = "unchanged";
        return 0;
    }
    LONG Write(unsigned, const UString &, bool, const QtAssociationRecord &, const UString &,
               const UString &, const UString &, int) override {
        ++writes;
        return 5;
    }
    void NotifyChanged() override {}
};
QImage image(const QIcon &icon) {
    QImage result(16, 16, QImage::Format_RGBA8888);
    result.fill(Qt::white);
    QPainter painter(&result);
    painter.drawPixmap(QRect(0, 0, 16, 16), icon.pixmap(16, 16));
    painter.end();
    return result;
}
} // namespace

class MacSystemPageTests final : public QObject {
    Q_OBJECT
  private slots:
    void ownSelectionUsesNativeBundleImage_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize &size : {QSize(640, 390), QSize(520, 380)})
                QTest::newRow(qPrintable(language + '-' + QString::number(size.width())))
                    << language << size;
    }
    void ownSelectionUsesNativeBundleImage() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("settings.ini");
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        CMyComPtr<IFolderManager> manager = new FormatFixture;
        const auto platform = std::make_shared<StateFixture>();
        QtFmOptionsDialog dialog(settings, nullptr, manager, CreateQtAssociationServices(platform));
        dialog.resize(size);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        auto *list = dialog.findChild<QTreeWidget *>("systemAssociations");
        auto *current = dialog.findChild<QPushButton *>("systemCurrent");
        QVERIFY(list && current && list->topLevelItemCount() == 1);
        auto *row = list->topLevelItem(0);
        const QImage other = image(row->icon(0));
        const QIcon own = QFileIconProvider().icon(QFileInfo(ToQString(platform->OwnIconPath())));
        QVERIFY(!own.isNull());
        QVERIFY(other != image(own));
        row->setSelected(true);
        list->setCurrentItem(row);
        dialog.setProperty("uiInspection_language", language == "ru" ? "ru" : "en");
        const QString tag = "mac-system-bundle-" + QString::fromLatin1(QTest::currentDataTag());
        QVERIFY(UiInspection::writeReport(dialog, tag + "-other"));
        current->click();
        QCOMPARE(row->text(1), QString("7-Zip"));
        QCOMPARE(image(row->icon(0)), image(own));
        QVERIFY(UiInspection::writeReport(dialog, tag + "-own"));
        current->click();
        QVERIFY(row->text(1).isEmpty());
        QVERIFY(row->icon(0).isNull());
        current->click();
        QCOMPARE(row->text(1), QString("Fixture.Other"));
        QCOMPARE(image(row->icon(0)), other);
        QVERIFY(UiInspection::writeReport(dialog, tag + "-restored"));
        current->click();
        dialog.reject();
        QCOMPARE(platform->writes, 0);
    }
};
QTEST_MAIN(MacSystemPageTests)
#include "mac_system_page_tests.moc"
