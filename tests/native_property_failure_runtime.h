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
    bool arm(UInt32 index, HRESULT result, UInt32 itemCount = 0);
    bool forceDisk(bool enabled);
    QByteArray diskReceipts() const;
    QByteArray extractionReceipts() const;
    QByteArray openReceipts() const;
    bool clear();
    QByteArray receipts() const;

  private:
    QTemporaryDir owned_;
    QString runtime_;
    QString control_;
    QString diskControl_;
};
