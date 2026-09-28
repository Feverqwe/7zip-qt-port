#pragma once

#include "archivebackend.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

class CreateArchiveDialog : public QDialog {
    Q_OBJECT

  public:
    explicit CreateArchiveDialog(const QString &directory, const QStringList &initialSources = {},
                                 QWidget *parent = nullptr, const QString &existingTarget = {});

    QString target() const { return target_; }
    QStringList sources() const { return selected_; }
    QString password() const {
        return encryption_ && encryption_->currentIndex() != 0 ? password_->text() : QString();
    }
    bool encryptHeaders() const { return encryption_ && encryption_->currentIndex() == 2; }
    CompressionOptions compressionOptions() const;

  private:
    void createSourcePicker(QVBoxLayout *layout);
    void createArchiveForm(QVBoxLayout *layout);
    void createCompressionOptions(QFormLayout *form);
    void createEncryptionOptions(QFormLayout *form);
    void SetLevel2();
    void SetOrder2();
    void createFooter(QVBoxLayout *layout);
    void updateTargetPreview();
    void validateAndAccept();
    void addSource(const QString &path);
    void showDirectory(const QString &path);

    QString currentDirectory_;
    QString target_;
    QString existingTarget_;
    QStringList selected_;
    QLineEdit *sourcePath_ = nullptr;
    QLineEdit *name_ = nullptr;
    QLineEdit *destination_ = nullptr;
    QLabel *preview_ = nullptr;
    QComboBox *format_ = nullptr;
    QComboBox *encryption_ = nullptr;
    QComboBox *level_ = nullptr;
    QComboBox *method_ = nullptr;
    QComboBox *dictionary_ = nullptr;
    QComboBox *wordSize_ = nullptr;
    QComboBox *solidBlock_ = nullptr;
    QCheckBox *solid_ = nullptr;
    QSpinBox *threads_ = nullptr;
    QLineEdit *password_ = nullptr;
    QLineEdit *passwordConfirmation_ = nullptr;
    QListWidget *available_ = nullptr;
    QListWidget *sources_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
};
