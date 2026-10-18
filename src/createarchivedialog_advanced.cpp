#include "7zip/PropID.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "qtcompressoptionsdialog.h"
#include "qtdisclosure.h"
#include "qtlangcontrols.h"

void CreateArchiveDialog::createLifecycleOptions(QFormLayout *form) {
    // CompressDialog.rc: right-hand Options group, SFX, Shared, Delete,
    // preceding Encryption.
    auto *group = new QGroupBox("&Parameters:", this);
    group->setObjectName("archiveLifecycleOptions");
    group->setFlat(true);
    auto *layout = new QVBoxLayout(group);
    sfx_ = new QCheckBox(group);
    sfx_->setObjectName("archiveSfx");
    if (codecs_)
        sfxModule_ = codecs_->sfxModule();
    layout->addWidget(sfx_);
    connect(sfx_, &QCheckBox::toggled, this, &CreateArchiveDialog::toggleSfx);
    updateSfxControls();
    sharedFiles_ = new QCheckBox("Compress shared files", group);
    sharedFiles_->setObjectName("archiveSharedFiles");
#ifndef Q_OS_WIN
    sharedFiles_->setEnabled(false);
    sharedFiles_->hide();
    QtBindLocalToolTip(sharedFiles_, "createarchivedialog_advanced",
                       QT_TRANSLATE_NOOP("createarchivedialog_advanced",
                                         "File sharing restrictions apply only on Windows."));
#endif
    layout->addWidget(sharedFiles_);
    deleteAfterCompressing_ = new QCheckBox("Delete files after compression", group);
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
    QtBindLocalToolTip(parameters_, "createarchivedialog_advanced",
                       QT_TRANSLATE_NOOP("createarchivedialog_advanced",
                                         "Method parameters, for example: tm=off or mt=2."));
    auto *parametersForm = new QFormLayout;
    parametersForm->setRowWrapPolicy(QFormLayout::WrapAllRows);
    parametersForm->addRow("&Parameters:", parameters_);
    form->addRow(parametersForm);
    advanced_ = new QPushButton("&Parameters:", this);
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
    updateRedesignSummary();
}

void CreateArchiveDialog::updateRedesignSummary() {
    if (!advancedDisclosure_)
        return;
    QStringList values;
    const auto add = [&values](QWidget *control, UInt32 id, const QString &value) {
        if (!control->isEnabled() || value.isEmpty())
            return;
        QString label = QtLangText(id);
        label.remove('&');
        label.remove(':');
        values.append(label + ": " + value);
    };
    add(method_, IDT_COMPRESS_METHOD, method_->currentText());
    add(dictionary_, IDT_COMPRESS_DICTIONARY, dictionary_->currentText());
    if (wordSize_->currentData().toUInt())
        add(wordSize_, IDT_COMPRESS_ORDER, wordSize_->currentText());
    if (solidBlock_->currentData().toUInt() != UInt32(-1))
        add(solidBlock_, IDT_COMPRESS_SOLID, solidBlock_->currentText());
    add(threads_, IDT_COMPRESS_THREADS, threads_->currentText());
    if (Get_MemUse_Spec().IsDefined)
        add(memUse_, IDT_COMPRESS_MEMORY, memUse_->currentText());
    add(volume_, IDT_SPLIT_TO_VOLUMES, volume_->currentText());
    add(updateMode_, IDT_COMPRESS_UPDATE_MODE, updateMode_->currentText());
    add(pathMode_, IDT_COMPRESS_PATH_MODE, pathMode_->currentText());
    if (!parameters_->text().isEmpty())
        values.append(QtLangText(IDT_COMPRESS_PARAMETERS).remove('&').remove(':'));
    for (auto *check : {sfx_, sharedFiles_, deleteAfterCompressing_})
        if (check->isEnabled() && check->isChecked())
            values.append(QString(check->text()).remove('&'));
    if (advancedSummary_ && !advancedSummary_->text().isEmpty())
        values.append(advancedSummary_->text());
    advancedDisclosure_->setSummary(values.join(QStringLiteral(" · ")));
}
