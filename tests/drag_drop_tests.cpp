#include "CPP/7zip/UI/FileManager/App.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "qtpaneldragmodel.h"
#include "qtpaneldragtransport.h"
#include <QDrag>
#include <QFile>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class Presentation final : public IQtDropTargetPresentation {
  public:
    int hit = 1, folder = -1;
    bool enabled = true, sameDrive = true;
    QList<QPair<int, bool>> highlights;
    int PanelAtPoint(int, int) const override { return hit; }
    bool PanelEnabled(unsigned) const override { return enabled; }
    int FolderAtPoint(unsigned, int, int, UString &name) const override {
        if (folder >= 0)
            name = L"подпапка с пробелом";
        return folder;
    }
    void Highlight(unsigned, int item, bool selected) override {
        highlights.append({item, selected});
    }
    bool IsSameDrive(const UString &, const UStringVector &) const override { return sameDrive; }
};
void BindDisk(CPanel &panel, const QString &path) {
    panel.SetNewFolder(new NFsFolder::CFSFolder(path));
    panel.LoadFullPath();
}
} // namespace

class DragDropTests : public QObject {
    Q_OBJECT
  private slots:
    void nativeInternalDropCopiesOrMovesThroughSourceCopyTo_data() {
        QTest::addColumn<bool>("move");
        QTest::newRow("control-copy") << false;
        QTest::newRow("shift-move") << true;
    }
    void nativeInternalDropCopiesOrMovesThroughSourceCopyTo() {
        QFETCH(bool, move);
        QTemporaryDir sourceRoot, targetRoot;
        const QString name = "между панелями.txt";
        QFile file(sourceRoot.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("panel bytes"), qint64(11));
        file.close();
        Presentation presentation;
        CApp app;
        BindDisk(app.Panels[0], sourceRoot.path());
        BindDisk(app.Panels[1], targetRoot.path());
        QCOMPARE(app.Panels[0].GetFolder()->LoadItems(), S_OK);
        app.m_PanelCallbackImp[0].Init(&app, 0);
        app.Panels[0].AttachPanelCallback(&app.m_PanelCallbackImp[0]);
        app.DropTarget().AttachPresentation(&presentation);
        QTableWidget list(1, 1);
        list.setSelectionBehavior(QAbstractItemView::SelectRows);
        auto *row = new QTableWidgetItem(name);
        row->setData(Qt::UserRole + 6, 0u);
        list.setItem(0, 0, row);
        list.setCurrentCell(0, 0);
        app.Panels[0].AttachSelectionView(&list);
        HRESULT entered = E_FAIL, dropped = E_FAIL;
        DWORD reported = 0;
        QtPanelDragTransport transport(&list, [&](QDrag &drag, Qt::DropActions) {
            auto *mime = dynamic_cast<const QtPanelDragMime *>(drag.mimeData());
            if (!mime)
                return Qt::IgnoreAction;
            const DWORD key = move ? MK_SHIFT : MK_CONTROL;
            DWORD effect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
            entered = app.DropTarget().DragEnter(&mime->Data(), key, 0, 0, &effect);
            effect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
            dropped = app.DropTarget().Drop(&mime->Data(), key, 0, 0, &effect);
            reported = effect;
            return effect == DROPEFFECT_MOVE ? Qt::MoveAction : Qt::CopyAction;
        });
        app.Panels[0].AttachDragServices(&transport);
        app.Panels[0].OnDrag(false);
        QCOMPARE(entered, S_OK);
        QCOMPARE(dropped, S_OK);
        QCOMPARE(reported, DWORD(move ? DROPEFFECT_MOVE : DROPEFFECT_COPY));
        QFile copied(targetRoot.filePath(name));
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("panel bytes"));
        QCOMPARE(QFileInfo::exists(sourceRoot.filePath(name)), !move);
        QVERIFY(!row->isSelected());
        QCOMPARE(app.DropTarget().SrcPanelIndex, -1);
    }
    void nativeDiskDragDeliversSelectedBytesAndKeepsSelectionOnCancel() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString file = root.filePath("перенос с пробелом.txt");
        QFile original(file);
        QVERIFY(original.open(QIODevice::WriteOnly));
        QCOMPARE(original.write("selected bytes"), qint64(14));
        original.close();
        CApp app;
        BindDisk(app.Panels[0], root.path());
        QCOMPARE(app.Panels[0].GetFolder()->LoadItems(), S_OK);
        app.m_PanelCallbackImp[0].Init(&app, 0);
        app.Panels[0].AttachPanelCallback(&app.m_PanelCallbackImp[0]);
        QTableWidget list(1, 1);
        list.setSelectionBehavior(QAbstractItemView::SelectRows);
        auto *row = new QTableWidgetItem("перенос с пробелом.txt");
        row->setData(Qt::UserRole + 6, 0u);
        list.setItem(0, 0, row);
        app.Panels[0].AttachSelectionView(&list);
        list.setCurrentCell(0, 0);
        class Recipient final : public IQtPanelDragServices {
          public:
            CApp &app;
            bool cancel = true;
            int calls = 0;
            QString path;
            QByteArray bytes;
            bool sourceStateCorrect = false;
            explicit Recipient(CApp &owner) : app(owner) {}
            HRESULT DoDragDrop(CDataObject &data, CDropSource &, DWORD allowed,
                               DWORD &effect) override {
                ++calls;
                sourceStateCorrect = app.DropTarget().SrcPanelIndex == 0 &&
                                     app.DropTarget().TargetPanelIndex == 1 &&
                                     allowed == (DROPEFFECT_COPY | DROPEFFECT_MOVE);
                if (cancel)
                    return DRAGDROP_S_CANCEL;
                if (data.Names.Size() != 1 || data.IsTempFiles)
                    return E_FAIL;
                path = ToQString(data.Names[0]);
                QFile file(path);
                if (!file.open(QIODevice::ReadOnly))
                    return E_FAIL;
                bytes = file.readAll();
                effect = DROPEFFECT_COPY;
                return DRAGDROP_S_DROP;
            }
        } receiver(app);
        app.Panels[0].AttachDragServices(&receiver);
        app.Panels[0].OnDrag(false);
        QCOMPARE(receiver.calls, 1);
        QVERIFY(receiver.sourceStateCorrect);
        QVERIFY(list.item(0, 0)->isSelected());
        QCOMPARE(app.DropTarget().SrcPanelIndex, -1);
        QVERIFY(app.Panels[0].ShouldProcessNotifications());
        receiver.cancel = false;
        app.Panels[0].OnDrag(false);
        QCOMPARE(receiver.calls, 2);
        QCOMPARE(receiver.bytes, QByteArray("selected bytes"));
        QCOMPARE(QDir::fromNativeSeparators(receiver.path).normalized(QString::NormalizationForm_C),
                 file.normalized(QString::NormalizationForm_C));
        QVERIFY(!list.item(0, 0)->isSelected());
        QVERIFY(QFileInfo::exists(file));
        QCOMPARE(app.DropTarget().SrcPanelIndex, -1);
        QVERIFY(app.Panels[0].ShouldProcessNotifications());
    }
    void targetTracksOppositePanelAndRejectsSource() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        Presentation view;
        CApp app;
        BindDisk(app.Panels[0], root.path());
        BindDisk(app.Panels[1], root.path());
        auto &target = app.DropTarget();
        target.AttachPresentation(&view);
        app.SetFocusedPanel(0);
        app.DragBegin(0);
        QCOMPARE(target.TargetPanelIndex, 1);
        QCOMPARE(target.SrcPanelIndex, 0);
        UStringVector names;
        names.Add(L"source файл.txt");
        target.LoadNames_From_DataObject(names, true);
        view.hit = 0;
        target.PositionCursor(0, 0);
        QCOMPARE(target.GetEffect(0, DROPEFFECT_COPY | DROPEFFECT_MOVE), DWORD(0));
        view.hit = 1;
        view.folder = 7;
        target.PositionCursor(0, 0);
        QCOMPARE(target.TargetPanel(), &app.Panels[1]);
        QVERIFY(QDir::fromNativeSeparators(ToQString(target.GetTargetPath()))
                    .endsWith("подпапка с пробелом/"));
        QCOMPARE(view.highlights, (QList<QPair<int, bool>>{{7, true}}));
        view.folder = -1;
        target.PositionCursor(0, 0);
        QCOMPARE(view.highlights.last(), (QPair<int, bool>{7, false}));
        QCOMPARE(ToQString(target.GetTargetPath()), ToQString(app.Panels[1].GetFsPath()));
        view.hit = -1;
        target.PositionCursor(0, 0);
        QVERIFY(target.IsAppTarget());
        QVERIFY(target.GetTargetPath().IsEmpty());
        app.SetFocusedPanel(1);
        app.DragEnd();
        QCOMPARE(target.TargetPanelIndex, 1);
        QCOMPARE(target.SrcPanelIndex, -1);
        target.ClearState();
        QCOMPARE(target.GetEffect(MK_SHIFT, DROPEFFECT_MOVE), DWORD(0));
        app.NumPanels = 1;
        app.DragBegin(1);
        QCOMPARE(target.TargetPanelIndex, 1);
        app.DragEnd();
    }
    void diskModifiersAndVolumeFallback() {
        QTemporaryDir root;
        Presentation view;
        CApp app;
        BindDisk(app.Panels[1], root.path());
        auto &target = app.DropTarget();
        target.AttachPresentation(&view);
        UStringVector paths;
        paths.Add(L"source.txt");
        target.LoadNames_From_DataObject(paths, true);
        target.PositionCursor(0, 0);
        const DWORD both = DROPEFFECT_COPY | DROPEFFECT_MOVE;
        QCOMPARE(target.GetEffect(0, both), DWORD(DROPEFFECT_MOVE));
        QCOMPARE(target.GetEffect(MK_CONTROL, both), DWORD(DROPEFFECT_COPY));
        QCOMPARE(target.GetEffect(MK_SHIFT, both), DWORD(DROPEFFECT_MOVE));
        QCOMPARE(target.GetEffect(MK_ALT, both), DWORD(0));
        QCOMPARE(target.GetEffect(MK_CONTROL | MK_SHIFT, both), DWORD(0));
        view.sameDrive = false;
        QCOMPARE(target.GetEffect(0, both), DWORD(DROPEFFECT_COPY));
        QCOMPARE(target.GetEffect(MK_CONTROL | MK_ALT, both), DWORD(DROPEFFECT_COPY));
        QCOMPARE(target.GetEffect(MK_SHIFT, DROPEFFECT_COPY), DWORD(DROPEFFECT_COPY));
        QCOMPARE(target.GetEffect(0, DROPEFFECT_MOVE), DWORD(0));
    }
    void sourceCancelAndArchivePreparationAreDistinct() {
        CDataObject archive;
        archive.IsTempFiles = true;
        archive.UsePreGlobal = true;
        CDropSource source;
        source.DataObjectSpec = &archive;
        QCOMPARE(source.QueryContinueDrag(false, MK_LBUTTON), S_OK);
        QCOMPARE(source.GiveFeedback(DROPEFFECT_COPY), DRAGDROP_S_USEDEFAULTCURSORS);
        QCOMPARE(source.QueryContinueDrag(false, 0), DRAGDROP_S_DROP);
        QVERIFY(archive.NeedCall_Copy);
        QVERIFY(!archive.UsePreGlobal);
        QVERIFY(!archive.DoNotProcessInTarget);
        CDataObject internal;
        internal.IsTempFiles = true;
        internal.DestDirPrefix_FromTarget = L"destination/";
        CDropSource direct;
        direct.DataObjectSpec = &internal;
        direct.GiveFeedback(DROPEFFECT_MOVE);
        QCOMPARE(direct.QueryContinueDrag(false, 0), DRAGDROP_S_DROP);
        QVERIFY(internal.DoNotProcessInTarget);
        QVERIFY(!internal.NeedCall_Copy);
        CDataObject cancelled;
        cancelled.IsTempFiles = true;
        CDropSource escape;
        escape.DataObjectSpec = &cancelled;
        escape.GiveFeedback(DROPEFFECT_COPY);
        QCOMPARE(escape.QueryContinueDrag(true, MK_LBUTTON), DRAGDROP_S_CANCEL);
        QCOMPARE(escape.QueryContinueDrag(false, 0), DRAGDROP_S_CANCEL);
        QVERIFY(!cancelled.NeedCall_Copy);
        CDropSource wrongButton;
        wrongButton.DataObjectSpec = &cancelled;
        QCOMPARE(wrongButton.QueryContinueDrag(false, MK_RBUTTON), DRAGDROP_S_CANCEL);
        CDropSource noTarget;
        noTarget.DataObjectSpec = &cancelled;
        QCOMPARE(noTarget.QueryContinueDrag(false, 0), DRAGDROP_S_CANCEL);
    }
};
QTEST_MAIN(DragDropTests)
#include "drag_drop_tests.moc"
