#pragma once

#include <QString>
#include <QStringList>
#include <functional>

struct CVolSeqName {
    QString UnchangedPart;
    QString ChangedPart = QStringLiteral("000");

    void SetNumDigits(quint64 numVolumes);
    bool ParseName(const QString &name);
    QString GetNextName();
    QString GetCombinedName() const;
};

// Functional worker from PanelSplitFile.cpp; Qt replaces NIO and progress sync.
class CThreadCombine {
  public:
    QString InputDirPrefix;
    QStringList Names;
    QString OutputPath;
    std::function<bool(qint64)> Set_NumBytesCur;
    bool ProcessVirt(QString *error);
};

class CThreadSplit {
  public:
    QString FilePath;
    QStringList OutputPaths;
    QList<qint64> VolumeSizes;
    qint64 TotalSize = 0;
    std::function<bool(qint64)> Set_NumBytesCur;
    bool ProcessVirt(QString *error);
};
