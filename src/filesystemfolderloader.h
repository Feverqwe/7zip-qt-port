#pragma once

#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include <QThread>
#include <atomic>

// Thread service only: CFSFolder owns the native walk and metadata algorithms.
class FilesystemFolderLoader final : public QThread {
    Q_OBJECT
  public:
    explicit FilesystemFolderLoader(QString directory, QObject *parent = nullptr);
    void cancel();
    // Call after wait(): non-atomic COM ownership never crosses a running load.
    HRESULT result() const;
    IFolderFolder *takeFolder();

  signals:
    void advanced(quint64 count);

  protected:
    void run() override;

  private:
    QString directory_;
    std::atomic_bool canceled_{false};
    HRESULT result_ = E_FAIL;
    CMyComPtr<IFolderFolder> folder_;
};
