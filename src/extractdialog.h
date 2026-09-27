#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

// The controls follow ExtractDialog.rc. Unsupported path and overwrite modes are
// displayed as fixed values so the UI cannot request a backend mode it lacks.
class ExtractDialog : public QDialog {
    Q_OBJECT

  public:
    ExtractDialog(const QString &initialPath, const QStringList &history, bool selected,
                  QWidget *parent = nullptr)
        : QDialog(parent) {
        setObjectName("extractDialog");
        setWindowTitle(selected ? "Извлечь выбранное" : "Извлечь");
        resize(470, 330);
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Извлечь в:", this));
        auto *pathRow = new QHBoxLayout;
        destination_ = new QComboBox(this);
        destination_->setObjectName("extractDestination");
        destination_->setEditable(true);
        destination_->setInsertPolicy(QComboBox::NoInsert);
        destination_->addItems(history);
        destination_->setCurrentText(initialPath);
        pathRow->addWidget(destination_, 1);
        auto *browse = new QPushButton("…", this);
        browse->setObjectName("extractBrowseButton");
        pathRow->addWidget(browse);
        layout->addLayout(pathRow);

        auto *options = new QHBoxLayout;
        auto *modeColumn = new QVBoxLayout;
        modeColumn->addWidget(new QLabel("Пути:", this));
        auto *pathMode = new QComboBox(this);
        pathMode->setObjectName("extractPathMode");
        pathMode->addItem("Полные пути");
        pathMode->setEnabled(false);
        modeColumn->addWidget(pathMode);
        modeColumn->addWidget(new QLabel("Перезапись:", this));
        auto *overwriteMode = new QComboBox(this);
        overwriteMode->setObjectName("extractOverwriteMode");
        overwriteMode->addItem("Не перезаписывать существующие файлы");
        overwriteMode->setEnabled(false);
        modeColumn->addWidget(overwriteMode);
        options->addLayout(modeColumn, 1);

        auto *passwordGroup = new QGroupBox("Пароль", this);
        auto *passwordLayout = new QVBoxLayout(passwordGroup);
        password_ = new QLineEdit(passwordGroup);
        password_->setObjectName("extractPassword");
        password_->setEchoMode(QLineEdit::Password);
        passwordLayout->addWidget(password_);
        auto *showPassword = new QCheckBox("Показать пароль", passwordGroup);
        showPassword->setObjectName("extractShowPassword");
        passwordLayout->addWidget(showPassword);
        options->addWidget(passwordGroup, 1);
        layout->addLayout(options);
        auto *hint = new QLabel("Папка назначения должна существовать. Пути внутри архива "
                                "сохраняются; существующие файлы не перезаписываются.",
                                this);
        hint->setWordWrap(true);
        layout->addWidget(hint);
        layout->addStretch();
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getExistingDirectory(this, "Папка для извлечения",
                                                                   destination_->currentText());
            if (!path.isEmpty())
                destination_->setCurrentText(path);
        });
        connect(showPassword, &QCheckBox::toggled, password_, [this](bool visible) {
            password_->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
        });
    }

    QString destination() const { return destination_->currentText().trimmed(); }
    QString password() const { return password_->text(); }

  private:
    QComboBox *destination_ = nullptr;
    QLineEdit *password_ = nullptr;
};
