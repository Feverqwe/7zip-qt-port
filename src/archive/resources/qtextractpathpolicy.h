// Bounded output policy for the source ArchiveExtractCallback path algorithm.
#pragma once
#include "Common/StringConvert.h"
#include "archive/backend/archivebackend.h"
#include "archive/callbacks/ArchiveExtractCallback.h"
#include <QDir>
#include <QFileInfo>

class CQtExtractPathPolicy final : public IQtExtractPathPolicy {
    QString _destination;

  public:
    explicit CQtExtractPathPolicy(const QString &destination) : _destination(destination) {}
    HRESULT CheckPath(const FString &path, bool directory, bool alternateStream,
                      UString &message) override {
        QString error;
        QString output = QString::fromWCharArray(fs2us(path).Ptr());
#ifdef Q_OS_WIN
        // The source has corrected the stream name and chosen its base file.
        // Validate the base independently, so an existing base link cannot
        // redirect an otherwise bounded NTFS stream write.
        if (alternateStream) {
            if (ArchiveBackend::validateExtractionStreamOutput(_destination, output, &error))
                return S_OK;
            message = error.toStdWString().c_str();
            return E_INVALIDARG;
        }
#else
        Q_UNUSED(alternateStream);
#endif
        if (ArchiveBackend::validateExtractionOutput(_destination, output, directory, &error))
            return S_OK;
        message = error.toStdWString().c_str();
        return E_INVALIDARG;
    }
    HRESULT CheckLink(const FString &path, const FString &target, bool relative,
                      UString &message) override {
        // The source has already normalized/rebased the link. Only resolve its
        // final filesystem location here; source link algorithms remain owners.
        const QString from = QString::fromWCharArray(fs2us(path).Ptr());
        const QString to = QString::fromWCharArray(fs2us(target).Ptr());
        const QString resolved = relative ? QDir(QFileInfo(from).absolutePath()).filePath(to) : to;
        QString error;
        if (ArchiveBackend::validateExtractionOutput(_destination, from, false, &error) &&
            ArchiveBackend::validateExtractionOutput(_destination, resolved,
                                                     QFileInfo(resolved).isDir(), &error))
            return S_OK;
        message = error.toStdWString().c_str();
        return E_INVALIDARG;
    }
};
