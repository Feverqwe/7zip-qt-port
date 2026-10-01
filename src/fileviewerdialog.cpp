#include "fileviewerdialog.h"
#include "qtlangcontrols.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QTextCursor>
#include <QVBoxLayout>

FileViewerDialog::FileViewerDialog(const QString &path, bool editable, QWidget *parent)
    : QDialog(parent), path_(path), editable_(editable) {
    setObjectName("fileViewerDialog");
    setProperty("languageSuffix", QStringLiteral(" — ") + QFileInfo(path).fileName());
    QtBindLocalText(this, editable ? "Editor" : "Viewer", editable ? "Редактор" : "Просмотр");
    resize(760, 560);
    auto *layout = new QVBoxLayout(this);
    text_ = new QPlainTextEdit(this);
    text_->setObjectName("fileViewerText");
    QtBindLocalText(text_, "File contents", "Содержимое файла", true);
    text_->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(text_);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    if (editable) {
        auto *saveButton = buttons->addButton(QDialogButtonBox::Save);
        saveButton->setObjectName("fileViewerSave");
        connect(saveButton, &QPushButton::clicked, this, &FileViewerDialog::save);
    }
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &FileViewerDialog::reject);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setReadOnlyMessage("Cannot open file: %1", "Не удалось открыть файл: %1",
                           file.errorString());
        if (auto *saveButton = buttons->button(QDialogButtonBox::Save))
            saveButton->setEnabled(false);
        return;
    }
    // Avoid loading an unbounded archive member into the GUI process.
    constexpr qint64 limit = 16 * 1024 * 1024;
    if (file.size() > limit) {
        setReadOnlyMessage("The file exceeds 16 MiB. Text preview is unavailable.",
                           "Файл больше 16 МиБ. Просмотр текста недоступен.");
        if (auto *saveButton = buttons->button(QDialogButtonBox::Save))
            saveButton->setEnabled(false);
        return;
    }
    original_ = file.read(limit + 1);
    if (original_.size() > limit) {
        setReadOnlyMessage("The file exceeds 16 MiB. Text preview is unavailable.",
                           "Файл больше 16 МиБ. Просмотр текста недоступен.");
        if (auto *saveButton = buttons->button(QDialogButtonBox::Save))
            saveButton->setEnabled(false);
        return;
    }
    const QString decoded = QString::fromUtf8(original_);
    if (original_.contains('\0') || decoded.contains(QChar::ReplacementCharacter)) {
        setReadOnlyMessage("The file is not UTF-8 text.", "Файл не является текстом UTF-8.");
        if (auto *saveButton = buttons->button(QDialogButtonBox::Save))
            saveButton->setEnabled(false);
        return;
    }
    text_->setPlainText(decoded);
    text_->setReadOnly(!editable);
}

void FileViewerDialog::save() {
    if (!editable_ || !text_->document()->isModified())
        return;
    QFile current(path_);
    if (!current.open(QIODevice::ReadOnly) || current.readAll() != original_) {
        QMessageBox::warning(this, windowTitle(),
                             QtLocalText("The file changed on disk. Saving was cancelled.",
                                         "Файл изменился на диске. Сохранение отменено."));
        return;
    }
    QSaveFile output(path_);
    if (!output.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, windowTitle(), output.errorString());
        return;
    }
    const QByteArray bytes = text_->toPlainText().toUtf8();
    if (output.write(bytes) != bytes.size() || !output.commit()) {
        QMessageBox::warning(this, windowTitle(), output.errorString());
        return;
    }
    original_ = bytes;
    wasSaved_ = true;
    text_->document()->setModified(false);
}

void FileViewerDialog::reject() {
    if (editable_ && text_->document()->isModified() &&
        QMessageBox::question(this, windowTitle(),
                              QtLocalText("Close the file without saving changes?",
                                          "Закрыть файл без сохранения изменений?")) !=
            QMessageBox::Yes)
        return;
    QDialog::reject();
}

void FileViewerDialog::setReadOnlyMessage(const QString &english, const QString &russian,
                                          const QString &detail) {
    messageEnglish_ = english;
    messageRussian_ = russian;
    messageDetail_ = detail;
    QString message = QtLocalText(english, russian);
    if (message.contains("%1"))
        message = message.arg(detail);
    text_->setPlainText(message);
    text_->setReadOnly(true);
}

void FileViewerDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && text_ && !messageEnglish_.isEmpty()) {
        const int anchor = text_->textCursor().anchor();
        const int position = text_->textCursor().position();
        setReadOnlyMessage(messageEnglish_, messageRussian_, messageDetail_);
        QTextCursor cursor = text_->textCursor();
        const int end = text_->document()->characterCount() - 1;
        cursor.setPosition(qMin(anchor, end));
        cursor.setPosition(qMin(position, end), QTextCursor::KeepAnchor);
        text_->setTextCursor(cursor);
    }
    QDialog::changeEvent(event);
}
