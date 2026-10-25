#include "qtmountsettings.h"
#include "qtdialoglayout.h"
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
class MountLabel final : public QLabel {
  public:
    using QLabel::QLabel;

  protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        // As in the benchmark's wrapped-caption boundary, native layouts can
        // otherwise use the wider layout hint when calculating this height.
        setMinimumHeight(heightForWidth(event->size().width()));
    }
};
} // namespace

QtMountOptions QtAskMountOptions(QWidget *owner, const QString &label,
                                 const QString &archivePassword) {
    QDialog settings(owner);
    settings.setObjectName("archiveMountSettings");
    settings.setWindowTitle(
        QCoreApplication::translate("QtArchiveMountManager", "Mount read-only"));
    QtUseContextDialogModality(settings);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &settings);
    auto *layout = QtCreateDialogLayout(settings, *buttons);
    auto *description = new MountLabel(
        QCoreApplication::translate(
            "QtArchiveMountManager",
            "Expose this container to other applications through a local read-only mount. "
            "Files that require extraction will ask for confirmation individually."),
        &settings);
    description->setWordWrap(true);
    layout->addWidget(description);
    auto *source = new MountLabel(label, &settings);
    source->setTextFormat(Qt::PlainText);
    source->setWordWrap(true);
    layout->addWidget(source);
    auto *form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto *quota = new QSpinBox(&settings);
    quota->setObjectName("mountCacheLimit");
    quota->setMinimumWidth(160);
    quota->setRange(16, 1048576);
    quota->setValue(1024);
    quota->setSuffix(QCoreApplication::translate("QtArchiveMountManager", " MiB"));
    form->addRow(QCoreApplication::translate("QtArchiveMountManager", "Temporary cache limit:"),
                 quota);
    auto *password = new QLineEdit(archivePassword, &settings);
    password->setObjectName("mountArchivePassword");
    password->setMinimumWidth(160);
    password->setEchoMode(QLineEdit::Password);
    form->addRow(
        QCoreApplication::translate("QtArchiveMountManager", "Archive password (if needed):"),
        password);
    layout->addLayout(form);
    auto *start = buttons->addButton(QCoreApplication::translate("QtArchiveMountManager", "Mount"),
                                     QDialogButtonBox::AcceptRole);
    start->setDefault(true);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &settings, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &settings, &QDialog::reject);
    settings.resize(520, settings.sizeHint().height());
    if (settings.exec() != QDialog::Accepted)
        return {};
    return {true, quint64(quota->value()) * 1024 * 1024, password->text()};
}
