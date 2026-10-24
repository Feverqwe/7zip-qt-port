#include "qtoperationlauncher.h"
#include "qtoperationprocess.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <optional>

struct QtOperationLauncher::State {
    QtOperationProcess process_;
    std::unique_ptr<QTemporaryDir> directory_;
    QLocalServer server_;
    QPointer<QLocalSocket> socket_;
    QtOperationMessageReader reader_;
    QByteArray job_;
    QTimer poll_, startup_;
    QElapsedTimer exited_;
    std::optional<QtOperationResult> result_;
    QStringList changedDirectories_;
    QStringList changedArchives_;
    bool sent_ = false;
    bool accepted_ = false;
    bool terminal_ = false;
};

QtOperationLauncher::QtOperationLauncher(QObject *parent)
    : QObject(parent), state_(std::make_unique<State>()) {
    state_->server_.setSocketOptions(QLocalServer::UserAccessOption);
    state_->startup_.setSingleShot(true);
    state_->poll_.setInterval(25);
    connect(&state_->server_, &QLocalServer::newConnection, this,
            &QtOperationLauncher::acceptConnection);
    connect(&state_->poll_, &QTimer::timeout, this, &QtOperationLauncher::observe);
    connect(&state_->startup_, &QTimer::timeout, this, [this] {
        protocolFailure(QCoreApplication::translate(
            "QtOperation", "Operation startup acknowledgement was not received."));
        // No child termination: a request may already have been accepted even
        // if its acknowledgement could not reach this observer.
        if (state_->socket_)
            state_->socket_->disconnectFromServer();
        state_->server_.close();
    });
}
QtOperationLauncher::~QtOperationLauncher() {
    if (state_->socket_) {
        disconnect(state_->socket_, nullptr, this, nullptr);
        state_->socket_->setParent(nullptr);
        state_->socket_->deleteLater();
    }
}

QString QtOperationLauncher::GuiProgram() {
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(
#ifdef Q_OS_WIN
            QStringLiteral("7zG.exe")
#else
            QStringLiteral("7zG")
#endif
        );
}

quint64 QtOperationLauncher::processId() const { return state_->process_.processId(); }

QtOperationLauncher *QtOperationLauncher::Launch(const QString &program,
                                                 const QtOperationMessage &job, QString &error) {
    auto *application = QCoreApplication::instance();
    if (!application || QThread::currentThread() != application->thread()) {
        error = QCoreApplication::translate("QtOperation",
                                            "GUI operations require the application owner thread.");
        return nullptr;
    }
    auto operation = std::make_unique<QtOperationLauncher>(application);
    connect(operation.get(), &QtOperationLauncher::finished, operation.get(),
            &QObject::deleteLater);
    if (!operation->start(program, job, error))
        return nullptr;
    return operation.release();
}

bool QtOperationLauncher::start(const QString &program, const QtOperationMessage &job,
                                QString &error) {
    if (state_->poll_.isActive() || state_->terminal_) {
        error =
            QCoreApplication::translate("QtOperation", "An operation observer cannot be reused.");
        return false;
    }
    if (job.Type != QtOperationMessage::Kind::Job) {
        error = QCoreApplication::translate("QtOperation", "Expected an operation job.");
        return false;
    }
    if (!EncodeOperationMessage(job, state_->job_, error))
        return false;
    QString channel;
#ifdef Q_OS_WIN
    channel = QStringLiteral("sevenzip-") + QUuid::createUuid().toString(QUuid::Id128);
#else
    // sockaddr_un cannot hold a socket under an arbitrarily long TMPDIR.
    // Both supported POSIX platforms provide /tmp. Keep the socket in a
    // private, short directory; job data never goes into a filesystem file.
    state_->directory_ = std::make_unique<QTemporaryDir>(QStringLiteral("/tmp/7z-XXXXXX"));
    if (!state_->directory_->isValid()) {
        error = state_->directory_->errorString();
        return false;
    }
    channel = state_->directory_->filePath(QStringLiteral("operation"));
#endif
    if (!state_->server_.listen(channel)) {
        error = state_->server_.errorString();
        return false;
    }
    if (!state_->process_.start(program, channel, error)) {
        state_->server_.close();
        return false;
    }
    state_->startup_.start(10000);
    state_->poll_.start();
    return true;
}

void QtOperationLauncher::acceptConnection() {
    const QPointer<QtOperationLauncher> owner(this);
    while (state_->server_.hasPendingConnections()) {
        QLocalSocket *socket = state_->server_.nextPendingConnection();
        if (state_->socket_) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
        state_->socket_ = socket;
        socket->setParent(this);
        socket->setReadBufferSize(64 * 1024 * 1024 + 4);
        connect(socket, &QLocalSocket::readyRead, this, &QtOperationLauncher::readMessages);
        connect(socket, &QLocalSocket::disconnected, this, [this] {
            // EOF is not cancellation or proof of a crash. Drain buffered
            // messages and continue observing the actual native process.
            readMessages();
        });
        readMessages();
        if (!owner)
            return;
    }
}

void QtOperationLauncher::protocolFailure(const QString &error) {
    if (!state_->result_ || state_->result_->Status != QtOperationResult::Outcome::ProtocolFailed)
        state_->result_ = QtOperationResult{QtOperationResult::Outcome::ProtocolFailed, 2, error};
}

void QtOperationLauncher::readMessages() {
    if (!state_->socket_ || state_->terminal_)
        return;
    for (;;) {
        QtOperationMessage message;
        QString error;
        const auto state = state_->reader_.read(*state_->socket_, message, error);
        if (state == QtOperationMessageReader::State::Pending)
            return;
        if (state == QtOperationMessageReader::State::Error) {
            protocolFailure(error);
            return;
        }
        if (message.Type == QtOperationMessage::Kind::Ready && !state_->sent_ && !state_->result_) {
            state_->sent_ = true;
            if (state_->socket_->write(state_->job_) != state_->job_.size())
                protocolFailure(QCoreApplication::translate(
                    "QtOperation", "Could not transfer the operation job."));
            state_->socket_->flush();
            state_->job_.clear();
        } else if (message.Type == QtOperationMessage::Kind::Accepted && state_->sent_ &&
                   !state_->accepted_ && !state_->result_) {
            state_->accepted_ = true;
            state_->startup_.stop();
            emit accepted();
            // A receiver may close the observer. It must not kill the child.
            return;
        } else if (message.Type == QtOperationMessage::Kind::Outputs && state_->accepted_ &&
                   !state_->result_) {
            for (const auto &directory : message.Result.ChangedDirectories)
                if (!state_->changedDirectories_.contains(directory))
                    state_->changedDirectories_.append(directory);
            for (const auto &archive : message.Result.ChangedArchives)
                if (!state_->changedArchives_.contains(archive))
                    state_->changedArchives_.append(archive);
        } else if (message.Type == QtOperationMessage::Kind::Result && state_->accepted_ &&
                   !state_->result_)
            state_->result_ = message.Result;
        else if (message.Type == QtOperationMessage::Kind::Rejected && !state_->accepted_ &&
                 !state_->result_) {
            state_->result_ = message.Result;
            state_->startup_.stop();
        } else {
            protocolFailure(QCoreApplication::translate("QtOperation",
                                                        "Unexpected operation protocol message."));
            return;
        }
    }
}

void QtOperationLauncher::observe() {
    if (state_->terminal_)
        return;
    const QPointer<QtOperationLauncher> owner(this);
    acceptConnection();
    if (!owner)
        return;
    readMessages();
    if (!owner)
        return;
    const auto native = state_->process_.poll();
    if (native.Status == QtOperationProcess::State::Running)
        return;
    // A native exit can precede the socket's queued final read/EOF. Drain
    // announced output roots before publishing even a crash result.
    if (!state_->exited_.isValid())
        state_->exited_.start();
    if (state_->socket_ && state_->socket_->state() == QLocalSocket::ConnectedState &&
        state_->exited_.elapsed() < 1000)
        return;
    using Outcome = QtOperationResult::Outcome;
    if (native.Status == QtOperationProcess::State::Crashed) {
        finish({Outcome::Crashed, int(native.Code),
                QCoreApplication::translate("QtOperation",
                                            "The operation process terminated unexpectedly.")});
        return;
    }
    if (native.Status == QtOperationProcess::State::ObservationFailed) {
        finish({Outcome::ProtocolFailed, 2, native.Error});
        return;
    }
    // Native exit and local socket notification can be delivered in either
    // order. Allow buffered result/EOF notifications to reach the observer.
    if (state_->result_) {
        auto result = *state_->result_;
        if (result.Status != Outcome::ProtocolFailed && native.Code != quint32(result.ExitCode))
            result = {Outcome::ProtocolFailed, int(native.Code),
                      QCoreApplication::translate("QtOperation",
                                                  "Operation result and process exit disagree.")};
        finish(result);
    } else if (state_->exited_.elapsed() >= 1000)
        finish({Outcome::ProtocolFailed, int(native.Code),
                QCoreApplication::translate(
                    "QtOperation", "Operation process exited without a complete result.")});
}

void QtOperationLauncher::finish(QtOperationResult result) {
    if (state_->terminal_)
        return;
    state_->terminal_ = true;
    for (const auto &directory : state_->changedDirectories_)
        if (!result.ChangedDirectories.contains(directory))
            result.ChangedDirectories.append(directory);
    for (const auto &archive : state_->changedArchives_)
        if (!result.ChangedArchives.contains(archive))
            result.ChangedArchives.append(archive);
    state_->poll_.stop();
    state_->startup_.stop();
    state_->server_.close();
    if (state_->socket_)
        state_->socket_->disconnectFromServer();
    emit finished(result);
}
