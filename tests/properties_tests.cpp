#include "mainwindow.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class PropertiesTests : public QObject {
    Q_OBJECT

  private slots:
    void selectedFilesAndFolderShowSummary() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString original = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temporary.path()));
        for (const auto &name :
             {QStringLiteral("один файл.txt"), QStringLiteral("два файла.txt")}) {
            QFile file(temporary.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("hello"), qint64(5));
        }
        QVERIFY(QDir().mkdir(QStringLiteral("папка")));
        MainWindow window;
        window.show();
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("selectedPropertiesAction");
        QVERIFY(table && action);
        QCOMPARE(table->rowCount(), 3);
        table->selectAll();
        QVERIFY(action->isEnabled());
        QStringList labels;
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
            QCOMPARE(dialog->windowTitle(), QString("Свойства выделения"));
            for (QLabel *label : dialog->findChildren<QLabel *>())
                labels << label->text();
            dialog->reject();
        });
        action->trigger();
        QVERIFY(labels.contains("Выбрано:"));
        QVERIFY(labels.contains("3"));
        QVERIFY(labels.contains("Файлов:"));
        QVERIFY(labels.contains("2"));
        QVERIFY(labels.contains("Папок:"));
        QVERIFY(labels.contains("1"));
        QVERIFY(labels.contains("Размер выбранных файлов:"));
        QVERIFY(labels.contains("10"));
        QVERIFY(QDir::setCurrent(original));
    }

    void archiveSummaryIncludesPackedSize() {
        PropertiesDialog dialog(
            {{"a.txt", "a.txt", {}, false, 10, 4}, {"b.txt", "b.txt", {}, false, 5, 2}}, true,
            "archive.7z");
        QStringList labels;
        for (QLabel *label : dialog.findChildren<QLabel *>())
            labels << label->text();
        QVERIFY(labels.contains("Сжатый размер выбранных файлов:"));
        QVERIFY(labels.contains("6"));
        QVERIFY(labels.contains("15"));
    }
};

QTEST_MAIN(PropertiesTests)
#include "properties_tests.moc"
