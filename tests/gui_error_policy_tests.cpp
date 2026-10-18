#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/GUI/ExtractGUI.h"
#include "CPP/7zip/UI/GUI/UpdateCallbackGUI.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "CPP/Windows/FileDir.h"
#include "archivecodeccatalog.h"
#include "layout_inspection.h"
#include "qtupdateguiservices.h"
#include "source_extract_fixture.h"
#include <QAbstractButton>
#include <QLabel>
#include <QMessageBox>
#include <QScopeGuard>
#include <QTimer>
#include <QtTest>

extern bool g_DisableUserQuestions;

namespace {
class CServices final : public IQtUpdateGUIServices {
  public:
    unsigned Dialogs = 0;
    unsigned Updates = 0;
    HRESULT PresentCompressDialog(CCompressDialog &, const CObjectVector<NWildcard::CCensorPath> &,
                                  QWidget *) override {
        ++Dialogs;
        return E_ABORT;
    }
    HRESULT UpdateArchive(CCodecs *, const CObjectVector<COpenType> &, const UString &,
                          NWildcard::CCensor &, CUpdateOptions &, CUpdateErrorInfo &,
                          IOpenCallbackUI *, IUpdateCallbackUI2 *, bool) override {
        ++Updates;
        return E_FAIL;
    }
};
class CMessages {
    QTimer _timer;

  public:
    QStringList Texts;
    explicit CMessages(const QString &tag) {
        QObject::connect(&_timer, &QTimer::timeout, [this, tag] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (!box)
                return;
            const auto close = qScopeGuard([&] { box->accept(); });
            Texts << box->text();
#ifdef Q_OS_MACOS
            // Qt's macOS message presentation omits the native window caption.
            QCOMPARE(box->windowTitle(), QString());
#else
            QCOMPARE(box->windowTitle(), QString("7-Zip"));
#endif
            QCOMPARE(box->icon(), QMessageBox::Critical);
            QCOMPARE(box->standardButtons(), QMessageBox::StandardButtons(QMessageBox::Ok));
            for (const QSize size : {QSize(480, 220), QSize(320, 160)}) {
                box->resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*box->button(QMessageBox::Ok), *box));
                auto *label = box->findChild<QLabel *>("qt_msgbox_label");
                QVERIFY(label && UiInspection::fullyVisible(*label, *box));
                QVERIFY(UiInspection::labelFits(*label));
                QVERIFY(UiInspection::writeReport(*box, tag + '-' + QString::number(size.width())));
            }
        });
        _timer.start(1);
    }
};
void addData(bool extract) {
    QTest::addColumn<bool>("suppressed");
    QTest::addColumn<bool>("showDialog");
    QTest::addColumn<QString>("language");
    for (bool suppressed : {false, true})
        for (bool showDialog : {false, true}) {
            if (!extract && !showDialog)
                continue;
            for (const QString &language : {QString("-"), QString("ru")})
                QTest::newRow(
                    qPrintable(QString("%1-%2-%3").arg(suppressed).arg(showDialog).arg(language)))
                    << suppressed << showDialog << language;
        }
}
} // namespace

class GuiErrorPolicyTests final : public QObject {
    Q_OBJECT
  private slots:
    void missingWriter_data() { addData(false); }
    void missingWriter() {
        QFETCH(bool, suppressed);
        QFETCH(bool, showDialog);
        QFETCH(QString, language);
        const bool previous = g_DisableUserQuestions;
        const auto restore = qScopeGuard([&] { g_DisableUserQuestions = previous; });
        g_DisableUserQuestions = suppressed;
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("language.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        const QString input = fixture.filePath(QStringLiteral("данные 空.bin"));
        QVERIFY(SourceExtractFixture::writeFile(input, "original"));
        CCodecs codecs;
        CUpdateOptions options;
        options.SetActionCommand_Add();
        CUpdateCallbackGUI callback;
        callback.Password = "unchanged";
        NWildcard::CCensor censor;
        censor.AddPreItem_NoWildcard(UString(input.toStdWString().c_str()));
        CServices services;
        CMessages messages(QString("gui-error-writer-%1-%2").arg(suppressed).arg(language));
        bool displayed = true;
        QCOMPARE(UpdateGUI(&codecs, {}, L"before.7z", censor, options, showDialog, displayed,
                           &callback, nullptr, services),
                 E_FAIL);
        QCOMPARE(messages.Texts, suppressed ? QStringList() : QStringList{"No Update Engines"});
        QVERIFY(!displayed);
        QCOMPARE(services.Dialogs, 0u);
        QCOMPARE(services.Updates, 0u);
        QCOMPARE(callback.Password, UString(L"unchanged"));
        QCOMPARE(SourceExtractFixture::readFile(input), QByteArray("original"));
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog catalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(catalog.result(), S_OK);
        CCodecs available;
        available.Formats = catalog.nativeCodecs().Formats;
        CObjectVector<COpenType> types;
        QVERIFY(ParseOpenTypes(available, UString(L"7z"), types));
        QVERIFY(options.InitFormatIndex(&available, types, L"before.7z"));
        QVERIFY(options.SetArcPath(&available, L"before.7z"));
        QCOMPARE(UpdateGUI(&available, types, L"before.7z", censor, options, true, displayed,
                           &callback, nullptr, services),
                 E_ABORT);
        QCOMPARE(services.Dialogs, 1u);
        QCOMPARE(services.Updates, 0u);
        QCOMPARE(SourceExtractFixture::readFile(input), QByteArray("original"));
#endif
    }
    void invalidOutput_data() { addData(true); }
    void invalidOutput() {
        QFETCH(bool, suppressed);
        QFETCH(bool, showDialog);
        QFETCH(QString, language);
        const bool previous = g_DisableUserQuestions;
        const auto restore = qScopeGuard([&] { g_DisableUserQuestions = previous; });
        g_DisableUserQuestions = suppressed;
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("language.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        const QString input = fixture.filePath(QStringLiteral("архив 空.7z"));
        QVERIFY(SourceExtractFixture::writeFile(input, "unchanged archive bytes"));
        CCodecs codecs;
        CExtractOptions options;
        const QString invalid = QString("../").repeated(256) + QStringLiteral("выход 空");
        options.OutputDir = us2fs(UString(invalid.toStdWString().c_str()));
        FString full;
        QVERIFY(!NWindows::NFile::NDir::MyGetFullPathName(options.OutputDir, full));
        CExtractCallbackImp *callback = new CExtractCallbackImp;
        CMyComPtr<IFolderArchiveExtractCallback> owner = callback;
        UStringVector paths, fullPaths;
        paths.Add(UString(input.toStdWString().c_str()));
        fullPaths = paths;
        NWildcard::CCensorNode censor;
        CMessages messages(
            QString("gui-error-output-%1-%2-%3").arg(suppressed).arg(showDialog).arg(language));
        bool displayed = false;
        QCOMPARE(ExtractGUI(&codecs, {}, {}, paths, fullPaths, censor, options, nullptr, showDialog,
                            displayed, callback, nullptr, nullptr),
                 E_FAIL);
        QCOMPARE(messages.Texts,
                 suppressed ? QStringList() : QStringList{"Incorrect output directory path"});
        QVERIFY(displayed);
        QCOMPARE(SourceExtractFixture::readFile(input), QByteArray("unchanged archive bytes"));
    }
};
#include "review_test_main.h"
SEVENZIP_UI_TEST_MAIN(GuiErrorPolicyTests)
#include "gui_error_policy_tests.moc"
