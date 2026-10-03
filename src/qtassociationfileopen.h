#pragma once
#include <QObject>
#include <QStringList>

// macOS delivers associated files as application events, including before the
// window exists. Other platforms keep the source command-line startup path.
class QtAssociationFileOpen final : public QObject {
    Q_OBJECT
    QObject &_source;
    QStringList _pending;
    bool _ready = false;
    void Dispatch(const QString &path);
    bool eventFilter(QObject *watched, QEvent *event) override;

  public:
    explicit QtAssociationFileOpen(QObject &source);
    void Activate();

  signals:
    void OpenRequested(const QString &path);
};
