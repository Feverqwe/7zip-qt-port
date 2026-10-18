#include "CPP/7zip/UI/GUI/HashGUI.h"
#include "Common/StringConvert.h"
#include "Windows/ErrorMsg.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivenativeopen.h"
#include "qtarchivehashoperation.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QFileInfo>
#include <QPointer>

void ArchiveBackend::hashMethods(const QStringList &paths, const QStringList &methods) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_INVALIDARG;
    if (paths.isEmpty() || methods.isEmpty()) {
        emit finished(false, QtLocalText("No files or hash method selected.",
                                         "Не выбраны файлы или метод хеширования."));
        return;
    }
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.exists() || info.isSymLink()) {
            emit finished(false, QtLocalText("File is missing or is a link: %1",
                                             "Файл отсутствует или является ссылкой: %1")
                                     .arg(path));
            return;
        }
    }
    const auto catalog = codecCatalog();
    CExternalCodecs providers;
    lastResult_ = loadHashProviders(providers);
    if (lastResult_ == S_OK) {
        UStringVector selected;
        for (const QString &method : methods)
            selected.Add(method.toStdWString().c_str());
        CHashBundle validation;
        lastResult_ = validation.SetMethods(&providers, selected);
    }
    if (lastResult_ != S_OK) {
        emit finished(
            false, QtLocalText("Unsupported hash method.", "Неподдерживаемый метод хеширования."));
        return;
    }
    hashOperation_ = std::make_unique<QtArchiveHashOperation>(paths, methods, catalog);
    const quint64 generation = ++hashGeneration_;
    const quint64 progressGeneration = beginWorkerProgress();
    hashOperation_->Progress = [this, progressGeneration](quint64 completed, quint64 total,
                                                          const QString &path) {
        const int percent = total ? int(qMin(100.0, 100.0 * completed / total)) : 0;
        postWorkerProgress(progressGeneration, percent, path);
    };
    connect(hashOperation_.get(), &QThread::finished, this, [this, generation] {
        if (hashOperation_ && hashGeneration_ == generation)
            finishHashOperation();
    });
    cancelled_ = false;
    job_ = Job::Hash;
    startWorkerPhase(*hashOperation_);
}

void ArchiveBackend::finishHashOperation() {
    if (!hashOperation_)
        return;
    hashOperation_->wait();
    finishWorkerProgress();
    auto operation = std::move(hashOperation_);
    lastResult_ = operation->Result;
    if (cancelled_)
        lastResult_ = E_ABORT;
    const HRESULT result = lastResult_;
    job_ = Job::None;
    const bool ok = result == S_OK && operation->Bundle.NumErrors == 0;
    const QPointer<ArchiveBackend> owner(this);
    if (result != E_ABORT) {
        for (const QString &line : operation->Output) {
            emit outputReceived(line);
            if (!owner)
                return;
        }
        for (const QString &error : operation->Errors) {
            emit outputReceived(error);
            if (!owner)
                return;
        }
        if (result == S_OK) {
            UString results;
            AddHashBundleRes(results, operation->Bundle);
            emit outputReceived(QString::fromWCharArray(results.Ptr()));
            if (!owner)
                return;
        }
    }
    emit finished(ok, ok ? QtLocalText("Checksums calculated.", "Контрольные суммы рассчитаны.")
                      : result == E_ABORT
                          ? QtLocalText("Operation canceled.", "Операция отменена.")
                          : QtLocalText("Checksum calculation failed.",
                                        "Расчёт контрольных сумм завершился с ошибками."));
}

HRESULT ArchiveBackend::validateNativeHashReads(const ArchiveNativeOpen &reader,
                                                const UInt32 *indices, UInt32 count) const {
    IInArchive *archive = reader.arc().Archive;
    UInt32 total = 0;
    RINOK(archive->GetNumberOfItems(&total))
    if (count != UInt32(-1) && count && !indices)
        return E_INVALIDARG;
    const QString root =
        QFileInfo(QString::fromWCharArray(reader.arc().Path.Ptr())).dir().canonicalPath();
    if (root.isEmpty())
        return E_INVALIDARG;
    if (count == UInt32(-1)) {
        count = total;
        indices = nullptr;
    }
    for (UInt32 n = 0; n < count; ++n) {
        const UInt32 index = indices ? indices[n] : n;
        if (index >= total)
            return E_INVALIDARG;
        NWindows::NCOM::CPropVariant path;
        RINOK(archive->GetProperty(index, kpidPath, &path))
        if (path.vt != VT_BSTR)
            return E_INVALIDARG;
        QString relative = QString::fromWCharArray(path.bstrVal);
        if (!safePath(relative))
            return E_INVALIDARG;
        relative.replace('\\', '/');
        QString current = root;
        for (const QString &part : relative.split('/')) {
            current = QDir(current).filePath(part);
            NWindows::NFile::NFind::CFileInfo info;
            if (info.Find(us2fs(current.toStdWString().c_str())) && info.IsOsSymLink())
                return E_INVALIDARG;
        }
    }
    return S_OK;
}
