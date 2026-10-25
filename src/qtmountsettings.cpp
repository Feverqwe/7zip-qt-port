#include "qtmountsettings.h"
#include "archivebackend.h"
#include "archivereadonlyview.h"
#include "qtdialoglayout.h"
#include "qthelpservices.h"
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QShortcut>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>

namespace {
class MountSettingsDialog final : public QDialog {
  public:
    using QDialog::QDialog;
    bool Ready = true;
    bool Preparing = false, Cancelled = false;
    std::function<void()> CancelPreparation;
    void accept() override {
        if (Ready)
            QDialog::accept();
    }
    void reject() override {
        if (Preparing) {
            if (!Cancelled) {
                Cancelled = true;
                CancelPreparation();
            }
        } else {
            QDialog::reject();
        }
    }
};

class MountPreparation final : public QThread {
    void run() override {
        Result = ArchiveBackend::prepareReadOnlyView(View, Error);
        if (Result == S_OK)
            Result = View->needsCache(CacheRequired);
    }

  public:
    std::shared_ptr<ArchiveReadOnlyView> View;
    HRESULT Result = E_FAIL;
    QString Error;
    bool CacheRequired = true;
    MountPreparation(IUnknown *resource, const QString &password, const QString &folder)
        : View(ArchiveBackend::retainReadOnlyView(resource, 0, password, {}, {}, folder)) {}
    void cancel() {
        View->cancel();
        requestInterruption();
    }
};

void ShowCacheHelp(QWidget *parent) {
    QtOpenHelpText(
        QCoreApplication::translate("QtArchiveMountManager", "Temporary cache"),
        QCoreApplication::translate(
            "QtArchiveMountManager",
            "Sevenzip connects the folder currently open in the file manager. It reuses the open "
            "reader and any temporary container files already created.\n\n"
            "Files that support direct reading are passed to other applications without "
            "extraction. For example, a ZIP inside a filesystem in a QCOW image is read as an "
            "ordinary ZIP file.\n\n"
            "When connecting a folder inside a compressed archive, files may need decoding. "
            "They are extracted with your confirmation when another application reads them, "
            "and kept in a temporary disk cache. The source remains read-only.\n\n"
            "The limit is the total size of these extracted files. Older cached files are "
            "removed to make room and may need to be extracted again. If one file exceeds "
            "the limit, that file cannot be read through this connection; other files and "
            "the connection remain available. Directly read files are not subject to this limit."),
        parent);
}

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
                                 const QString &archivePassword, IUnknown *resource,
                                 const QString &folder) {
    MountSettingsDialog settings(owner);
    settings.Ready = !resource;
    settings.setObjectName("archiveMountSettings");
    settings.setWindowTitle(
        QCoreApplication::translate("QtArchiveMountManager", "Mount read-only"));
    QtUseContextDialogModality(settings);
    auto *buttons =
        new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Help, &settings);
    auto *help = buttons->button(QDialogButtonBox::Help);
    help->setObjectName("mountCacheHelpButton");
    auto *layout = QtCreateDialogLayout(settings, *buttons);
    auto *source = new MountLabel(label, &settings);
    source->setTextFormat(Qt::PlainText);
    source->setWordWrap(true);
    layout->addWidget(source);
    auto *checking = new QLabel(
        QCoreApplication::translate("QtArchiveMountManager", "Checking file access…"), &settings);
    checking->setWordWrap(true);
    checking->setVisible(resource != nullptr);
    layout->addWidget(checking);
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
    start->setEnabled(settings.Ready);
    auto *shortcut = new QShortcut(QKeySequence::HelpContents, &settings);
    const auto showHelp = [&settings, quota] {
        if (quota->isVisible())
            ShowCacheHelp(&settings);
    };
    QObject::connect(buttons, &QDialogButtonBox::helpRequested, &settings, showHelp);
    QObject::connect(shortcut, &QShortcut::activated, &settings, showHelp);
    const auto showCache = [form, quota, help, shortcut](bool required) {
        form->setRowVisible(quota, required);
        help->setVisible(required);
        shortcut->setEnabled(required);
    };
    bool cacheRequired = !resource;
    showCache(cacheRequired);
    std::unique_ptr<MountPreparation> preparation;
    if (resource) {
        preparation = std::make_unique<MountPreparation>(resource, archivePassword, folder);
        settings.Preparing = true;
        settings.CancelPreparation = [&] {
            preparation->cancel();
            checking->setText(QCoreApplication::translate("QtArchiveMountManager", "Cancelling…"));
            buttons->setEnabled(false);
        };
        QObject::connect(preparation.get(), &QThread::finished, &settings, [&] {
            settings.Preparing = false;
            if (settings.Cancelled) {
                settings.reject();
                return;
            }
            if (preparation->Result != S_OK) {
                checking->setText(preparation->Error.isEmpty()
                                      ? QCoreApplication::translate("QtArchiveMountManager",
                                                                    "Could not check file access.")
                                      : preparation->Error);
                return;
            }
            cacheRequired = preparation->CacheRequired;
            showCache(cacheRequired);
            checking->hide();
            settings.Ready = true;
            start->setEnabled(true);
            settings.resize(settings.width(), settings.sizeHint().height());
        });
        preparation->start();
    }
    QObject::connect(buttons, &QDialogButtonBox::accepted, &settings, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &settings, &QDialog::reject);
    settings.resize(520, settings.sizeHint().height());
    const bool accepted = settings.exec() == QDialog::Accepted;
    if (preparation) {
        if (!accepted)
            preparation->cancel();
        preparation->wait();
    }
    if (!accepted)
        return {};
    return {true, cacheRequired ? quint64(quota->value()) * 1024 * 1024 : 0, password->text(),
            preparation ? preparation->View : nullptr};
}
