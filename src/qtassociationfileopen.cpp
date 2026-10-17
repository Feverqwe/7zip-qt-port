#include "qtassociationfileopen.h"
#include <QFileOpenEvent>
#include <QTimer>
#include <QUrl>

QtAssociationFileOpen::QtAssociationFileOpen(QObject &source) : _source(source) {
    _source.installEventFilter(this);
}

void QtAssociationFileOpen::Dispatch(const QString &path) {
    QTimer::singleShot(0, this, [this, path] { emit OpenRequested(path); });
}

bool QtAssociationFileOpen::Activate() {
    if (_ready)
        return false;
    _ready = true;
    const QStringList pending = std::move(_pending);
    for (const auto &path : pending)
        Dispatch(path);
    return !pending.isEmpty();
}

bool QtAssociationFileOpen::eventFilter(QObject *watched, QEvent *event) {
    if (watched != &_source || event->type() != QEvent::FileOpen)
        return false;
    const auto *request = static_cast<QFileOpenEvent *>(event);
    if (!request->url().isLocalFile() || request->file().isEmpty())
        return false;
    const QString path = request->file();
    if (_ready)
        Dispatch(path);
    else
        _pending.append(path);
    event->accept();
    return true;
}
