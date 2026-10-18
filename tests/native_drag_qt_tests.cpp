#include "qtpaneldragnative.h"
#include "qtpanelfolder.h"
#include <QClipboard>
#include <QFile>
#include <QMimeData>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>
#include <ole2.h>

namespace {
int RunNativeConsumer() {
    auto *converter = QtNativePanelDragConverter::ForApplication();
    if (!converter)
        return 1;
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    if (!mime || !mime->hasUrls())
        return 2;
    auto data = converter->Read(*mime);
    if (!data)
        return 3;
    UStringVector names;
    if (data->GetNames(names) != S_OK || names.Size() != 1)
        return 4;
    QFile file(ToQString(names[0]));
    if (!file.open(QIODevice::ReadOnly) || file.readAll() != QByteArray("Qt native control"))
        return 5;
    if (!data->SetTargetFolder(L"C:\\native peer δ\\"))
        return 6;
    CTargetTransferInfo info;
    info.FuncType = k_DragTargetMode_Drop_End;
    info.Cmd_Type = NDragMenu::k_Copy_Base;
    info.Cmd_Effect = DROPEFFECT_MOVE;
    info.Flags = k_TargetFlags_MustBeProcessedBySource;
    info.FolderType = k_FolderType_Fs;
    info.X = -12;
    info.Y = 35;
    if (!data->SetTransfer(info))
        return 7;
    UInt32 flags = 0;
    if (!data->ReadTransferFlags(flags) || !(flags & k_SourceFlags_DoNotProcessInTarget) ||
        !(flags & k_SourceFlags_SetTargetFolder_NonEmpty))
        return 8;
    data->SendPerformedEffect(DROPEFFECT_COPY);
    return 0;
}
} // namespace

class NativeDragQtTests final : public QObject {
    Q_OBJECT
  private slots:
    void independentProcessExchangesNativeTransferThroughComProxy() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile file(fixture.filePath(QStringLiteral("peer перенос δ with spaces.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("Qt native control"), qint64(17));
        file.close();
        auto model = std::make_shared<CDataObject>();
        model->Names.Add(ToUString(QDir::toNativeSeparators(file.fileName())));
        CMyComPtr<IDataObject> data;
        QCOMPARE(CreatePanelDragDataObject(model, &data), S_OK);
        QCOMPARE(OleSetClipboard(data), S_OK);
        struct ClearClipboard {
            ~ClearClipboard() { OleSetClipboard(nullptr); }
        } clear;
        QProcess consumer;
        consumer.setWorkingDirectory(fixture.path());
        consumer.start(QCoreApplication::applicationFilePath(), {"--native-drag-consumer"});
        QVERIFY(consumer.waitForStarted(5000));
        QVERIFY(consumer.processId() != QCoreApplication::applicationPid());
        // Pump the native GUI dispatcher while the other process calls our STA.
        QTRY_COMPARE_WITH_TIMEOUT(consumer.state(), QProcess::NotRunning, 20000);
        QVERIFY2(consumer.exitStatus() == QProcess::NormalExit && consumer.exitCode() == 0,
                 qPrintable(QString("native consumer exit=%1: %2")
                                .arg(consumer.exitCode())
                                .arg(QString::fromLocal8Bit(consumer.readAllStandardError()))));
        QCOMPARE(ToQString(model->DestDirPrefix_FromTarget), QStringLiteral("C:\\native peer δ\\"));
        QVERIFY(model->m_Transfer_WasSet);
        QCOMPARE(model->m_Transfer.Target.FuncType, UInt32(k_DragTargetMode_Drop_End));
        QCOMPARE(model->m_Transfer.Target.Cmd_Effect, UInt32(DROPEFFECT_MOVE));
        QCOMPARE(model->m_Transfer.Target.X, -12);
        QCOMPARE(model->m_Transfer.Target.Y, 35);
        QVERIFY(model->DoNotProcessInTarget);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("Qt native control"));
    }
    void repeatedNativeConversionReleasesWhenMimeEnds() {
        QtNativePanelDragConverter converter;
        auto model = std::make_shared<CDataObject>();
        model->Names.Add(L"C:\\repeated δ.txt");
        std::weak_ptr<CDataObject> weak = model;
        CMyComPtr<IDataObject> data;
        QCOMPARE(CreatePanelDragDataObject(model, &data), S_OK);
        const FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        const QString type = converter.mimeForFormat(format);
        auto mime = std::make_unique<QMimeData>();
        for (int request = 0; request < 2; ++request) {
            mime->setData(type,
                          converter.convertToMime(type, data, QMetaType::fromType<QByteArray>())
                              .toByteArray());
            QVERIFY(converter.Read(*mime));
        }
        model.reset();
        data.Release();
        QVERIFY(!weak.expired());
        mime.reset();
        QVERIFY(weak.expired());
    }
    void converterRetainsNativeObjectAndRejectsUnrelatedMime() {
        QtNativePanelDragConverter converter;
        auto model = std::make_shared<CDataObject>();
        model->Names.Add(L"C:\\retained through Qt δ.txt");
        std::weak_ptr<CDataObject> weak = model;
        CMyComPtr<IDataObject> data;
        QCOMPARE(CreatePanelDragDataObject(model, &data), S_OK);
        const FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        const QString mime = converter.mimeForFormat(format);
        QVERIFY(!mime.isEmpty());
        QVERIFY(converter.canConvertToMime(mime, data));
        const QByteArray token =
            converter.convertToMime(mime, data, QMetaType::fromType<QByteArray>()).toByteArray();
        QVERIFY(!token.isEmpty());
        QMimeData converted;
        converted.setData(mime, token);
        auto receiver = converter.Read(converted);
        QVERIFY(receiver);
        QMimeData unrelated;
        unrelated.setData(mime, "caller supplied marker");
        QVERIFY(!converter.Read(unrelated));
        QVERIFY(converter.formatsForMime(mime, &converted).isEmpty());
        QVERIFY(!converter.canConvertFromMime(format, &converted));
        model.reset();
        data.Release();
        QVERIFY(!weak.expired());
        converter.Clear();
        QVERIFY(!converter.Read(converted));
        UStringVector names;
        QCOMPARE(receiver->GetNames(names), S_OK);
        QCOMPARE(names.Size(), unsigned(1));
        receiver.reset();
        QVERIFY(weak.expired());
    }
    void retiredMimeCannotClearTheNextNativeSession_data() {
        QTest::addColumn<bool>("retireBeforeRead");
        QTest::newRow("before-next-read") << true;
        QTest::newRow("after-next-read") << false;
    }
    void retiredMimeCannotClearTheNextNativeSession() {
        QFETCH(bool, retireBeforeRead);
        QtNativePanelDragConverter converter;
        const FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        const QString type = converter.mimeForFormat(format);
        auto previous = std::make_unique<QMimeData>();
        {
            auto model = std::make_shared<CDataObject>();
            model->Names.Add(L"C:\\previous δ.txt");
            CMyComPtr<IDataObject> data;
            QCOMPARE(CreatePanelDragDataObject(model, &data), S_OK);
            previous->setData(type,
                              converter.convertToMime(type, data, QMetaType::fromType<QByteArray>())
                                  .toByteArray());
            QVERIFY(converter.Read(*previous));
        }
        auto current = std::make_unique<QMimeData>();
        auto model = std::make_shared<CDataObject>();
        model->Names.Add(L"C:\\current 空 with spaces.txt");
        std::weak_ptr<CDataObject> weak = model;
        CMyComPtr<IDataObject> data;
        QCOMPARE(CreatePanelDragDataObject(model, &data), S_OK);
        current->setData(
            type,
            converter.convertToMime(type, data, QMetaType::fromType<QByteArray>()).toByteArray());
        model.reset();
        data.Release();
        if (retireBeforeRead)
            previous.reset();
        auto receiver = converter.Read(*current);
        QVERIFY(receiver);
        previous.reset();
        UStringVector names;
        QCOMPARE(receiver->GetNames(names), S_OK);
        QCOMPARE(names.Size(), unsigned(1));
        QCOMPARE(ToQString(names[0]), QStringLiteral("C:\\current 空 with spaces.txt"));
        current.reset();
        QVERIFY(!weak.expired());
        receiver.reset();
        QVERIFY(weak.expired());
    }
    void qtPlatformMimeRetainsOriginalNativeProtocol() {
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("windows"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile file(fixture.filePath(QStringLiteral("Qt native перенос δ.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("Qt native control"), qint64(17));
        file.close();
        auto *converter = QtNativePanelDragConverter::ForApplication();
        QVERIFY(converter);
        auto model = std::make_shared<CDataObject>();
        model->Names.Add(ToUString(QDir::toNativeSeparators(file.fileName())));
        CMyComPtr<IDataObject> data;
        QCOMPARE(CreatePanelDragDataObject(model, &data), S_OK);
        QCOMPARE(OleSetClipboard(data), S_OK);
        struct ClearClipboard {
            ~ClearClipboard() { OleSetClipboard(nullptr); }
        } clear;
        // QClipboard exposes Qt's real QWindowsInternalMimeData, the same public
        // conversion path used by QDropEvent. No fabricated Qt private type.
        const QMimeData *mime = QApplication::clipboard()->mimeData();
        QVERIFY(mime);
        QVERIFY(mime->hasUrls());
        QCOMPARE(mime->urls().size(), 1);
        auto native = converter->Read(*mime);
        QVERIFY(native);
        QVERIFY(native->SetTargetFolder(L"C:\\native Qt target δ\\"));
        QCOMPARE(ToQString(model->DestDirPrefix_FromTarget),
                 QStringLiteral("C:\\native Qt target δ\\"));
        UInt32 flags = 0;
        QVERIFY(native->ReadTransferFlags(flags));
        QVERIFY(flags & k_SourceFlags_SetTargetFolder_NonEmpty);
        UStringVector names;
        QCOMPARE(native->GetNames(names), S_OK);
        QCOMPARE(names.Size(), unsigned(1));
        QCOMPARE(QDir::fromNativeSeparators(ToQString(names[0])), file.fileName());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("Qt native control"));
    }
};
int main(int argc, char **argv) {
    QApplication application(argc, argv);
    if (argc == 2 && QByteArray(argv[1]) == "--native-drag-consumer")
        return RunNativeConsumer();
    NativeDragQtTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "native_drag_qt_tests.moc"
