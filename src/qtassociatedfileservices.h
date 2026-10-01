// Qt OS-association boundary for the source external-open no-handle workflow.
#pragma once
#include "CPP/7zip/UI/FileManager/Panel.h"
#include <QUrl>
#include <functional>

class QtAssociatedFileServices final : public IQtExternalFileServices {
    std::function<bool(const QUrl &)> _opener;
    QtNativeProcessServices _processServices;
    std::function<bool(CPanel &, const std::function<bool()> &)> _updateRunner;
    std::function<bool()> _updateReady;

  public:
    explicit QtAssociatedFileServices(
        std::function<bool(const QUrl &)> opener,
        std::function<bool(CPanel &, const std::function<bool()> &)> updateRunner = {},
        std::function<bool()> updateReady = {});
    bool CanHandleExternalUpdate() const override;
    bool RunArchiveUpdate(CPanel &panel, const std::function<bool()> &body) override;
    HRESULT StartApplication(const UString &dir, const UString &path, QWidget *window) override;
    HRESULT CreateProcess(const UString &program, const UString &parameters,
                          const UStringVector &paths, QWidget *window) override;
    IQtProcessServices *ProcessServices() override { return &_processServices; }
    HRESULT CreateKnownProcess(const UString &program, const UString &parameters,
                               const UStringVector &paths, QWidget *window,
                               IQtProcessHandle *&process) override;
};
