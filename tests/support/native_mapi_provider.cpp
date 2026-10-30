#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <windows.h>

#include <MAPI.h>

namespace {
QString text(const wchar_t *value) { return value ? QString::fromWCharArray(value) : QString(); }
QString text(const char *value) { return value ? QString::fromLocal8Bit(value) : QString(); }
template <class Message>
ULONG record(const Message *message, const char *encoding, LHANDLE session, ULONG_PTR owner,
             FLAGS flags, ULONG reserved) {
    const QString report = qEnvironmentVariable("SEVENZIP_MAPI_TEST_REPORT");
    if (!message || report.isEmpty())
        return MAPI_E_FAILURE;
    QJsonObject result{{"encoding", encoding},
                       {"session", double(session)},
                       {"owner", double(owner)},
                       {"flags", double(flags)},
                       {"reserved", double(reserved)},
                       {"messageReserved", double(message->ulReserved)},
                       {"messageFlags", double(message->flFlags)}};
    QJsonArray files;
    for (ULONG index = 0; index < message->nFileCount; ++index) {
        const auto &file = message->lpFiles[index];
        const QString path = text(file.lpszPathName);
        const QString copy = report + QString(".attachment-%1.zip").arg(index);
        QFile input(path);
        const bool readable = input.open(QIODevice::ReadOnly);
        const QByteArray bytes = readable ? input.readAll() : QByteArray();
        input.close();
        files.append(QJsonObject{{"path", path},
                                 {"name", text(file.lpszFileName)},
                                 {"position", double(file.nPosition)},
                                 {"reserved", double(file.ulReserved)},
                                 {"flags", double(file.flFlags)},
                                 {"fileTypeNull", file.lpFileType == nullptr},
                                 {"readable", readable},
                                 {"size", bytes.size()},
                                 {"copied", QFile::copy(path, copy)}});
    }
    result["files"] = files;
    QJsonArray recipients;
    for (ULONG index = 0; index < message->nRecipCount; ++index) {
        const auto &recipient = message->lpRecips[index];
        recipients.append(QJsonObject{{"class", double(recipient.ulRecipClass)},
                                      {"address", text(recipient.lpszAddress)},
                                      {"nameNull", recipient.lpszName == nullptr},
                                      {"entrySize", double(recipient.ulEIDSize)},
                                      {"entryNull", recipient.lpEntryID == nullptr}});
    }
    result["recipients"] = recipients;
    result["originatorNull"] = message->lpOriginator == nullptr;
    result["subjectNull"] = message->lpszSubject == nullptr;
    result["cwdChanged"] = QDir::setCurrent(qEnvironmentVariable("SEVENZIP_MAPI_TEST_CWD"));
    QFile output(report);
    if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(result).toJson()) <= 0)
        return MAPI_E_FAILURE;
    // The source intentionally ignores the MAPI result. No mail is sent.
    return qEnvironmentVariableIntValue("SEVENZIP_MAPI_TEST_CANCEL") ? MAPI_USER_ABORT
                                                                     : SUCCESS_SUCCESS;
}
} // namespace

extern "C" __declspec(dllexport) DWORD WINAPI SevenzipMAPIProvider() { return 0x375a4d50; }

#ifdef SEVENZIP_MAPI_WIDE
extern "C"
    __declspec(dllexport) ULONG WINAPI SevenzipMAPISendMailW(LHANDLE session, ULONG_PTR owner,
                                                             lpMapiMessageW message, FLAGS flags,
                                                             ULONG reserved) {
    return record(message, "wide", session, owner, flags, reserved);
}
#endif
#ifdef SEVENZIP_MAPI_ANSI
extern "C" __declspec(dllexport) ULONG WINAPI SevenzipMAPISendMail(LHANDLE session, ULONG_PTR owner,
                                                                   lpMapiMessage message,
                                                                   FLAGS flags, ULONG reserved) {
    return record(message, "ansi", session, owner, flags, reserved);
}
#endif
