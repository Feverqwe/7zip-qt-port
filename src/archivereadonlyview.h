#pragma once
#include "7zip/IStream.h"
#include <QByteArray>
#include <QList>
#include <QString>
#include <functional>
#include <memory>

class ArchiveBackend;

// Capture one reader generation on the panel owner thread. Preparation and reads
// run on its worker; cancel is thread-safe. Navigation cannot replace this reader.
class ArchiveReadOnlyView final {
  public:
    struct Entry {
        quint64 Id = 0;
        quint64 Parent = 0;
        QString Name;
        quint64 Size = 0;
        bool Directory = false;
        quint64 Modified = 0;
    };
    struct ReadResult {
        HRESULT Result = E_FAIL;
        Int32 Operation = -1;
        QByteArray Data;
        QString Error;
    };
    ~ArchiveReadOnlyView();
    const QList<Entry> &entries() const;
    QString limitations() const;
    ReadResult read(quint64 id, quint64 offset, quint32 length);
    void cancel();

  private:
    struct State;
    std::unique_ptr<State> state_;
    ArchiveReadOnlyView(IUnknown *resource, quint64 quota, QString password,
                        std::function<bool(const QString &, quint64)> extractionConsent,
                        std::function<bool(const QString &, quint64, Int32)> recoveryConsent);
    HRESULT initialize(QString &error);
    friend class ArchiveBackend;
};
