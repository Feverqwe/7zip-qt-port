#include "qtdiagnostictext.h"
#include "Common/StringConvert.h"
#include "qttranslationcatalog.h"
#include <QStringList>

UString QtDiagnosticText(const char *source) {
    const QByteArray key = QString::fromWCharArray(GetUnicodeString(source)).toUtf8();
    const std::wstring text =
        QtTranslatedApplicationText("SevenZipDiagnostics", key.constData()).toStdWString();
    return UString(text.c_str());
}

UString QtDiagnosticText(const char *source, unsigned code) {
    const std::wstring text =
        QtTranslatedApplicationText("SevenZipDiagnostics", source).arg(code).toStdWString();
    return UString(text.c_str());
}

UString QtDiagnosticLines(const AString &source) {
    UString result;
    const auto lines = QString::fromLatin1(source.Ptr()).split('\n');
    for (qsizetype i = 0; i < lines.size(); ++i) {
        if (i)
            result.Add_LF();
        result += QtDiagnosticText(lines[i].toLatin1().constData());
    }
    return result;
}

UString QtArchiveDiagnosticText(const UString &source) {
    const QString text = QString::fromWCharArray(source);
    if (text.contains('\n')) {
        UString result;
        const auto lines = text.split('\n');
        for (qsizetype i = 0; i < lines.size(); ++i) {
            if (i)
                result.Add_LF();
            result += QtArchiveDiagnosticText(UString(lines[i].toStdWString().c_str()));
        }
        return result;
    }
    static const QString extFeaturesPrefix = QStringLiteral("Unverified ext read-only features: ");
    if (text.startsWith(extFeaturesPrefix) && text.endsWith('.')) {
        const QString message = QtTranslatedApplicationText(
            "SevenZipDiagnostics",
            QT_TRANSLATE_NOOP("SevenZipDiagnostics", "Unverified ext read-only features: %1."));
        return UString(
            message
                .arg(text.mid(extFeaturesPrefix.size(), text.size() - extFeaturesPrefix.size() - 1))
                .toStdWString()
                .c_str());
    }
    static const QString missingPrefix = QStringLiteral("Missing volume : ");
    if (text.startsWith(missingPrefix)) {
        const QString message = QtTranslatedApplicationText(
            "SevenZipDiagnostics", QT_TRANSLATE_NOOP("SevenZipDiagnostics", "Missing volume: %1"));
        return UString(message.arg(text.mid(missingPrefix.size())).toStdWString().c_str());
    }
    const char *const warnings[] = {
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some FAT files share clusters with directories. File contents may "
                          "include directory entries or unrelated data. Copying remains available "
                          "but reports an error."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The FAT file shares clusters with a directory. Its contents are "
                          "unverified."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The CramFS volume origin is ambiguous. File names or contents may "
                          "belong to another candidate volume. Results from the selected origin "
                          "are unverified."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some CramFS file block tables are incomplete. Only readable portions "
                          "of affected files can be copied; incomplete copies report an error."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The declared CramFS volume size does not bound all file references "
                          "reliably. Reading uses the available source bounds. Some files may be "
                          "damaged or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some CramFS directories could not be read completely. Files or names "
                          "may be missing. Readable files remain accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The CramFS directory could not be read completely."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The CramFS volume checksum could not be verified. Some files may be "
                          "damaged or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The CramFS volume checksum does not match. Some files may be damaged."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The CramFS source is shorter than the declared volume. Some files may "
                          "be incomplete or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "32-bit overflow in headers"),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "16-bit overflow for number of files in headers"),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Volume was not properly unmounted. Some data may be corrupt."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Medium has reported failures. Some data may be lost."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "The HFS wrapper lacks protection flags. Embedded volume data may be inconsistent."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The free-space count exceeds the volume size. Free space is unknown."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The volume state is unavailable. Files may be inconsistent."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The NTFS volume is marked dirty. Some data may be corrupt."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some NTFS reparse metadata is unavailable. Unclassified entries expose "
                          "underlying DATA streams where readable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some NTFS extended attributes are unavailable. WSL ownership and file "
                          "types may be unknown. Readable DATA streams remain accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some NTFS DATA mappings are incomplete or unverified. Files or streams "
                          "may be outdated, incomplete or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "NTFS DATA mapping is incomplete or unverified."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some NTFS file metadata is unavailable or unverified. Names, times and "
                          "permissions may be missing or outdated."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "NTFS file metadata is unavailable or unverified."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The ext filesystem requires journal recovery. The journal will not be "
                          "replayed; files may be missing, outdated or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The ext filesystem requires orphan cleanup. Orphaned files will not be "
                          "repaired; files may be missing or incomplete."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The ext filesystem was not cleanly unmounted or reports errors. Files "
                          "may be inconsistent."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "Some NTFS MFT records are unavailable or unverified. Files or metadata may "
            "be missing, incomplete or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The NTFS MFT record is unavailable or unverified."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "Some NTFS security descriptors are unavailable or unverified. Permissions "
            "may be missing. Readable file data remains accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The NTFS security descriptor is unavailable or unverified."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The NTFS source is shorter than the declared volume. Some files may be "
                          "incomplete or unreadable."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "NTFS is being read using a backup boot sector. The primary sector is invalid."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "The backup NTFS boot sector is unavailable or differs from the primary sector."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "Deleted NTFS records are shown for recovery. Names, metadata and file data "
            "may be missing, outdated or reused."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "NTFS reparse metadata is unavailable. The main file stream cannot be decoded."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The FAT volume was not properly unmounted. Some data may be corrupt."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The FAT source is shorter than the declared volume. Some files may be "
                          "incomplete or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some FAT table entries could not be read. Free space is unknown; some "
                          "files or directories may be incomplete or unreadable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some FAT directories could not be read completely. Files or names may "
                          "be missing. Readable files remain accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some FAT file chains are incomplete. Only readable prefixes of affected "
                          "files are available; copying them reports an error."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "The FAT file chain is incomplete. Only the readable prefix is available."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "The FAT directory could not be read completely."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "The ext directory could not be read completely."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some extended attributes of this ext item could not be read."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some ext extended attributes could not be read. Unreadable attributes "
                          "have been omitted; readable files remain accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some ext directories could not be read completely. The file list may be "
                          "incomplete or outdated."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS extended attributes are unavailable. Readable file data "
                          "and attributes remain accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS extended attributes are unavailable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS ownership metadata is unavailable. User or group IDs may "
                          "be unknown. Readable file data remains accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "SquashFS user or group IDs are unavailable."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS file fragments are unavailable. Only readable prefixes of "
                          "affected files are available; copying them reports an error."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "The SquashFS file fragment is unavailable. Only the readable prefix is available."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS directories could not be read completely. Files or names "
                          "may be missing. Readable files remain accessible."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The SquashFS directory could not be read completely."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "The SquashFS inode is unavailable. File data and metadata could not be read."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS directory references are unavailable. Files recovered by "
                          "inode number may be misidentified or outdated."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The SquashFS inode-number mapping is unverified."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS inode records are unavailable. Files or metadata may be "
                          "missing. Inode identities are unverified."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS metadata indexes are unavailable. File data, ownership or "
                          "extended attributes may be incomplete."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS file block lists are unavailable. Only readable portions "
                          "of affected files can be copied; incomplete copies report an error."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Some SquashFS file block-list metadata is unavailable.")};
    for (const char *warning : warnings)
        if (text == QString::fromLatin1(warning))
            return QtDiagnosticText(warning);
    if (text == QString::fromLatin1(warnings[0]) + ' ' + QString::fromLatin1(warnings[1])) {
        UString result = QtDiagnosticText(warnings[0]);
        result.Add_Space();
        result += QtDiagnosticText(warnings[1]);
        return result;
    }
    return source;
}
