#include "operation_dialogs.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/GUI/ExtractDialog.h"
#include "layout_inspection.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>

// Public widget interactions in the fixture process only. Production 7zG owns
// these same source dialogs; no automation flags are added to its entrypoint.
void DriveOperationDialogs() {
    for (auto *widget : QApplication::topLevelWidgets()) {
        if (!widget->isVisible())
            continue;
        if (auto *extract = qobject_cast<CExtractDialog *>(widget)) {
            const auto report = qEnvironmentVariable("SEVENZIP_OPERATION_DIALOG_REPORT");
            if (!report.isEmpty()) {
                QSaveFile file(report);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(
                        QJsonDocument(QJsonObject{{"title", extract->windowTitle()}}).toJson());
                    file.commit();
                }
                const auto tag = qEnvironmentVariable("SEVENZIP_OPERATION_DIALOG_TAG");
                if (!tag.isEmpty())
                    for (const QSize size : {QSize(700, 390), QSize(600, 340)}) {
                        extract->resize(size);
                        QApplication::processEvents();
                        UiInspection::writeReport(*extract,
                                                  tag + '-' + QString::number(size.width()));
                    }
            }
            const auto decision = qEnvironmentVariable("SEVENZIP_OPERATION_EXTRACT_DECISION");
            if (decision == "cancel") {
                extract->reject();
            } else if (decision == "accept") {
                const QString destination = qEnvironmentVariable("SEVENZIP_OPERATION_DESTINATION");
                if (!destination.isEmpty()) {
                    extract->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
                    extract->findChild<QComboBox *>("extractDestination")->setEditText(destination);
                }
                extract->accept();
            }
        } else if (widget->objectName() == "passwordDialog") {
            auto *dialog = qobject_cast<QDialog *>(widget);
            if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_CANCEL_PASSWORD"))
                dialog->reject();
            else if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_PASSWORD")) {
                dialog->findChild<QLineEdit *>("passwordEdit")
                    ->setText(qEnvironmentVariable("SEVENZIP_OPERATION_PASSWORD"));
                dialog->accept();
            }
        } else if (widget->objectName() == "overwriteDialog") {
            const auto button = qEnvironmentVariable("SEVENZIP_OPERATION_OVERWRITE");
            if (auto *choice = widget->findChild<QPushButton *>(button))
                choice->click();
        } else if (auto *message = qobject_cast<QMessageBox *>(widget))
            message->accept();
        else if (widget->objectName() == "progressDialog") {
            auto *close = widget->findChild<QPushButton *>("IDCANCEL");
            if (close && close->property("languageID").toUInt() == IDS_CLOSE)
                close->click();
        }
    }
}
