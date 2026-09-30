#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
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
