#include "fileviewerdialog.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QVBoxLayout>

FileViewerDialog::FileViewerDialog(const QString &path, bool editable, QWidget *parent)
    : QDialog(parent), path_(path), editable_(editable) {
    setObjectName("fileViewerDialog");
    setWindowTitle(QStringLiteral("%1 — %2").arg(editable ? QStringLiteral("Редактор")
                                                          : QStringLiteral("Просмотр"),
                                                 QFileInfo(path).fileName()));
    resize(760, 560);
    auto *layout = new QVBoxLayout(this);
    text_ = new QPlainTextEdit(this);
    text_->setObjectName("fileViewerText");
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
        text_->setPlainText(QStringLiteral("Не удалось открыть файл: %1").arg(file.errorString()));
        text_->setReadOnly(true);
        if (auto *saveButton = buttons->button(QDialogButtonBox::Save))
            saveButton->setEnabled(false);
        return;
    }
    // Avoid loading an unbounded archive member into the GUI process.
    constexpr qint64 limit = 16 * 1024 * 1024;
    if (file.size() > limit) {
        text_->setPlainText(QStringLiteral("Файл больше 16 МиБ. Просмотр текста недоступен."));
        text_->setReadOnly(true);
        if (auto *saveButton = buttons->button(QDialogButtonBox::Save))
            saveButton->setEnabled(false);
        return;
    }
    original_ = file.read(limit + 1);
    if (original_.size() > limit) {
        text_->setPlainText(QStringLiteral("Файл больше 16 МиБ. Просмотр текста недоступен."));
        text_->setReadOnly(true);
        if (auto *saveButton = buttons->button(QDialogButtonBox::Save))
            saveButton->setEnabled(false);
        return;
    }
    const QString decoded = QString::fromUtf8(original_);
    if (original_.contains('\0') || decoded.contains(QChar::ReplacementCharacter)) {
        text_->setPlainText(QStringLiteral("Файл не является текстом UTF-8."));
        text_->setReadOnly(true);
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
        QMessageBox::warning(this, QStringLiteral("Редактор"),
                             QStringLiteral("Файл изменился на диске. Сохранение отменено."));
        return;
    }
    QSaveFile output(path_);
    if (!output.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, QStringLiteral("Редактор"), output.errorString());
        return;
    }
    const QByteArray bytes = text_->toPlainText().toUtf8();
    if (output.write(bytes) != bytes.size() || !output.commit()) {
        QMessageBox::warning(this, QStringLiteral("Редактор"), output.errorString());
        return;
    }
    original_ = bytes;
    wasSaved_ = true;
    text_->document()->setModified(false);
}

void FileViewerDialog::reject() {
    if (editable_ && text_->document()->isModified() &&
        QMessageBox::question(this, QStringLiteral("Редактор"),
                              QStringLiteral("Закрыть файл без сохранения изменений?")) !=
            QMessageBox::Yes)
        return;
    QDialog::reject();
}
