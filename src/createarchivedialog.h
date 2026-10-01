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
    QString password() const { return password_ ? password_->text() : QString(); }
    bool encryptHeaders() const {
        return encryptHeaders_ && encryptHeaders_->isEnabled() && encryptHeaders_->isChecked();
    }
    CompressionOptions compressionOptions() const;

  private:
    void createSourcePicker(QVBoxLayout *layout);
    void createArchiveForm(QVBoxLayout *layout);
    void createCompressionOptions(QFormLayout *form);
    void createEncryptionOptions(QFormLayout *form);
    void SetEncryptionMethod();
    QString GetEncryptionMethodSpec() const;
    void UpdatePasswordControl();
    void SetLevel2();
    void FormatChanged();
    void MethodChanged();
    void SetOrder2();
    void SetDictionary2();
    void SetSolidBlockSize2();
    UInt64 GetDict2() const;
    void SetNumThreads2();
    void SetMemoryUsage();
    NCompression::CMemUse Get_MemUse_Spec() const;
    bool checkMemoryUsage();
    int FindRegistryFormat(const QString &name) const;
    unsigned FindRegistryFormat_Always(const QString &name);
    NCompression::CFormatOptions &Get_FormatOptions();
    void SaveOptionsInMem();
    void SetMemUseCombo();
    static QString compressionSettingsPath();
    void createFooter(QVBoxLayout *layout);
    void updateTargetPreview();
    void validateAndAccept();
    void addSource(const QString &path);
    void showDirectory(const QString &path);
    QString archivePath() const;

    QString currentDirectory_;
    const QString StartDirPrefix;
    QString target_;
    QString existingTarget_;
    QStringList selected_;
    QLineEdit *sourcePath_ = nullptr;
    QLineEdit *name_ = nullptr;
    QComboBox *archivePath_ = nullptr;
    QLineEdit *destination_ = nullptr;
    QLabel *preview_ = nullptr;
    QComboBox *format_ = nullptr;
    QComboBox *updateMode_ = nullptr;
    QComboBox *pathMode_ = nullptr;
    QComboBox *encryption_ = nullptr;
    QComboBox *level_ = nullptr;
    QComboBox *method_ = nullptr;
    QComboBox *dictionary_ = nullptr;
    QComboBox *wordSize_ = nullptr;
    QComboBox *solidBlock_ = nullptr;
    QComboBox *threads_ = nullptr;
    QLabel *hardwareThreads_ = nullptr;
    QLabel *memoryUsage_ = nullptr;
    QComboBox *memUse_ = nullptr;
    NCompression::CInfo m_RegistryInfo;
    QString registryFormat_;
    QLabel *decompressionMemory_ = nullptr;
    const NCompressDialog::CSystemInfo system_ = NCompressDialog::GetSystemInfo();
    QLineEdit *password_ = nullptr;
    QLineEdit *passwordConfirmation_ = nullptr;
    QCheckBox *showPassword_ = nullptr;
    QCheckBox *encryptHeaders_ = nullptr;
    QFormLayout *encryptionForm_ = nullptr;
    QListWidget *available_ = nullptr;
    QListWidget *sources_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
};
