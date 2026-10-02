#include "7zip/UI/Common/ArchiveName.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include <QSignalBlocker>

void CreateArchiveDialog::prepareInitialSources(const QStringList &sources) {
    UStringVector paths;
    for (const QString &path : sources) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (QFileInfo::exists(absolute) && !selected_.contains(absolute)) {
            selected_.append(absolute);
            paths.Add(UString(absolute.toStdWString().c_str()));
        }
    }
    if (codecs_) {
        UString baseName;
        const UString name = CreateArchiveName(paths, false, nullptr, baseName);
        suggestedArchiveName_ = QString::fromWCharArray(name.Ptr());
    }
}

void CreateArchiveDialog::populateArchiveFormats() {
    if (codecs_) {
        CObjectVector<NWildcard::CCensorPath> censor;
        for (const QString &path : selected_)
            censor.AddNew().Path = path.toStdWString().c_str();
        NCompressDialog::PrepareFormatSelection(archiveNameState_, codecs_->nativeCodecs(), censor);
        const auto &formats = codecs_->nativeCodecs().Formats;
        CUIntVector indices = archiveNameState_.ArcIndices;
        archiveNameState_.ArcIndices.Clear();
        FOR_VECTOR(i, indices) {
            const unsigned index = indices[i];
            const auto &format = formats[index];
            if (format.UpdateEnabled) {
                format_->addItem(QString::fromWCharArray(format.Name.Ptr()).toLower());
                archiveNameState_.ArcIndices.Add(index);
            }
        }
    } else
        format_->addItems({"7z", "zip"});
}

void CreateArchiveDialog::refreshArchiveFormats() {
    if (!codecs_)
        return;
    const QString previous = format_->currentText();
    // An existing stream format can disappear while its source selection is
    // empty/ineligible. Keep the detected target type as the restoration key.
    const QString selected = existingTarget_.isEmpty() ? format_->currentText()
                             : existingFormat_.isEmpty()
                                 ? QFileInfo(existingTarget_).suffix().toLower()
                                 : existingFormat_.toLower();
    {
        const QSignalBlocker blocker(format_);
        format_->clear();
        populateArchiveFormats();
        const int index = format_->findText(selected, Qt::MatchFixedString);
        format_->setCurrentIndex(index >= 0 ? index : existingTarget_.isEmpty() ? 0 : -1);
    }
    if (previous != format_->currentText())
        emit format_->currentTextChanged(format_->currentText());
    configureArchiveName(format_->currentText());
    buttons_->button(QDialogButtonBox::Ok)->setEnabled(format_->currentIndex() >= 0);
}

void CreateArchiveDialog::configureArchiveName(const QString &format) {
    archiveNameState_.Format = format;
    if (codecs_ && codecs_->result() == S_OK) {
        archiveNameState_.ArcFormats = &codecs_->nativeCodecs().Formats;
        archiveNameState_.Info.FormatIndex = codecs_->formatIndex(format);
    }
}
