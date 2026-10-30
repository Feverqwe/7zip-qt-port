#pragma once
#include <QString>
#include <memory>
class QWidget;
class ArchiveReadOnlyView;
struct IUnknown;
struct QtMountOptions {
    bool Accepted = false;
    quint64 Quota = 0;
    QString Password;
    std::shared_ptr<ArchiveReadOnlyView> View;
};
QtMountOptions QtAskMountOptions(QWidget *owner, const QString &source, const QString &password,
                                 IUnknown *resource = nullptr, const QString &folder = {});
