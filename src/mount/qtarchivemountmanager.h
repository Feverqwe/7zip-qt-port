#pragma once
#include <QCoreApplication>
#include <QDialog>
#include <QHash>
#include <QList>

class QtArchiveMountSession;
struct IUnknown;
class QTreeWidget;
class QPlainTextEdit;
class QPushButton;
class QAction;

// Modeless mount management, independent of either file-manager panel.
class QtArchiveMountManager final : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(QtArchiveMountManager)
    QWidget *owner_;
    QList<QtArchiveMountSession *> sessions_;
    QHash<QtArchiveMountSession *, QStringList> issues_;
    QTreeWidget *list_;
    QPlainTextEdit *details_;
    QPushButton *open_, *copy_, *unmount_, *force_;
    QAction *remove_;
    bool closingOwner_ = false;
    QtArchiveMountSession *selected() const;
    void refresh();
    void updateDetails();
    void unmountSelected(bool force);
    void removeSelected();
    void showManager();
    void retranslate();

  protected:
    void changeEvent(QEvent *event) override;

  public:
    explicit QtArchiveMountManager(QWidget *owner);
    void mount(IUnknown *resource, const QString &label, const QString &archivePassword,
               const QString &folder = {});
    bool canCloseOwner();
};
