// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSplitFile.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// QString replaces UString; file I/O remains in the Qt platform adapter.

#include "PanelSplitFile.h"
#include <QFile>
#include <QFileInfo>

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
        *error = QStringLiteral("Не удалось создать файл: %1").arg(OutputPath);
        return false;
    }
    const qint64 kBufSize = 1 << 20;
    QByteArray bufferObject(kBufSize, Qt::Uninitialized);
    char *buffer = bufferObject.data();
    qint64 pos = 0;
    if (!Set_NumBytesCur(pos)) {
        *error = QStringLiteral("Операция отменена.");
        return false;
    }
    for (qsizetype i = 0; i < Names.size(); ++i) {
        const QString nextName = InputDirPrefix + Names[i];
        QFile inFile(nextName);
        const QFileInfo info(nextName);
        if (!info.isFile() || info.isSymLink() || !inFile.open(QIODevice::ReadOnly)) {
            *error = QStringLiteral("Часть файла недоступна: %1").arg(nextName);
            return false;
        }
        const qint64 expectedSize = info.size();
        qint64 partSize = 0;
        for (;;) {
            qint64 processedSize = inFile.read(buffer, kBufSize);
            if (processedSize < 0) {
                *error = QStringLiteral("Ошибка чтения файла: %1").arg(nextName);
                return false;
            }
            if (processedSize == 0)
                break;
            const qint64 needSize = processedSize;
            processedSize = outFile.write(buffer, needSize);
            if (processedSize < 0) {
                *error = QStringLiteral("Ошибка записи файла: %1").arg(OutputPath);
                return false;
            }
            if (needSize != processedSize) {
                *error = QStringLiteral("Неполная запись файла: %1").arg(OutputPath);
                return false;
            }
            pos += processedSize;
            partSize += processedSize;
            if (!Set_NumBytesCur(pos)) {
                *error = QStringLiteral("Операция отменена.");
                return false;
            }
        }
        if (partSize != expectedSize || inFile.size() != expectedSize) {
            *error = QStringLiteral("Часть файла изменилась: %1").arg(nextName);
            return false;
        }
    }
    if (!outFile.flush()) {
        *error = QStringLiteral("Не удалось записать объединённый файл.");
        return false;
    }
    return true;
}
