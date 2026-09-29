#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "Windows/PropVariant.h"
#include "filetable.h"
#include "layout_inspection.h"
#include "window_state_support.h"

#include <QHeaderView>

struct CurrentDirectoryGuard {
    const QString previous = QDir::currentPath();
    ~CurrentDirectoryGuard() { QDir::setCurrent(previous); }
};

class RawPropertyFolder final : public IFolderFolder,
                                public IArchiveGetRawProps,
                                public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IArchiveGetRawProps)
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
};

Z7_COM7F_IMF(RawPropertyFolder::LoadItems()) { return S_OK; }
Z7_COM7F_IMF(RawPropertyFolder::GetNumberOfItems(UInt32 *count)) {
    *count = 2;
    return S_OK;
}
Z7_COM7F_IMF(RawPropertyFolder::GetNumberOfProperties(UInt32 *count)) {
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(RawPropertyFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id,
                                                VARTYPE *type)) {
    if (index != 0)
        return E_INVALIDARG;
    *name = nullptr;
    *id = kpidName;
    *type = VT_BSTR;
    return S_OK;
}
Z7_COM7F_IMF(RawPropertyFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (index >= 2)
        return E_INVALIDARG;
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidName)
        prop = index == 0 ? L"z.txt" : L"a.txt";
    else if (id == kpidIsDir)
        prop = false;
    return prop.Detach(value);
}
Z7_COM7F_IMF(RawPropertyFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidType)
        prop = L"7-Zip.raw";
    return prop.Detach(value);
}
Z7_COM7F_IMF(RawPropertyFolder::BindToFolder(UInt32, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(RawPropertyFolder::BindToFolder(const wchar_t *, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(RawPropertyFolder::BindToParentFolder(IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(RawPropertyFolder::GetParent(UInt32, UInt32 *, UInt32 *)) { return E_FAIL; }
Z7_COM7F_IMF(RawPropertyFolder::GetNumRawProps(UInt32 *count)) {
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(RawPropertyFolder::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *id)) {
    if (index != 0)
        return E_INVALIDARG;
    *name = nullptr;
    *id = kpidCRC;
    return S_OK;
}
Z7_COM7F_IMF(RawPropertyFolder::GetRawProp(UInt32 index, PROPID id, const void **data, UInt32 *size,
                                           UInt32 *type)) {
    if (index >= 2 || id != kpidCRC)
        return E_INVALIDARG;
    static const Byte crc[] = {0xde, 0xad, 0xbe, 0xef};
    *data = index == 0 ? nullptr : crc;
    *size = index == 0 ? 0 : UInt32(sizeof(crc));
    *type = NPropDataType::kRaw;
    return S_OK;
}

class PanelColumnsTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void init() {
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.remove("Columns");
    }

    void rawCatalogTextAndSortUseSourceInterface() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CMyComPtr<IFolderFolder> folder = new RawPropertyFolder;
        CPanel panel(folder);
        panel.AttachFolderManager(nullptr, temp.filePath("settings.ini"));
        QCOMPARE(panel.InitColumns(), S_OK);
        const int rawIndex = panel.GetColumns().FindItem_for_PropID(kpidCRC);
        QVERIFY(rawIndex >= 0);
        const CPropColumn &raw = panel.GetColumns()[rawIndex];
        QVERIFY(raw.IsRawProp);
        QCOMPARE(panel.GetItemText(1, raw), QString("DEADBEEF"));
        QCOMPARE(panel.GetItemText(0, raw), QString());
        QVERIFY(panel.CompareItemIndices(0, 1, kpidCRC, true) < 0);
        QVERIFY(panel.CompareItemIndices(0, 1, kpidName) > 0);
    }

    void sourceCatalogAndHeaderSettingsPersist() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(temp.path()));
        QFile file(temp.filePath(QStringLiteral("имя с пробелом.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("1234567890");
        file.close();

        int accessTimeColumn = -1;
        int createdTimeColumn = -1;
        QString accessTimeLabel;
        {
            MainWindow window;
            window.show();
            auto *files = static_cast<FileTable *>(table(window));
            auto *header = files->horizontalHeader();
            accessTimeColumn = files->columnForProperty(kpidATime);
            createdTimeColumn = files->columnForProperty(kpidCTime);
            QVERIFY(accessTimeColumn >= 7);
            QVERIFY(createdTimeColumn >= 7);
            QVERIFY(files->isColumnHidden(accessTimeColumn));
            QVERIFY(!files->isColumnHidden(createdTimeColumn));
            accessTimeLabel = files->horizontalHeaderItem(accessTimeColumn)->text();
            QTimer::singleShot(0, [&] {
                auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                QVERIFY(menu);
                for (QAction *option : menu->actions())
                    if (option->text() == accessTimeLabel)
                        option->trigger();
                menu->close();
            });
            QVERIFY(QMetaObject::invokeMethod(header, "customContextMenuRequested",
                                              Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5))));
            QVERIFY(!files->isColumnHidden(accessTimeColumn));
            bool foundFile = false;
            for (int row = 0; row < files->rowCount(); ++row) {
                if (files->item(row, 0)->text() == QStringLiteral("имя с пробелом.txt")) {
                    foundFile = true;
                    QVERIFY(!files->item(row, accessTimeColumn)->text().isEmpty());
                }
            }
            QVERIFY(foundFile);
            header->resizeSection(accessTimeColumn, 183);
            header->moveSection(header->visualIndex(accessTimeColumn), 1);
            files->sortItems(createdTimeColumn, Qt::DescendingOrder);
            QCOMPARE(header->sortIndicatorSection(), createdTimeColumn);
        }
        {
            MainWindow reopened;
            reopened.show();
            auto *files = static_cast<FileTable *>(table(reopened));
            auto *header = files->horizontalHeader();
            QCOMPARE(files->columnForProperty(kpidATime), accessTimeColumn);
            QCOMPARE(files->columnForProperty(kpidCTime), createdTimeColumn);
            QVERIFY(!files->isColumnHidden(accessTimeColumn));
            QCOMPARE(header->sectionSize(accessTimeColumn), 183);
            QCOMPARE(header->visualIndex(accessTimeColumn), 1);
            QCOMPARE(header->sortIndicatorSection(), createdTimeColumn);
            QCOMPARE(header->sortIndicatorOrder(), Qt::DescendingOrder);
            for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
                reopened.resize(size);
                for (int mode = 0; mode < 4; ++mode) {
                    auto *modeAction =
                        reopened.findChild<QAction *>(QStringLiteral("viewMode%1Action").arg(mode));
                    QVERIFY(modeAction);
                    modeAction->trigger();
                    QCoreApplication::processEvents();
                    QVERIFY(UiInspection::fullyVisible(*reopened.centralWidget(), reopened));
                    QVERIFY(UiInspection::writeReport(
                        reopened,
                        QStringLiteral("source-columns-%1-%2").arg(size.width()).arg(mode)));
                }
            }
        }
    }

    void folderTypesKeepIndependentLayouts() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(temp.path()));
        QFile file(temp.filePath("local.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("disk");
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *files = static_cast<FileTable *>(table(window));
        auto *header = files->horizontalHeader();
        const int diskSize = files->columnForProperty(kpidSize);
        QVERIFY(!files->isColumnHidden(diskSize));
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            for (QAction *option : menu->actions())
                if (option->text() == files->horizontalHeaderItem(diskSize)->text())
                    option->trigger();
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(header, "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5))));
        QVERIFY(files->isColumnHidden(diskSize));

        openListedArchive(window, backend, temp.filePath("sample.7z"),
                          {{QStringLiteral("архив/файл с пробелом.txt"), 45, 18}});
        QCOMPARE(files->columnForProperty(kpidPackSize), 3);
        QVERIFY(!files->isColumnHidden(3));
        QVERIFY(files->columnForProperty(kpidNumSubFiles) >= 7);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        QVERIFY(address);
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QVERIFY(files->isColumnHidden(diskSize));
    }
};

QTEST_MAIN(PanelColumnsTests)
#include "panel_columns_tests.moc"
