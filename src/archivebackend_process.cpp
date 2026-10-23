#include "CPP/7zip/UI/Common/CompressCall.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "qtarchivereadoperation.h"
#include "qtownedtempdir.h"
#include <QCoreApplication>
#include <QFile>
#include <QPointer>
#include <QRegularExpression>
#include <utility>

std::shared_ptr<QProcess> ArchiveBackend::makeProcess(const QString &executable) {
    std::shared_ptr<QProcess> process(new QProcess, [](QProcess *value) {
        // A backend receiver can release us inside QProcess's own event frame.
        // Keep that sender alive until Qt returns; the app also owns cleanup if
        // its event loop stops before deferred deletion is delivered.
        if (auto *app = QCoreApplication::instance()) {
            if (app->thread() == value->thread())
                value->setParent(app);
            value->deleteLater();
        } else {
            // Native shell hosts can construct an idle backend without a Qt
            // application; there is no event loop to own deferred deletion.
            delete value;
        }
    });
    process->setProgram(executable);
    auto *transport = process.get();
    connect(transport, &QProcess::started, this, [this, transport] {
        if (process_.get() != transport)
            return;
        processPause_.Attach(UInt32(transport->processId()));
        if (!cancelled_)
            emit engineStarted();
    });
    connect(transport, &QProcess::readyRead, this, [this, transport] {
        if (process_.get() == transport)
            consumeOutput(transport->readAll());
    });
    connect(transport, &QProcess::readyReadStandardError, this, [this, transport] {
        if (process_.get() == transport && job_ == Job::SourceExtract)
            consumeOutput(transport->readAllStandardError());
    });
    connect(transport, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, transport](int code, QProcess::ExitStatus status) {
                if (process_.get() == transport)
                    complete(code, status);
            });
    connect(
        transport, &QProcess::errorOccurred, this, [this, transport](QProcess::ProcessError error) {
            if (process_.get() != transport)
                return;
            if (error == QProcess::FailedToStart && job_ != Job::None) {
                processPause_.ResetAfterExit();
                if (job_ == Job::Create || job_ == Job::Add || job_ == Job::Delete ||
                    job_ == Job::Rename || job_ == Job::ReplaceDelete || job_ == Job::ReplaceAdd ||
                    job_ == Job::SourceUpdate || job_ == Job::SourcePlanDelete ||
                    job_ == Job::SourcePlanFiles || job_ == Job::SourcePlanDirectories ||
                    job_ == Job::SourcePlanRename || job_ == Job::SourceMutation ||
                    job_ == Job::SourceExtract)
                    QFile::remove(createTemporary_);
                job_ = Job::None;
                sourceExtractSelection_.reset();
                extractQueue_.clear();
                pendingPassword_.clear();
                createStages_.clear();
                createPassword_.clear();
                extractPassword_.clear();
                sourcePlanPassword_.clear();
                emit finished(false,
                              QCoreApplication::translate(
                                  "archivebackend",
                                  "Could not start 7zz. Install official 7zz and add it to PATH."));
            }
        });
    return process;
}

void ArchiveBackend::start(Job job, const QStringList &arguments, const QString &workingDirectory) {
    if (busy()) {
        emit finished(false, QCoreApplication::translate("archivebackend",
                                                         "Another operation is still running."));
        return;
    }
    auto next = makeProcess(process_->program());
    disconnect(process_.get(), nullptr, this, nullptr);
    process_ = std::move(next);
    const auto transport = process_;
    const QPointer<ArchiveBackend> owner(this);
    job_ = job;
    lastResult_ = E_FAIL;
    output_.clear();
    listingOutput_.clear();
    pendingOutput_.clear();
    cancelled_ = false;
    passwordPrompted_ = false;
    process_->setWorkingDirectory(workingDirectory);
    process_->setProcessChannelMode(job == Job::SourceExtract ? QProcess::SeparateChannels
                                                              : QProcess::MergedChannels);
    process_->setStandardOutputFile(job == Job::SourceExtract ? createTemporary_ : QString());
    QStringList processArguments = arguments;
    processArguments.insert(1, "-sccUTF-8");
    if (IsLargePageSupported()) {
        UString switches;
        AddLagePagesSwitch(switches, FmSettingsPath());
        if (!switches.IsEmpty())
            processArguments.insert(1, QString::fromWCharArray(switches).trimmed());
    }
    process_->setArguments(processArguments);
    process_->start();
    if (!owner || process_ != transport || job_ != job)
        return;
    if (!pendingPassword_.isEmpty())
        providePassword(pendingPassword_);
}

void ArchiveBackend::providePassword(const QString &password) {
    if (!busy())
        return;
    pendingPassword_ = password;
    if (readOperation_) {
        readOperation_->providePassword(password);
        return;
    }
    if (job_ == Job::Extract)
        extractPassword_ = password;
    process_->write(password.toUtf8() + '\n');
}

void ArchiveBackend::consumeLine(const QByteArray &line) {
    QString text = QString::fromUtf8(line);
    text.remove(QLatin1Char('\b'));
    text = text.trimmed();
    if (!pendingPassword_.isEmpty())
        text.replace(pendingPassword_, QCoreApplication::translate("archivebackend", "[hidden]"));
    if (text.isEmpty())
        return;
    const QPointer<ArchiveBackend> owner(this);
    const auto transport = process_;
    const Job job = job_;
    output_ += text + '\n';
    emit outputReceived(text);
    if (!owner || process_ != transport || job_ != job)
        return;
    static const QRegularExpression percent(R"((?:^|\s)(\d{1,3})%)");
    auto match = percent.globalMatch(text);
    int value = -1;
    QString detail;
    while (match.hasNext()) {
        const auto found = match.next();
        value = found.captured(1).toInt();
        detail = text.mid(found.capturedEnd()).trimmed();
    }
    if (value >= 0 && value <= 100) {
        if (detail.startsWith('+') || detail.startsWith('-'))
            detail = detail.mid(1).trimmed();
        emit progress(value, detail);
    } else if (text.startsWith("+ ") || text.startsWith("- ")) {
        emit progress(-1, text.mid(2).trimmed());
    }
}

void ArchiveBackend::consumeOutput(const QByteArray &chunk, bool flush) {
    // Metadata must retain spaces, blank/multiline values and fragmented UTF-8.
    // The existing trimmed/redacted progress journal is a separate presentation.
    if (job_ == Job::List)
        listingOutput_ += chunk;
    pendingOutput_ += chunk;
    const bool askPassword = !passwordPrompted_ && pendingPassword_.isEmpty() &&
                             (pendingOutput_.contains("Enter password:") ||
                              pendingOutput_.contains("Enter password (will not be echoed):"));
    // Retire the current batch before any receiver can synchronously drain the
    // process again. Keep incomplete UTF-8/lines pending for the next read.
    QByteArray batch;
    if (flush)
        batch = std::exchange(pendingOutput_, QByteArray());
    else {
        const qsizetype end =
            qMax(pendingOutput_.lastIndexOf('\r'), pendingOutput_.lastIndexOf('\n'));
        if (end >= 0) {
            batch = pendingOutput_.left(end + 1);
            pendingOutput_.remove(0, end + 1);
        }
    }
    const QPointer<ArchiveBackend> owner(this);
    const auto transport = process_;
    const Job job = job_;
    if (askPassword) {
        passwordPrompted_ = true;
        emit passwordRequired();
        if (!owner || process_ != transport || job_ != job)
            return;
    }
    qsizetype start = 0;
    while (true) {
        const qsizetype cr = batch.indexOf('\r', start);
        const qsizetype lf = batch.indexOf('\n', start);
        const qsizetype end = cr < 0 ? lf : (lf < 0 ? cr : qMin(cr, lf));
        if (end < 0)
            break;
        if (end > start) {
            consumeLine(batch.mid(start, end - start));
            if (!owner || process_ != transport || job_ != job)
                return;
        }
        start = end + 1;
    }
    if (flush && start < batch.size())
        consumeLine(batch.mid(start));
}
