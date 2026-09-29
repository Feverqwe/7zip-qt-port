#include "7zip/PropID.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "Windows/PropVariant.h"
#include "qtarchivefileresource.h"
#include "qtarchiveproperties.h"
#include "qtlocationfolder.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class AgentCapabilityTests : public QObject {
    Q_OBJECT
    static void expect(IFolderFolder *folder, PROPID id, bool expected) {
        NWindows::NCOM::CPropVariant prop;
        QCOMPARE(folder->GetFolderProperty(id, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_BOOL));
        QCOMPARE(prop.boolVal != VARIANT_FALSE, expected);
    }

  private slots:
    void nativeReadOnlyHashAndTypedUpdateCapability() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath(QStringLiteral("архив с пробелом.zip"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ArchiveBackend backend(nullptr, QStringLiteral("/missing/7zz"));
        auto *resourceSpec = new QtArchiveFileResource(path);
        CMyComPtr<IUnknown> resource = resourceSpec;
        CMyComPtr<IFolderFolder> metadata;
        const QList<ArchiveEntry> entries = {{QStringLiteral("папка/имя.txt"), 7, 3}};
        QCOMPARE(CreateArchiveMetadata(entries, &metadata), S_OK);
        auto *agentSpec = new CAgent;
        CMyComPtr<IUnknown> agent = agentSpec;
        QCOMPARE(agentSpec->Init(entries, metadata, UString("zip"), resource), S_OK);
        CMyComPtr<IFolderFolder> root;
        QCOMPARE(agentSpec->BindToRootFolder(&root), S_OK);
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(root->BindToFolder(L"папка", &child), S_OK);
        // Missing engine writer is authoritative, even for a .zip filename.
        QVERIFY(!ArchiveUpdateEnabled(agentSpec->GetArchive()));
        expect(root, kpidReadOnly, true);
        expect(child, kpidReadOnly, true);
        expect(root, kpidIsHash, false);
        resourceSpec->AttachUpdateService(backend, QStringLiteral("zip"));
        QVERIFY(ArchiveUpdateEnabled(agentSpec->GetArchive()));
        expect(root, kpidReadOnly, false);
        expect(child, kpidReadOnly, false);
        agentSpec->_proxy->Are_Changed_LongPaths = true;
        expect(child, kpidReadOnly, true);
        agentSpec->_proxy->Are_Changed_LongPaths = false;
        agentSpec->_archiveLink.Arcs[0].IsReadOnly = true;
        expect(child, kpidReadOnly, true);
        agentSpec->_archiveLink.Arcs[0].IsReadOnly = false;
        agentSpec->_attrib = FILE_ATTRIBUTE_READONLY;
        expect(child, kpidReadOnly, true);
        agentSpec->_attrib = (DWORD)-1;
        expect(child, kpidReadOnly, false);
        agentSpec->_isHashHandler = true;
        expect(child, kpidIsHash, true);
        QVERIFY(!backend.busy());
    }

    void fileAttributesComeFromNativeFileFindAtOpen() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath(QStringLiteral("readonly й.zip"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        QVERIFY(file.setPermissions(QFileDevice::ReadOwner));
        auto *resourceSpec = new QtArchiveFileResource(path);
        CMyComPtr<IUnknown> resource = resourceSpec;
        CMyComPtr<IFolderFolder> metadata;
        QCOMPARE(CreateArchiveMetadata({}, &metadata), S_OK);
        CAgent agent;
        QCOMPARE(agent.Init({}, metadata, UString("zip"), resource), S_OK);
        QVERIFY(agent.Is_Attrib_ReadOnly());
        QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    }
};

QTEST_GUILESS_MAIN(AgentCapabilityTests)
#include "agent_capability_tests.moc"
