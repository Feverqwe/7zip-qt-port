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
bool PathIsRepresentable(const QString &path) {
    QStringEncoder encoder(QStringEncoder::Utf8);
    const QByteArray encoded = encoder(path);
    Q_UNUSED(encoded);
    return !encoder.hasError() && !path.contains(QChar::Null) && QDir::isAbsolutePath(path);
}
QString KindName(QtOperationMessage::Kind type) {
    switch (type) {
    case QtOperationMessage::Kind::Ready:
        return QStringLiteral("ready");
    case QtOperationMessage::Kind::Job:
        return QStringLiteral("job");
    case QtOperationMessage::Kind::Accepted:
        return QStringLiteral("accepted");
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
        if (object.contains("yesToAll")) {
            fields.insert("yesToAll");
            if (!object.value("yesToAll").isBool())
                return false;
        }
        if (!ExactFields(object, fields) || object.value("command") != "test" ||
            !object.value("paths").isArray())
            return false;
        const auto paths = object.value("paths").toArray();
        if (paths.isEmpty())
            return false;
        message.Type = QtOperationMessage::Kind::Job;
        message.YesToAll = object.value("yesToAll").toBool(false);
        for (const auto &path : paths) {
            if (!path.isString() || !PathIsRepresentable(path.toString()))
                return false;
            message.Paths.append(path.toString());
        }
    } else if (type == "result") {
        if (!ExactFields(object, {"outcome", "exitCode"}))
            return false;
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
        object.insert("command", "test");
        object.insert("paths", QJsonArray::fromStringList(message.Paths));
        object.insert("yesToAll", message.YesToAll);
    } else if (message.Type == QtOperationMessage::Kind::Result) {
        object.insert("outcome", OutcomeName(message.Result.Status));
        object.insert("exitCode", message.Result.ExitCode);
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
