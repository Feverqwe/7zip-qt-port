// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSplitFile.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// QString replaces UString; file I/O remains in the Qt platform adapter.

#include "PanelSplitFile.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "Panel.h"
#include "qtlanguageservices.h"
#include <QFile>
#include <QFileInfo>

static void AddInfoFileName(QString &dest, const QString &name) {
    dest += "\n  ";
    dest += name;
}

// Summary block from CApp::Combine, kept beside its original worker. The Qt
// caller supplies the validated names and sizes without Win32 panel access.
QString CThreadCombine::GetInfo() const {
    UString summary;
    AddValuePair2(summary, IDS_PROP_FILES, quint64(Names.size()), TotalSize);
    QString info = ToQString(summary);
    info += '\n';
    info += InputDirPrefix;
    qsizetype i;
    for (i = 0; i < Names.size() && i < 2; i++)
        AddInfoFileName(info, Names[i]);
    if (i != Names.size()) {
        if (i + 1 != Names.size())
            AddInfoFileName(info, "...");
        AddInfoFileName(info, Names.back());
    }
    return info;
}

void CVolSeqName::SetNumDigits(quint64 numVolumes) {
    ChangedPart = QStringLiteral("000");
    while (numVolumes > 999) {
        numVolumes /= 10;
        ChangedPart += QLatin1Char('0');
    }
}

bool CVolSeqName::ParseName(const QString &name) {
    if (name.size() < 2)
        return false;
    if (name.back() != QLatin1Char('1') || name[name.size() - 2] != QLatin1Char('0'))
        return false;

    qsizetype pos = name.size() - 2;
    for (; pos > 0 && name[pos - 1] == QLatin1Char('0'); --pos)
        ;
    UnchangedPart = name.left(pos);
    ChangedPart = name.mid(pos);
    return true;
}

QString CVolSeqName::GetNextName() {
    for (int i = ChangedPart.size() - 1; i >= 0; --i) {
        const QChar c = ChangedPart[i];
        if (c != QLatin1Char('9')) {
            ChangedPart[i] = QChar(c.unicode() + 1);
            break;
        }
        ChangedPart[i] = QLatin1Char('0');
        if (i == 0)
            ChangedPart.prepend(QLatin1Char('1'));
    }
    return UnchangedPart + ChangedPart;
}

// CThreadCombine::ProcessVirt from upstream 0766b73. NIO Read/Write and
// HRESULT become QFile calls and bool; a callback replaces CProgressSync.
// NewOnly, flush and input-size checks preserve the Qt transaction contract.
bool CThreadCombine::ProcessVirt(QString *error) {
    QFile outFile(OutputPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        *error =
            QtLocalText("Could not create file: %1", QStringLiteral("Не удалось создать файл: %1"))
                .arg(OutputPath);
        return false;
    }
    const qint64 kBufSize = 1 << 20;
    QByteArray bufferObject(kBufSize, Qt::Uninitialized);
    char *buffer = bufferObject.data();
    qint64 pos = 0;
    if (!Set_NumBytesCur(pos)) {
        *error = QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
        return false;
    }
    for (qsizetype i = 0; i < Names.size(); ++i) {
        const QString nextName = InputDirPrefix + Names[i];
        QFile inFile(nextName);
        const QFileInfo info(nextName);
        if (!info.isFile() || info.isSymLink() || !inFile.open(QIODevice::ReadOnly)) {
            *error = QtLocalText("File part is unavailable: %1",
                                 QStringLiteral("Часть файла недоступна: %1"))
                         .arg(nextName);
            return false;
        }
        const qint64 expectedSize = info.size();
        qint64 partSize = 0;
        for (;;) {
            qint64 processedSize = inFile.read(buffer, kBufSize);
            if (processedSize < 0) {
                *error =
                    QtLocalText("Error reading file: %1", QStringLiteral("Ошибка чтения файла: %1"))
                        .arg(nextName);
                return false;
            }
            if (processedSize == 0)
                break;
            const qint64 needSize = processedSize;
            processedSize = outFile.write(buffer, needSize);
            if (processedSize < 0) {
                *error =
                    QtLocalText("Error writing file: %1", QStringLiteral("Ошибка записи файла: %1"))
                        .arg(OutputPath);
                return false;
            }
            if (needSize != processedSize) {
                *error = QtLocalText("Incomplete file write: %1",
                                     QStringLiteral("Неполная запись файла: %1"))
                             .arg(OutputPath);
                return false;
            }
            pos += processedSize;
            partSize += processedSize;
            if (!Set_NumBytesCur(pos)) {
                *error = QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
                return false;
            }
        }
        if (partSize != expectedSize || inFile.size() != expectedSize) {
            *error =
                QtLocalText("File part changed: %1", QStringLiteral("Часть файла изменилась: %1"))
                    .arg(nextName);
            return false;
        }
    }
    if (!outFile.flush()) {
        *error = QtLocalText("Could not write combined file.",
                             QStringLiteral("Не удалось записать объединённый файл."));
        return false;
    }
    return true;
}

// Output-name block from CApp::Combine in upstream PanelSplitFile.cpp.
QString CVolSeqName::GetCombinedName() const {
    QString outName = UnchangedPart;
    while (!outName.isEmpty()) {
        if (outName.back() != QLatin1Char('.'))
            break;
        outName.chop(1);
    }
    if (outName.isEmpty())
        outName = QStringLiteral("file");
    return outName;
}

// CThreadSplit::ProcessVirt from upstream 0766b73. The transaction adapter
// supplies stage paths instead of VolBasePath. QFile replaces NIO; optional
// preallocation is omitted, while flush errors are reported before closing.
bool CThreadSplit::ProcessVirt(QString *error) {
    QFile inFile(FilePath);
    if (!inFile.open(QIODevice::ReadOnly)) {
        *error = inFile.errorString();
        return false;
    }
    QFile outFile;
    const qint64 kBufSize = 1 << 20;
    QByteArray buffer(kBufSize, Qt::Uninitialized);
    qint64 Written = 0;
    qint64 pos = 0;
    qint64 prev = 0;
    qsizetype numFiles = 0;
    qsizetype volIndex = 0;
    const auto Close = [&] {
        const bool ok = !outFile.isOpen() || outFile.flush();
        outFile.close();
        Written = 0;
        if (!ok)
            *error = QtLocalText("Could not write file part.",
                                 QStringLiteral("Не удалось записать часть файла."));
        return ok;
    };
    if (!Set_NumBytesCur(pos)) {
        *error = QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
        return false;
    }
    for (;;) {
        qint64 volSize;
        if (volIndex < VolumeSizes.size())
            volSize = VolumeSizes[volIndex];
        else
            volSize = VolumeSizes.last();
        const qint64 needSize = qMin(kBufSize, volSize - Written);
        qint64 processedSize = inFile.read(buffer.data(), needSize);
        if (processedSize < 0) {
            *error = inFile.errorString();
            return false;
        }
        if (processedSize == 0) {
            if (!Close())
                return false;
            if (pos != TotalSize || inFile.size() != TotalSize) {
                *error =
                    QtLocalText("Source file changed while splitting.",
                                QStringLiteral("Исходный файл изменился во время разделения."));
                return false;
            }
            return true;
        }
        if (Written == 0) {
            if (numFiles >= OutputPaths.size()) {
                *error =
                    QtLocalText("Source file changed while splitting.",
                                QStringLiteral("Исходный файл изменился во время разделения."));
                return false;
            }
            outFile.setFileName(OutputPaths[numFiles]);
            if (!outFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
                *error = outFile.errorString();
                return false;
            }
        }
        const qint64 readSize = processedSize;
        processedSize = outFile.write(buffer.constData(), readSize);
        if (processedSize != readSize) {
            *error = QtLocalText("Error writing file part.",
                                 QStringLiteral("Ошибка записи части файла."));
            return false;
        }
        Written += processedSize;
        pos += processedSize;
        if (Written == volSize) {
            if (!Close())
                return false;
            ++numFiles;
            if (volIndex < VolumeSizes.size())
                ++volIndex;
        }
        if (pos - prev >= (qint64(1) << 22) || Written == 0) {
            if (!Set_NumBytesCur(pos)) {
                *error = QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
                return false;
            }
            prev = pos;
        }
    }
}
