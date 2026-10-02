#pragma once

#include <QAbstractItemView>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QTimer>

// Qt presentation adapter for 7zFM LVS_EX_TRACKSELECT on the main file list.
class QtFileListHover final : public QObject {
  public:
    explicit QtFileListHover(QAbstractItemView &view);
    void SetEnabled(bool enabled);
    void SetSuspended(bool suspended) {
        suspended_ = suspended;
        Cancel();
    }

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void Cancel();
    QPointer<QAbstractItemView> view_;
    QTimer timer_;
    QPersistentModelIndex index_;
    QPoint position_;
    bool enabled_ = false;
    bool suspended_ = false;
};
