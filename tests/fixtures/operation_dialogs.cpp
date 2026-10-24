#include "operation_dialogs.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/GUI/ExtractDialog.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QTimer>

// Public widget interactions in the fixture process only. Production 7zG owns
// these same source dialogs; no automation flags are added to its entrypoint.
void DriveOperationDialogs() {
    const QString report = qEnvironmentVariable("SEVENZIP_OPERATION_UI_REPORT");
    if (!report.isEmpty()) {
        QJsonArray widgets;
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (!widget->isVisible())
                continue;
            QJsonArray buttons;
            for (auto *button : widget->findChildren<QPushButton *>())
                buttons.append(QJsonObject{{"name", button->objectName()},
                                           {"text", button->text()},
                                           {"enabled", button->isEnabled()},
                                           {"visible", button->isVisible()}});
            widgets.append(QJsonObject{{"class", widget->metaObject()->className()},
                                       {"name", widget->objectName()},
                                       {"title", widget->windowTitle()},
                                       {"buttons", buttons}});
        }
        QSaveFile file(report);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(widgets).toJson());
            file.commit();
        }
    }
    for (auto *widget : QApplication::topLevelWidgets()) {
        if (!widget->isVisible())
            continue;
        if (auto *modal = QApplication::activeModalWidget(); modal && modal != widget)
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
                QTimer::singleShot(0, extract, &QDialog::reject);
            } else if (decision == "accept") {
                const QString destination = qEnvironmentVariable("SEVENZIP_OPERATION_DESTINATION");
                if (!destination.isEmpty()) {
                    extract->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
                    extract->findChild<QComboBox *>("extractDestination")->setEditText(destination);
                }
                QTimer::singleShot(0, extract, &QDialog::accept);
            }
        } else if (auto *create = qobject_cast<CreateArchiveDialog *>(widget)) {
            const QString decision = qEnvironmentVariable("SEVENZIP_OPERATION_CREATE_DECISION");
            if (decision == "cancel")
                QTimer::singleShot(0, create, &QDialog::reject);
            else if (decision == "accept") {
                const QString format = qEnvironmentVariable("SEVENZIP_OPERATION_CREATE_FORMAT");
                if (!format.isEmpty()) {
                    auto *selector = create->findChild<QComboBox *>("archiveFormat");
                    selector->setCurrentIndex(selector->findText(format, Qt::MatchFixedString));
                }
                const QString target = qEnvironmentVariable("SEVENZIP_OPERATION_CREATE_TARGET");
                if (!target.isEmpty()) {
                    create->findChild<QLineEdit *>("archiveFolder")
                        ->setText(QFileInfo(target).absolutePath());
                    create->findChild<QLineEdit *>("archiveName")
                        ->setText(QFileInfo(target).fileName());
                }
                const QString password = qEnvironmentVariable("SEVENZIP_OPERATION_CREATE_PASSWORD");
                if (!password.isEmpty()) {
                    create->findChild<QLineEdit *>("archivePassword")->setText(password);
                    create->findChild<QLineEdit *>("archivePasswordConfirmation")
                        ->setText(password);
                }
                if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_DELETE_INPUTS"))
                    create->findChild<QCheckBox *>("archiveDeleteAfterCompression")
                        ->setChecked(true);
                if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_SFX"))
                    create->findChild<QCheckBox *>("archiveSfx")->setChecked(true);
                const QString volume = qEnvironmentVariable("SEVENZIP_OPERATION_VOLUME");
                if (!volume.isEmpty())
                    create->findChild<QComboBox *>("archiveVolumeSize")->setEditText(volume);
                auto *ok = create->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
                QTimer::singleShot(0, ok, &QPushButton::click);
            }
        } else if (widget->objectName() == "passwordDialog") {
            auto *dialog = qobject_cast<QDialog *>(widget);
            if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_CANCEL_PASSWORD"))
                QTimer::singleShot(0, dialog, &QDialog::reject);
            else if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_PASSWORD")) {
                dialog->findChild<QLineEdit *>("passwordEdit")
                    ->setText(qEnvironmentVariable("SEVENZIP_OPERATION_PASSWORD"));
                QTimer::singleShot(0, dialog, &QDialog::accept);
            }
        } else if (widget->objectName() == "overwriteDialog") {
            const auto button = qEnvironmentVariable("SEVENZIP_OPERATION_OVERWRITE");
            if (auto *choice = widget->findChild<QPushButton *>(button))
                QTimer::singleShot(0, choice, &QPushButton::click);
        } else if (widget->objectName() == "operationMessagesDialog") {
            auto *close = widget->findChild<QPushButton *>("IDOK");
            QTimer::singleShot(0, close, &QPushButton::click);
        } else if (auto *message = qobject_cast<QMessageBox *>(widget)) {
            QTimer::singleShot(0, message, [message] {
                if (auto *ok = message->button(QMessageBox::Ok))
                    ok->click();
                else
                    message->accept();
            });
        } else if (widget->objectName() == "progressDialog") {
            auto *close = widget->findChild<QPushButton *>("IDCANCEL");
            if (close && close->property("languageID").toUInt() == IDS_CLOSE)
                QTimer::singleShot(0, close, &QPushButton::click);
        }
        // Let the source slot unwind before inspecting another modal. A Close
        // underneath a final-message box starts a different cancellation dialog.
        return;
    }
}
