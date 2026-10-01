#include "7zip/PropID.h"
#include "createarchivedialog.h"
#include "qtcompressoptionsdialog.h"

void CreateArchiveDialog::createAdvancedOptions(QFormLayout *form) {
    parameters_ = new QLineEdit(this);
    parameters_->setObjectName("archiveParameters");
    parameters_->setText(QString::fromWCharArray(Get_FormatOptions().Options.Ptr()));
    parameters_->setToolTip("Параметры метода, например: tm=off или mt=2.");
    form->addRow("Параметры:", parameters_);
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
    source.MethodID = NCompressDialog::GetMethodID(method_->currentText(), registryFormat_);
    source.ArcInfo.Name = registryFormat_.toStdWString().c_str();
    source.ArcInfo.Flags = NArcInfoFlags::kCTime | NArcInfoFlags::kATime | NArcInfoFlags::kMTime |
                           NArcInfoFlags::kMTime_Default;
    source.ArcInfo.TimeFlags = TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kWindows) |
                               TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT(NFileTimeType::kWindows);
    if (registryFormat_ == "zip")
        source.ArcInfo.TimeFlags |= TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kUnix) |
                                    TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kDOS);
    // Local profile copy is source COptionsDialog's in-memory transaction:
    // Options Cancel does not mutate the Add form; Add Cancel never saves disk.
    NCompression::CFormatOptions profile = Get_FormatOptions();
    source.FormatOptions = &profile;
    COptionsDialog dialog(&source);
    if (dialog.Create(this) == QDialog::Accepted)
        Get_FormatOptions() = profile;
    updateAdvancedSummary();
}

void CreateArchiveDialog::updateAdvancedSummary() {
    if (!advancedSummary_)
        return;
    CCompressDialog source;
    source.FormatOptions = &Get_FormatOptions();
    source.ShowOptionsString();
    advancedSummary_->setText(QString::fromWCharArray(source.OptionsString.Ptr()));
}
