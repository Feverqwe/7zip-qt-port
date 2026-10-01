// Qt OS-association boundary for the source external-open no-handle workflow.
#pragma once
#include "CPP/7zip/UI/FileManager/Panel.h"
#include <QUrl>
#include <functional>

class QtAssociatedFileServices final : public IQtExternalFileServices {
    std::function<bool(const QUrl &)> _opener;

  public:
    explicit QtAssociatedFileServices(std::function<bool(const QUrl &)> opener);
    HRESULT StartApplication(const UString &dir, const UString &path, QWidget *window) override;
    HRESULT CreateProcess(const UString &program, const UString &parameters,
                          const UStringVector &paths, QWidget *window) override;
};
