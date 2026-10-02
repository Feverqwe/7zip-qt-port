#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class BrowseDialogTests final : public QObject {
    Q_OBJECT
  private slots:
    void folderCancelAndUnicodeSelection() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString selected = fixture.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QDir().mkdir(selected));
        QString result = QStringLiteral("stale");
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget()))
                dialog->reject();
        });
        QVERIFY(!MyBrowseForFolder(nullptr, "Папка", fixture.path(), result));
        QVERIFY(result.isEmpty());
        QTimer::singleShot(0, [selected] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                dialog->setDirectory(selected);
                QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
            }
        });
        QVERIFY(MyBrowseForFolder(nullptr, "Папка", fixture.path(), result));
        QCOMPARE(result, selected);
    }

    void firstArchiveVolumeUsesArchiveFilter_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("archives");
        QTest::addColumn<QString>("allFiles");
        QTest::newRow("English") << QString("-") << QString("Archives") << QString("All files");
        QTest::newRow("Russian") << QString("ru") << QStringLiteral("Архивы")
                                 << QStringLiteral("Все файлы");
    }
    void firstArchiveVolumeUsesArchiveFilter() {
        QFETCH(QString, language);
        QFETCH(QString, archives);
        QFETCH(QString, allFiles);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("language.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        const QString selected = fixture.filePath(QStringLiteral("архив с пробелом.7z.001"));
        QFile file(selected);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        CBrowseInfo browse;
        browse.Title = "Открыть архив";
        browse.Filters = archives + " (*.7z *.zip *.001);;" + allFiles + " (*)";
        browse.FilePath = fixture.path() + '/';
        bool answered = false;
        QStringList observedFilters;
        QString selectedFilter;
        QTimer::singleShot(0, [&] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                answered = true;
                selectedFilter = dialog->selectedNameFilter();
                observedFilters = dialog->nameFilters();
                dialog->selectFile(selected);
                QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
            }
        });
        QVERIFY(browse.BrowseForFile());
        QVERIFY(answered);
        QVERIFY(selectedFilter.contains("*.001"));
        QCOMPARE(observedFilters.size(), 2);
        QVERIFY(observedFilters.at(0).startsWith(archives + " ("));
        QCOMPARE(observedFilters.at(1), allFiles + " (*)");
        QCOMPARE(browse.FilePath, selected);
        QCOMPARE(browse.FilterIndex, 0);
        SaveRegLang(UString("-"), settings);
        ReloadLang(settings);
    }
    void fileCancelRestoresPathThenSelectsUnicodeFile() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString selected = fixture.filePath(QStringLiteral("архив с пробелом.7z"));
        QFile file(selected);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        CBrowseInfo browse;
        browse.Title = "Открыть архив";
        browse.Filters = "Архивы (*.7z);;Все файлы (*)";
        browse.FilePath = selected;
        browse.FilterIndex = 1;
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget()))
                dialog->reject();
        });
        QVERIFY(!browse.BrowseForFile());
        QCOMPARE(browse.FilePath, selected);
        QTimer::singleShot(0, [selected] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                dialog->selectFile(selected);
                QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
            }
        });
        QVERIFY(browse.BrowseForFile());
        QCOMPARE(browse.FilePath, selected);
        QCOMPARE(browse.FilterIndex, 1);
    }
};

QTEST_MAIN(BrowseDialogTests)
#include "browse_dialog_tests.moc"
