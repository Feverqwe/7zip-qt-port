#include "filemanager/AppState.h"
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class AppStateTests : public QObject {
    Q_OBJECT
  private slots:
    void independentInstancesAndCombinedPersistence() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString firstPath = temp.filePath("first.ini");
        const QString secondPath = temp.filePath("second.ini");
        const UString firstName = ToUString(QStringLiteral("/Папка с пробелом/😀"));
        CAppState first;
        CAppState second;
        first.FastFolders.SetString(7, firstName);
        first.FolderHistory.AddString(firstName);
        second.FastFolders.SetString(2, UString("/other"));
        second.FolderHistory.AddString(UString("/other"));
        QVERIFY(second.FastFolders.GetString(7).IsEmpty());
        QVERIFY(first.FastFolders.GetString(2).IsEmpty());
        first.Save(firstPath);
        second.Save(secondPath);

        CAppState restored;
        restored.Read(firstPath);
        QCOMPARE(ToQString(restored.FastFolders.GetString(7)), ToQString(firstName));
        UStringVector history;
        restored.FolderHistory.GetList(history);
        QCOMPARE(history.Size(), 1u);
        QCOMPARE(ToQString(history[0]), ToQString(firstName));
        restored.Read(secondPath);
        QVERIFY(restored.FastFolders.GetString(7).IsEmpty());
        QCOMPARE(ToQString(restored.FastFolders.GetString(2)), QString("/other"));
        restored.FolderHistory.GetList(history);
        QCOMPARE(history.Size(), 1u);
        QCOMPARE(ToQString(history[0]), QString("/other"));
    }

    void readNormalizesHistoryAndPreservesOtherSettings() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = temp.filePath("state.ini");
        {
            QSettings settings(path, QSettings::IniFormat);
            QStringList paths;
            for (int i = 0; i < 120; ++i)
                paths.append(QString("/folder/%1").arg(i));
            settings.setValue("FolderHistory", paths);
            settings.setValue("unrelated", "keep");
        }
        CAppState state;
        state.Read(path);
        UStringVector history;
        state.FolderHistory.GetList(history);
        QCOMPARE(history.Size(), 100u);
        state.FolderHistory.RemoveAll();
        state.Save(path);
        CAppState restored;
        restored.Read(path);
        restored.FolderHistory.GetList(history);
        QVERIFY(history.IsEmpty());
        QVERIFY(restored.FastFolders.GetString(0).IsEmpty());
        QSettings settings(path, QSettings::IniFormat);
        QCOMPARE(settings.value("unrelated").toString(), QString("keep"));
    }
};

QTEST_GUILESS_MAIN(AppStateTests)
#include "app_state_tests.moc"
