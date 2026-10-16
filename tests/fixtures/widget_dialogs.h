#pragma once

#include <QDialog>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <functional>

// Public-Qt responders and widget geometry inspect Qt dialogs. Native OS panels
// require their separate platform/manual acceptance rather than these fixtures.
class WidgetDialogFixture final : public QObject {
    const bool previous_;
    QHash<const QDialog *, int> showLevels_;
    bool eventFilter(QObject *watched, QEvent *event) override;

  public:
    WidgetDialogFixture();
    ~WidgetDialogFixture() override;
    bool ready(QDialog &dialog) const;
    QDialog *activeDialog() const;
    void respond(QObject &context, std::function<void(QDialog &)> callback);
    void connectResponder(QTimer &timer, QObject *context, std::function<void()> callback) const;
    WidgetDialogFixture(const WidgetDialogFixture &) = delete;
    WidgetDialogFixture &operator=(const WidgetDialogFixture &) = delete;
};
