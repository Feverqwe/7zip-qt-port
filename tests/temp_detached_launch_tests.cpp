#include "CPP/7zip/UI/FileManager/BrowseDialog2.h"
#include "qtownedtempdir.h"
#include <QContextMenuEvent>
#include <QFile>
#include <QMenu>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

class TempDetachedLaunchTests final : public QObject {
    Q_OBJECT
  private slots:
    void directoryLaunchPassesExactArgumentAndKeepsExternalProtection() {
        QTemporaryDir root(QDir::tempPath() + QStringLiteral("/детач 空 с пробелом-XXXXXX"));
        QVERIFY(root.isValid());
        const QByteArray expected = QFileInfo(root.path()).canonicalFilePath().toUtf8();
        {
            auto lease = QtOwnedTempRegistry().RegisterCreated(root.path());
            QVERIFY(lease);
        }
        bool selected = false, enabled = false;
        QTimer::singleShot(0, qApp, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            auto *list = dialog->findChild<QTreeWidget *>("tempList");
            for (int index = 0; index < list->topLevelItemCount(); ++index) {
                auto *item = list->topLevelItem(index);
                if (item->text(0).normalized(QString::NormalizationForm_C) ==
                    QFileInfo(root.path()).fileName().normalized(QString::NormalizationForm_C)) {
                    list->setCurrentItem(item);
                    item->setSelected(true);
                    selected = true;
                    break;
                }
            }
            if (!selected) {
                dialog->reject();
                return;
            }
            QTimer choose;
            connect(&choose, &QTimer::timeout, dialog, [&] {
                auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                if (!menu)
                    return;
                for (QAction *action : menu->actions()) {
                    if (action->data().toUInt() == 3) {
                        enabled = action->isEnabled();
                        if (enabled) {
                            menu->setActiveAction(action);
                            QTest::keyClick(menu, Qt::Key_Return);
                        } else
                            menu->close();
                        return;
                    }
                }
                menu->close();
            });
            choose.start(1);
            const QPoint point = list->visualItemRect(list->currentItem()).center();
            QContextMenuEvent context(QContextMenuEvent::Mouse, point,
                                      list->viewport()->mapToGlobal(point));
            QApplication::sendEvent(list->viewport(), &context);
            choose.stop();
            dialog->reject();
        });
        MyBrowseForTempFolder(nullptr);
        QVERIFY(selected && enabled);
        QFile receipt(root.filePath("received-argument.txt"));
        QTRY_VERIFY_WITH_TIMEOUT(receipt.exists(), 10000);
        QVERIFY(receipt.open(QIODevice::ReadOnly));
        QCOMPARE(QDir::fromNativeSeparators(QString::fromUtf8(receipt.readAll()))
                     .normalized(QString::NormalizationForm_C),
                 QDir::fromNativeSeparators(QString::fromUtf8(expected))
                     .normalized(QString::NormalizationForm_C));
        bool protectedEntry = false;
        for (const auto &entry : QtOwnedTempRegistry().Entries())
            if (QFileInfo(entry.path).canonicalFilePath() == QString::fromUtf8(expected))
                protectedEntry = entry.externallyProtected;
        QVERIFY(protectedEntry); // Detached exit supplies no confirmed process lifetime.
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    // Native launch receiver fixture, distinct from the application's startup
    // path consumer tested in startup_path. No user path is accepted here.
    if (app.arguments().size() == 2 && QFileInfo(app.arguments().at(1)).isDir()) {
        const QString path = app.arguments().at(1);
        QFile receipt(QDir(path).filePath("received-argument.txt"));
        const QByteArray bytes = path.toUtf8();
        return receipt.open(QIODevice::WriteOnly) && receipt.write(bytes) == bytes.size() ? 0 : 1;
    }
    TempDetachedLaunchTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "temp_detached_launch_tests.moc"
