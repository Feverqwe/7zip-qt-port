#include "qtoperationprotocol.h"
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStringConverter>
#include <QtEndian>

namespace {
constexpr quint32 kVersion = 1;
constexpr quint32 kMaximumMessage = 64 * 1024 * 1024;

bool ExactFields(const QJsonObject &object, QSet<QString> fields) {
    fields.insert(QStringLiteral("version"));
    fields.insert(QStringLiteral("type"));
    const QStringList keys = object.keys();
    return QSet<QString>(keys.begin(), keys.end()) == fields;
}
bool TextIsRepresentable(const QString &path) {
    QStringEncoder encoder(QStringEncoder::Utf8);
    const QByteArray encoded = encoder(path);
    Q_UNUSED(encoded);
    return !encoder.hasError() && !path.contains(QChar::Null);
}
bool PathIsRepresentable(const QString &path) {
    return TextIsRepresentable(path) && QDir::isAbsolutePath(path);
}
bool RelativeHashPath(QString path) {
    if (!TextIsRepresentable(path) || path.isEmpty() || !QDir::isRelativePath(path))
        return false;
#ifdef Q_OS_WIN
    path.replace('\\', '/');
#endif
    const auto parts = path.split('/');
    return !parts.contains("..") && !parts.contains(".");
}
QString KindName(QtOperationMessage::Kind type) {
    switch (type) {
    case QtOperationMessage::Kind::Ready:
        return QStringLiteral("ready");
    case QtOperationMessage::Kind::Job:
        return QStringLiteral("job");
    case QtOperationMessage::Kind::Accepted:
        return QStringLiteral("accepted");
    case QtOperationMessage::Kind::Outputs:
        return QStringLiteral("outputs");
    case QtOperationMessage::Kind::Result:
        return QStringLiteral("result");
    case QtOperationMessage::Kind::Rejected:
        return QStringLiteral("rejected");
    }
    return {};
}
QString OutcomeName(QtOperationResult::Outcome status) {
    switch (status) {
    case QtOperationResult::Outcome::Success:
        return QStringLiteral("success");
    case QtOperationResult::Outcome::Warning:
        return QStringLiteral("warning");
    case QtOperationResult::Outcome::Cancelled:
        return QStringLiteral("cancelled");
    case QtOperationResult::Outcome::Failure:
        return QStringLiteral("failure");
    default:
        return {}; // Launch, observation and protocol failures belong to the parent.
    }
}

bool Decode(const QJsonObject &object, QtOperationMessage &message) {
    if (object.value("version") != QJsonValue(int(kVersion)))
        return false;
    const QString type = object.value("type").toString();
    if (type == "ready" || type == "accepted") {
        if (!ExactFields(object, {}))
            return false;
        message.Type =
            type == "ready" ? QtOperationMessage::Kind::Ready : QtOperationMessage::Kind::Accepted;
    } else if (type == "job") {
        QSet<QString> fields{"command", "paths"};
        const bool extract = object.value("command") == "extract";
        const bool compress = object.value("command") == "compress";
        const bool hash = object.value("command") == "hash";
        const bool benchmark = object.value("command") == "benchmark";
        if (benchmark) {
            fields.insert("totalMode");
            if (!object.value("totalMode").isBool())
                return false;
            message.Operation = QtOperationMessage::Command::Benchmark;
            message.TotalMode = object.value("totalMode").toBool();
        }
        if (hash) {
            fields.unite({"method", "basePrefix", "enterToDirs"});
            if (!object.value("method").isString() ||
                !TextIsRepresentable(object.value("method").toString()) ||
                !object.value("basePrefix").isString() || !object.value("enterToDirs").isBool())
                return false;
            message.Operation = QtOperationMessage::Command::Hash;
            message.HashMethod = object.value("method").toString();
            message.HashBasePrefix = object.value("basePrefix").toString();
            message.HashEnterToDirs = object.value("enterToDirs").toBool();
            if (!message.HashBasePrefix.isEmpty() &&
                (!PathIsRepresentable(message.HashBasePrefix) ||
                 !(message.HashBasePrefix.endsWith('/')
#ifdef Q_OS_WIN
                   || message.HashBasePrefix.endsWith('\\') || message.HashBasePrefix.endsWith(':')
#endif
                       )))
                return false;
        }
        if (object.value("command") == "test" && object.contains("hashMode")) {
            fields.insert("hashMode");
            if (!object.value("hashMode").isBool())
                return false;
            message.HashMode = object.value("hashMode").toBool();
        }
        if (extract) {
            fields.unite({"destination", "showDialog", "elimDup", "writeZone"});
            const auto zone = object.value("writeZone");
            const double value = zone.toDouble(-1);
            if (!object.value("destination").isString() ||
                !PathIsRepresentable(object.value("destination").toString()) ||
                !object.value("showDialog").isBool() || !object.value("elimDup").isBool() ||
                !(value == 0 || value == 1 || value == 2 || value == double(quint32(-1))))
                return false;
            message.Operation = QtOperationMessage::Command::Extract;
            message.Destination = object.value("destination").toString();
            message.ShowDialog = object.value("showDialog").toBool();
            message.ElimDup = object.value("elimDup").toBool();
            message.WriteZone = quint32(value);
        }
        if (compress) {
            fields.unite({"archivePath", "archiveType", "addExtension", "email", "showDialog"});
            if (!object.value("archivePath").isString() ||
                !PathIsRepresentable(object.value("archivePath").toString()) ||
                !object.value("archiveType").isString() ||
                !TextIsRepresentable(object.value("archiveType").toString()) ||
                !object.value("addExtension").isBool() || !object.value("email").isBool() ||
                !object.value("showDialog").isBool())
                return false;
            message.Operation = QtOperationMessage::Command::Compress;
            message.ArchivePath = object.value("archivePath").toString();
            message.ArchiveType = object.value("archiveType").toString();
            message.AddExtension = object.value("addExtension").toBool();
            message.EMail = object.value("email").toBool();
            message.ShowDialog = object.value("showDialog").toBool();
        }
        if (object.contains("yesToAll")) {
            fields.insert("yesToAll");
            if (!object.value("yesToAll").isBool())
                return false;
        }
        if (!ExactFields(object, fields) ||
            (!extract && !compress && !hash && !benchmark && object.value("command") != "test") ||
            !object.value("paths").isArray())
            return false;
        const auto paths = object.value("paths").toArray();
        if (benchmark ? !paths.isEmpty() : paths.isEmpty())
            return false;
        message.Type = QtOperationMessage::Kind::Job;
        message.YesToAll = object.value("yesToAll").toBool(false);
        for (const auto &path : paths) {
            if (!path.isString() ||
                !(hash && !message.HashBasePrefix.isEmpty() ? RelativeHashPath(path.toString())
                                                            : PathIsRepresentable(path.toString())))
                return false;
            message.Paths.append(path.toString());
        }
    } else if (type == "result" || type == "outputs") {
        const bool outputs = type == "outputs";
        QSet<QString> fields =
            outputs ? QSet<QString>{"changedDirectories"} : QSet<QString>{"outcome", "exitCode"};
        if (object.contains("changedDirectories")) {
            fields.insert("changedDirectories");
            if (!object.value("changedDirectories").isArray())
                return false;
            for (const auto &path : object.value("changedDirectories").toArray()) {
                if (!path.isString() || !PathIsRepresentable(path.toString()))
                    return false;
                message.Result.ChangedDirectories.append(path.toString());
            }
        }
        if (object.contains("changedArchives")) {
            fields.insert("changedArchives");
            if (!object.value("changedArchives").isArray())
                return false;
            for (const auto &path : object.value("changedArchives").toArray()) {
                if (!path.isString() || !PathIsRepresentable(path.toString()))
                    return false;
                message.Result.ChangedArchives.append(path.toString());
            }
        }
        if (!ExactFields(object, fields))
            return false;
        if (outputs) {
            message.Type = QtOperationMessage::Kind::Outputs;
            return !message.Result.ChangedDirectories.isEmpty();
        }
        message.Type = QtOperationMessage::Kind::Result;
        const QString status = object.value("outcome").toString();
        bool found = false;
        for (const auto outcome :
             {QtOperationResult::Outcome::Success, QtOperationResult::Outcome::Warning,
              QtOperationResult::Outcome::Cancelled, QtOperationResult::Outcome::Failure}) {
            if (OutcomeName(outcome) == status) {
                message.Result.Status = outcome;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
        const int code = object.value("exitCode").toInt(-1);
        if (code < 0 || code > 255 || object.value("exitCode") != QJsonValue(code) ||
            (status == "success" && code != 0) || (status == "warning" && code != 1) ||
            (status == "cancelled" && code != 255) ||
            (status == "failure" && (code == 0 || code == 1 || code == 255)))
            return false;
        message.Result.ExitCode = code;
    } else if (type == "rejected") {
        if (!ExactFields(object, {"error"}) || !object.value("error").isString())
            return false;
        message.Type = QtOperationMessage::Kind::Rejected;
        message.Result.Status = QtOperationResult::Outcome::ProtocolFailed;
        message.Result.Error = object.value("error").toString();
    } else
        return false;
    return true;
}
} // namespace

bool EncodeOperationMessage(const QtOperationMessage &message, QByteArray &frame, QString &error) {
    QJsonObject object{{"version", int(kVersion)}, {"type", KindName(message.Type)}};
    if (message.Type == QtOperationMessage::Kind::Job) {
        object.insert("command",
                      message.Operation == QtOperationMessage::Command::Extract     ? "extract"
                      : message.Operation == QtOperationMessage::Command::Compress  ? "compress"
                      : message.Operation == QtOperationMessage::Command::Hash      ? "hash"
                      : message.Operation == QtOperationMessage::Command::Benchmark ? "benchmark"
                                                                                    : "test");
        if (message.Operation == QtOperationMessage::Command::Benchmark)
            object.insert("totalMode", message.TotalMode);
        if (message.Operation == QtOperationMessage::Command::Hash) {
            object.insert("method", message.HashMethod);
            object.insert("basePrefix", message.HashBasePrefix);
            object.insert("enterToDirs", message.HashEnterToDirs);
        }
        if (message.Operation == QtOperationMessage::Command::Test && message.HashMode)
            object.insert("hashMode", true);
        if (message.Operation == QtOperationMessage::Command::Extract) {
            object.insert("destination", message.Destination);
            object.insert("showDialog", message.ShowDialog);
            object.insert("elimDup", message.ElimDup);
            object.insert("writeZone", double(message.WriteZone));
        }
        if (message.Operation == QtOperationMessage::Command::Compress) {
            object.insert("archivePath", message.ArchivePath);
            object.insert("archiveType", message.ArchiveType);
            object.insert("addExtension", message.AddExtension);
            object.insert("email", message.EMail);
            object.insert("showDialog", message.ShowDialog);
        }
        object.insert("paths", QJsonArray::fromStringList(message.Paths));
        object.insert("yesToAll", message.YesToAll);
    } else if (message.Type == QtOperationMessage::Kind::Result ||
               message.Type == QtOperationMessage::Kind::Outputs) {
        if (message.Type == QtOperationMessage::Kind::Result) {
            object.insert("outcome", OutcomeName(message.Result.Status));
            object.insert("exitCode", message.Result.ExitCode);
        }
        object.insert("changedDirectories",
                      QJsonArray::fromStringList(message.Result.ChangedDirectories));
        if (!message.Result.ChangedArchives.isEmpty())
            object.insert("changedArchives",
                          QJsonArray::fromStringList(message.Result.ChangedArchives));
    } else if (message.Type == QtOperationMessage::Kind::Rejected)
        object.insert("error", message.Result.Error);
    QtOperationMessage verified;
    if (!Decode(object, verified)) {
        error = QCoreApplication::translate("QtOperation", "Invalid operation message.");
        return false;
    }
    const QByteArray body = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (body.size() > kMaximumMessage) {
        error = QCoreApplication::translate("QtOperation", "Operation message is too large.");
        return false;
    }
    frame.resize(4);
    qToBigEndian(quint32(body.size()), frame.data());
    frame.append(body);
    return true;
}

QtOperationMessageReader::State
QtOperationMessageReader::read(QIODevice &input, QtOperationMessage &message, QString &error) {
    if (failed_)
        return State::Error;
    const auto reject = [&](const QString &reason) {
        failed_ = true;
        header_.clear();
        body_.clear();
        error = reason;
        return State::Error;
    };
    if (header_.size() < 4) {
        header_.append(input.read(4 - header_.size()));
        if (header_.size() < 4)
            return State::Pending;
        size_ = qFromBigEndian<quint32>(header_.constData());
        if (!size_ || size_ > kMaximumMessage)
            return reject(
                QCoreApplication::translate("QtOperation", "Invalid operation frame size."));
    }
    body_.append(input.read(qint64(size_) - body_.size()));
    if (body_.size() < size_)
        return State::Pending;
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(body_, &parseError);
    QtOperationMessage decoded;
    if (parseError.error != QJsonParseError::NoError || !document.isObject() ||
        !Decode(document.object(), decoded))
        return reject(QCoreApplication::translate("QtOperation",
                                                  "Unsupported or malformed operation message."));
    message = std::move(decoded);
    header_.clear();
    body_.clear();
    size_ = 0;
    return State::Message;
}
