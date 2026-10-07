#pragma once
#include "Common/MyWindows.h"
#include <QTemporaryDir>

// Private copied runtime and per-test file control for the delegating handler.
class NativePropertyFailureRuntime {
  public:
    bool prepare(const QString &prepared, const QString &facade);
    QString engine() const;
    QString module() const;
    bool createArchive(const QString &path, int count) const;
    bool arm(UInt32 index, HRESULT result);
    bool clear();
    QByteArray receipts() const;

  private:
    QTemporaryDir owned_;
    QString runtime_;
    QString control_;
};
