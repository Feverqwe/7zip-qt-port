#pragma once

#include "archivebackend.h"
#include "archivesfxmodule.h"

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
                                 QWidget *parent = nullptr,
                                 std::shared_ptr<const ArchiveCodecCatalog> codecs = {},
                                 const CCompressDialog *sourceDialog = nullptr);

    NCompressDialog::CInfo sourceInfo() const;
    static HRESULT PresentSource(CCompressDialog &source,
                                 const CObjectVector<NWildcard::CCensorPath> &paths,
                                 std::shared_ptr<const ArchiveCodecCatalog> codecs,
                                 QWidget *parent);

    QString target() const { return target_; }
    QStringList sources() const { return selected_; }
    QString password() const {
        return password_ && password_->isEnabled() ? password_->text() : QString();
    }
    bool encryptHeaders() const {
        return encryptHeaders_ && encryptHeaders_->isEnabled() && encryptHeaders_->isChecked();
    }
    CompressionOptions compressionOptions() const;

  private:
    void createArchiveForm(QVBoxLayout *layout);
    void createCompressionOptions(QFormLayout *form);
    void createEncryptionOptions(QFormLayout *form);
    void createVolumeOptions(QFormLayout *form);
    void createAdvancedOptions(QFormLayout *form);
    void createLifecycleOptions(QFormLayout *form);
    void updateSfxControls();
    void toggleSfx(bool enabled);
    void showAdvancedOptions();
    void updateAdvancedSummary();
    bool checkVolumeSizes();
    void SetEncryptionMethod();
    QString GetEncryptionMethodSpec() const;
    void UpdatePasswordControl();
    void SetLevel2();
    void FormatChanged();
    void MethodChanged();
    void SetOrder2(UInt32 defaultOrder);
    void SetDictionary2(UInt32 defaultDict);
    void SetSolidBlockSize2(UInt32 defaultBlockSize);
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
    void BindLanguage();
    void ReloadLanguageItems();
    static QString LevelText(unsigned level);
    void changeEvent(QEvent *event) override;
    void validateAndAccept();
    QString archivePath() const;

    QString currentDirectory_;
    const QString StartDirPrefix;
    QString target_;
    std::shared_ptr<const ArchiveCodecCatalog> codecs_;
    CCompressDialog archiveNameState_;
    void configureArchiveName(const QString &format);
    void populateArchiveFormats();
    void prepareInitialSources(const QStringList &sources);
    void initializeSourceControls(const NCompressDialog::CInfo &info);
    bool fixedSources_ = false;
    QString suggestedArchiveName_ = QStringLiteral("archive");
    QStringList selected_;
    QLineEdit *name_ = nullptr;
    QComboBox *archivePath_ = nullptr;
    QLabel *archiveFolder_ = nullptr;
    QComboBox *format_ = nullptr;
    QComboBox *updateMode_ = nullptr;
    QComboBox *pathMode_ = nullptr;
    QComboBox *volume_ = nullptr;
    QLineEdit *parameters_ = nullptr;
    QPushButton *advanced_ = nullptr;
    QLabel *advancedSummary_ = nullptr;
    QCheckBox *sharedFiles_ = nullptr;
    QCheckBox *sfx_ = nullptr;
    ArchiveSfxModule sfxModule_;
    QCheckBox *deleteAfterCompressing_ = nullptr;
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
    QDialogButtonBox *buttons_ = nullptr;
};
