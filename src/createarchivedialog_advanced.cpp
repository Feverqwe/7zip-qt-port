#include "7zip/PropID.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "qtcompressoptionsdialog.h"
#include "qtlangcontrols.h"

void CreateArchiveDialog::createLifecycleOptions(QFormLayout *form) {
    // CompressDialog.rc: right-hand Options group, SFX, Shared, Delete,
    // preceding Encryption.
    auto *group = new QGroupBox("Параметры", this);
    group->setObjectName("archiveLifecycleOptions");
    auto *layout = new QVBoxLayout(group);
    sfx_ = new QCheckBox(group);
    sfx_->setObjectName("archiveSfx");
    if (codecs_)
        sfxModule_ = codecs_->sfxModule();
    layout->addWidget(sfx_);
    connect(sfx_, &QCheckBox::toggled, this, &CreateArchiveDialog::toggleSfx);
    updateSfxControls();
    sharedFiles_ = new QCheckBox("Сжимать открытые для записи файлы", group);
    sharedFiles_->setObjectName("archiveSharedFiles");
#ifndef Q_OS_WIN
    sharedFiles_->setEnabled(false);
    QtBindLocalToolTip(sharedFiles_, "File sharing restrictions apply only on Windows.",
                       "Ограничение совместного доступа к файлам используется только в Windows.");
#endif
    layout->addWidget(sharedFiles_);
    deleteAfterCompressing_ = new QCheckBox("Удалить файлы после сжатия", group);
    deleteAfterCompressing_->setObjectName("archiveDeleteAfterCompression");
    layout->addWidget(deleteAfterCompressing_);
    form->addRow(group);
    // OnInit leaves this checkbox available for all update modes. The original
    // UpdateArchive owner validates incompatible action sets after confirmation.
}

void CreateArchiveDialog::createAdvancedOptions(QFormLayout *form) {
    parameters_ = new QLineEdit(this);
    parameters_->setObjectName("archiveParameters");
    parameters_->setText(QString::fromWCharArray(Get_FormatOptions().Options.Ptr()));
    QtBindLocalToolTip(parameters_, "Method parameters, for example: tm=off or mt=2.",
                       "Параметры метода, например: tm=off или mt=2.");
    auto *parametersForm = new QFormLayout;
    parametersForm->setRowWrapPolicy(QFormLayout::WrapAllRows);
    parametersForm->addRow("Параметры:", parameters_);
    form->addRow(parametersForm);
    advanced_ = new QPushButton("Параметры…", this);
    advanced_->setObjectName("archiveAdvancedOptions");
    auto *row = new QHBoxLayout;
    row->addWidget(advanced_, 0);
    advancedSummary_ = new QLabel(this);
    advancedSummary_->setObjectName("archiveAdvancedSummary");
    advancedSummary_->setWordWrap(true);
    row->addWidget(advancedSummary_, 1);
    form->addRow(row);
    updateAdvancedSummary();
    connect(advanced_, &QPushButton::clicked, this, &CreateArchiveDialog::showAdvancedOptions);
}

void CreateArchiveDialog::showAdvancedOptions() {
    // The exact registered handler flags supply the original dialog capability
    // owner while codecs/compression remain in the official external engine.
    CCompressDialog source;
    source.Format = registryFormat_;
    if (fixedSources_)
        source.Info = archiveNameState_.Info;
    source.MethodID = archiveNameState_.GetMethodID();
#ifdef Q_OS_WIN
    source.PreserveATime.SetVal_as_Supported(m_RegistryInfo.PreserveATime.Val);
#endif
    source.ArcInfo.Name = registryFormat_.toStdWString().c_str();
    source.ArcInfo.Flags = NArcInfoFlags::kCTime | NArcInfoFlags::kATime | NArcInfoFlags::kMTime |
                           NArcInfoFlags::kMTime_Default;
    source.ArcInfo.TimeFlags = TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kWindows) |
                               TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT(NFileTimeType::kWindows);
    if (registryFormat_ == "zip")
        source.ArcInfo.TimeFlags |= TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kUnix) |
                                    TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kDOS);
    if (codecs_ && codecs_->result() == S_OK) {
        const int index = codecs_->formatIndex(registryFormat_);
        if (index >= 0)
            source.ArcInfo = codecs_->nativeCodecs().Formats[unsigned(index)];
    }
    source.LoadMetadataOptions(m_RegistryInfo);
    // Local profile copy is source COptionsDialog's in-memory transaction:
    // Options Cancel does not mutate the Add form; Add Cancel never saves disk.
    NCompression::CFormatOptions profile = Get_FormatOptions();
    source.FormatOptions = &profile;
    COptionsDialog dialog(&source);
    if (dialog.Create(this) == QDialog::Accepted) {
        Get_FormatOptions() = profile;
        source.SaveMetadataOptions(m_RegistryInfo);
        if (fixedSources_) {
            archiveNameState_.Info.SymLinks = source.Info.SymLinks;
            archiveNameState_.Info.HardLinks = source.Info.HardLinks;
            archiveNameState_.Info.AltStreams = source.Info.AltStreams;
            archiveNameState_.Info.NtSecurity = source.Info.NtSecurity;
        }
        if (source.PreserveATime.Supported) {
            m_RegistryInfo.PreserveATime.Val = source.PreserveATime.Val;
            m_RegistryInfo.PreserveATime.Def = true;
        }
    }
    updateAdvancedSummary();
}

void CreateArchiveDialog::updateAdvancedSummary() {
    if (!advancedSummary_)
        return;
    CCompressDialog source;
    if (fixedSources_)
        source.Info = archiveNameState_.Info;
    if (codecs_ && codecs_->result() == S_OK) {
        const int index = codecs_->formatIndex(registryFormat_);
        if (index >= 0)
            source.ArcInfo = codecs_->nativeCodecs().Formats[unsigned(index)];
    }
    source.LoadMetadataOptions(m_RegistryInfo);
    source.FormatOptions = &Get_FormatOptions();
    source.ShowOptionsString();
    advancedSummary_->setText(QString::fromWCharArray(source.OptionsString.Ptr()));
}
