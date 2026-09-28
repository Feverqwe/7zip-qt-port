#include "copydialog.h"

CopyDialog::CopyDialog(bool move, const QString &initialPath, const QStringList &sources,
                       const QStringList &history, QWidget *parent)
    : QDialog(parent) {
    setObjectName("copyDialog");
    setWindowTitle(move ? "Переместить" : "Копировать");
    resize(440, 250);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(move ? "Переместить в:" : "Копировать в:", this));
    auto *pathRow = new QHBoxLayout;
    destination_ = new QComboBox(this);
    destination_->setObjectName("copyDestination");
    destination_->setEditable(true);
    destination_->setInsertPolicy(QComboBox::NoInsert);
    destination_->addItems(history);
    destination_->setCurrentText(initialPath);
    pathRow->addWidget(destination_);
    auto *browse = new QPushButton("…", this);
    browse->setObjectName("copyBrowseButton");
    pathRow->addWidget(browse);
    layout->addLayout(pathRow);
    QStringList names;
    for (const QString &source : sources)
        names << QFileInfo(source).fileName();
    auto *info = new QLabel(names.join('\n'), this);
    info->setObjectName("copySelectionInfo");
    info->setWordWrap(true);
    layout->addWidget(info, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getExistingDirectory(this, "Папка назначения",
                                                               destination_->currentText());
        if (!path.isEmpty())
            destination_->setCurrentText(path);
    });
}
