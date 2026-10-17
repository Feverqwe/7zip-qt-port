#include "split_dialog_prompt.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "layout_inspection.h"
#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTest>
#include <QTimer>

SplitPromptObservation SubmitInvalidSplit(CSplitDialog &dialog, const QString &report,
                                          bool escape) {
    SplitPromptObservation observed;
    QTimer responder;
    responder.setSingleShot(true);
    QObject::connect(&responder, &QTimer::timeout, &responder, [&] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!box)
            return;
        observed.shown = true;
        observed.text = box->text();
        const auto *ok = qobject_cast<QPushButton *>(box->button(QMessageBox::Ok));
        observed.sourcePolicy =
            box->icon() == QMessageBox::Critical && box->standardButtons() == QMessageBox::Ok &&
            box->textFormat() == Qt::PlainText && ok && ok->isDefault() &&
            observed.text == QString::fromWCharArray(LangString(IDS_INCORRECT_VOLUME_SIZE));
#ifdef Q_OS_MACOS
        observed.sourcePolicy &= box->windowTitle().isEmpty();
#else
        observed.sourcePolicy &= box->windowTitle() == QStringLiteral("7-Zip");
#endif
        observed.geometry = true;
        for (const auto *widget : box->findChildren<QWidget *>()) {
            if (!widget->isVisible())
                continue;
            observed.geometry &= UiInspection::fullyVisible(*widget, *box);
            if (const auto *label = qobject_cast<const QLabel *>(widget))
                if (!label->text().isEmpty())
                    observed.geometry &= UiInspection::labelFits(*label);
        }
        if (!report.isEmpty())
            observed.geometry &= UiInspection::writeReport(*box, report);
        if (escape)
            QTest::keyClick(box, Qt::Key_Escape);
        else
            box->button(QMessageBox::Ok)->click();
    });
    responder.start(0);
    dialog.accept();
    responder.stop();
    return observed;
}
