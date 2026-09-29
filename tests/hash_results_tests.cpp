#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "CPP/7zip/UI/GUI/HashGUI.h"
#include "layout_inspection.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

class HashResultsTests final : public QObject {
    Q_OBJECT
  private slots:
    void nativeBundleResultsAndModalLayout() {
        CHashBundle hash;
        UStringVector methods;
        methods.Add(L"CRC32");
        QCOMPARE(hash.SetMethods(methods), S_OK);
        hash.FirstFileName = L"имя с пробелом.bin";
        hash.InitForNewFile();
        hash.Update("123456789", 9);
        hash.SetSize(9);
        hash.Final(false, false, hash.FirstFileName);
        CPropNameValPairs pairs;
        AddHashBundleRes(pairs, hash);
        QVERIFY(!pairs.IsEmpty());
        bool foundDigest = false;
        FOR_VECTOR(i, pairs)
        if (pairs[i].Value.IsEqualTo("CBF43926"))
            foundDigest = true;
        QVERIFY(foundDigest);
        for (int width : {540, 420}) {
            bool inspected = false, valid = false, report = false;
            QString details;
            QTimer::singleShot(0, qApp, [&] {
                auto *dialog = qobject_cast<CListViewDialog *>(QApplication::activeModalWidget());
                if (!dialog)
                    return;
                inspected = true;
                dialog->resize(width, 360);
                QApplication::processEvents();
                auto *list = dialog->findChild<QTreeWidget *>("propertiesList");
                auto *buttons = dialog->findChild<QDialogButtonBox *>("listViewButtons");
                details = QString("list=%1 buttons=%2 rows=%3 selected=%4 delete=%5 title=%6")
                              .arg(bool(list))
                              .arg(bool(buttons))
                              .arg(list ? list->topLevelItemCount() : -1)
                              .arg(list ? list->selectedItems().size() : -1)
                              .arg(dialog->DeleteIsAllowed)
                              .arg(dialog->windowTitle());
                valid = list && buttons && list->columnCount() == 2 &&
                        list->topLevelItemCount() == int(pairs.Size()) &&
                        list->selectedItems().isEmpty() && dialog->DeleteIsAllowed &&
                        dialog->windowTitle() == QStringLiteral("Контрольные суммы");
                if (list && buttons) {
                    details +=
                        QString(
                            " total=%1 columns=%2 listVisible=%3 buttonsVisible=%4 separated=%5")
                            .arg(pairs.Size())
                            .arg(list->columnCount())
                            .arg(UiInspection::fullyVisible(*list, *dialog))
                            .arg(UiInspection::fullyVisible(*buttons, *dialog))
                            .arg(UiInspection::separate(*list, *buttons, *dialog));
                    FOR_VECTOR(i, pairs)
                    valid = valid && list->topLevelItem(i)->text(0) == ToQString(pairs[i].Name) &&
                            list->topLevelItem(i)->text(1) == ToQString(pairs[i].Value);
                    valid = valid && UiInspection::fullyVisible(*list, *dialog) &&
                            UiInspection::fullyVisible(*buttons, *dialog) &&
                            UiInspection::separate(*list, *buttons, *dialog);
                    for (auto *button : buttons->findChildren<QPushButton *>()) {
                        details += QString(" button=%1 visible=%2 fits=%3")
                                       .arg(button->text())
                                       .arg(UiInspection::fullyVisible(*button, *dialog))
                                       .arg(button->sizeHint().width() <= button->width());
                        valid = valid && UiInspection::fullyVisible(*button, *dialog) &&
                                button->sizeHint().width() <= button->width() &&
                                button->sizeHint().height() <= button->height();
                    }
                }
                report = UiInspection::writeReport(*dialog, QString("hash-results-%1").arg(width));
                dialog->reject();
            });
            ShowHashResults(hash, nullptr);
            QVERIFY(inspected);
            QVERIFY2(valid, qPrintable(details));
            QVERIFY(report);
        }
    }
};
QTEST_MAIN(HashResultsTests)
#include "hash_results_tests.moc"
