#include "7zip/UI/Common/ArchiveName.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"

void CreateArchiveDialog::prepareInitialSources(const QStringList &sources) {
    UStringVector paths;
    for (const QString &path : sources) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (fixedSources_) {
            selected_.append(absolute);
            continue;
        }
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
        if (!fixedSources_)
            NCompressDialog::PrepareFormatSelection(archiveNameState_, codecs_->nativeCodecs(),
                                                    censor);
        const auto &formats = codecs_->nativeCodecs().Formats;
        archiveNameState_.ArcFormats = &formats;
        archiveNameState_.OnInit(*format_, m_RegistryInfo);
    } else
        format_->addItems({"7z", "zip"});
}

void CreateArchiveDialog::configureArchiveName(const QString &format) {
    archiveNameState_.Format = format;
    if (codecs_ && codecs_->result() == S_OK) {
        archiveNameState_.ArcFormats = &codecs_->nativeCodecs().Formats;
        archiveNameState_.Info.FormatIndex = codecs_->formatIndex(format);
    } else
        archiveNameState_.ArcInfo.Name = format.toStdWString().c_str();
}
