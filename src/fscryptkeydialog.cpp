#include "fscryptkeydialog.h"
#include "qtdialoglayout.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QString Hex(const QString &input) {
    QString result;
    for (const QChar c : input)
        if (!c.isSpace())
            result += c.toLower();
    return result;
}
bool IsHex(const QString &input) {
    for (const QChar c : input)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;
    return true;
}
QLabel *Explanation(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    return label;
}
} // namespace

FscryptKeyDialog::FscryptKeyDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("fscryptKeyDialog");
    setWindowTitle(tr("Open encrypted filesystem"));
    QtUseContextDialogModality(*this);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, this);
    open_ = buttons->button(QDialogButtonBox::Open);
    open_->setObjectName("fscryptOpen");
    open_->setDefault(true);
    auto *withoutKeys = buttons->addButton(tr("Browse without keys"), QDialogButtonBox::ActionRole);
    withoutKeys->setObjectName("fscryptWithoutKeys");
    withoutKeys->setAutoDefault(false);
    auto *layout = QtCreateDialogLayout(*this, *buttons);
    layout->addWidget(
        Explanation(tr("This ext filesystem uses fscrypt encryption. Enter raw master "
                       "keys from its administrator or key backup. A login password "
                       "or disk-encryption password cannot be used here."),
                    this));
    layout->addWidget(Explanation(tr("You can browse without keys, but encrypted names will be "
                                     "hidden and encrypted files cannot be copied."),
                                  this));
    area_ = new QScrollArea(this);
    area_->setObjectName("fscryptKeysArea");
    area_->setWidgetResizable(true);
    area_->setFrameShape(QFrame::NoFrame);
    auto *keys = new QWidget(area_);
    keysLayout_ = new QVBoxLayout(keys);
    keysLayout_->setContentsMargins(0, 0, 0, 0);
    keysLayout_->setAlignment(Qt::AlignTop);
    area_->setWidget(keys);
    layout->addWidget(area_, 1);
    auto *add = new QPushButton(tr("Add another key"), this);
    add->setObjectName("fscryptAddKey");
    add->setAutoDefault(false);
    layout->addWidget(add, 0, Qt::AlignLeft);
    show_ = new QCheckBox(tr("Show keys"), this);
    show_->setObjectName("fscryptShowKeys");
    layout->addWidget(show_);
    error_ = Explanation({}, this);
    error_->setObjectName("fscryptValidation");
    layout->addWidget(error_);
    connect(add, &QPushButton::clicked, this, &FscryptKeyDialog::addKey);
    connect(show_, &QCheckBox::toggled, this, [this] {
        for (const auto &row : rows_)
            row.key->setEchoMode(show_->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &FscryptKeyDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(withoutKeys, &QPushButton::clicked, this, [this] {
        description_ = QStringLiteral("fscrypt:none");
        QDialog::accept();
    });
    addKey();
    resize(560, 490);
    rows_.first().key->setFocus();
}

void FscryptKeyDialog::addKey() {
    Row row{};
    row.group = new QGroupBox(this);
    auto *form = new QFormLayout(row.group);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    row.version = new QComboBox(row.group);
    row.version->setObjectName("fscryptVersion");
    row.version->addItems({QStringLiteral("fscrypt v2"), QStringLiteral("fscrypt v1")});
    form->addRow(tr("Key version:"), row.version);
    row.key = new QLineEdit(row.group);
    row.key->setObjectName("fscryptMasterKey");
    row.key->setEchoMode(show_->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
    row.key->setPlaceholderText(tr("32–128 hexadecimal characters"));
    form->addRow(tr("Master key (hex):"), row.key);
    row.descriptor = new QLineEdit(row.group);
    row.descriptor->setObjectName("fscryptDescriptor");
    row.descriptor->setPlaceholderText(tr("16 hexadecimal characters"));
    row.descriptorLabel = new QLabel(tr("Key descriptor (hex):"), row.group);
    row.descriptorLabel->setBuddy(row.descriptor);
    form->addRow(row.descriptorLabel, row.descriptor);
    row.remove = new QPushButton(tr("Remove key"), row.group);
    row.remove->setObjectName("fscryptRemoveKey");
    row.remove->setAutoDefault(false);
    form->addRow(row.remove);
    connect(row.remove, &QPushButton::clicked, this, [this, group = row.group] {
        for (qsizetype i = 0; i < rows_.size(); ++i)
            if (rows_[i].group == group) {
                rows_[i].key->clear();
                rows_.removeAt(i);
                delete group;
                break;
            }
        updateState();
    });
    connect(row.version, &QComboBox::currentIndexChanged, this, &FscryptKeyDialog::updateState);
    connect(row.key, &QLineEdit::textChanged, this, &FscryptKeyDialog::updateState);
    connect(row.descriptor, &QLineEdit::textChanged, this, &FscryptKeyDialog::updateState);
    keysLayout_->addWidget(row.group);
    rows_.append(row);
    updateState();
    row.key->setFocus();
    QTimer::singleShot(0, this, [this, key = QPointer<QLineEdit>(row.key)] {
        if (key)
            area_->ensureWidgetVisible(key);
    });
}

QString FscryptKeyDialog::serialize(QString *error) const {
    QStringList keys;
    QSet<QString> descriptors;
    for (qsizetype i = 0; i < rows_.size(); ++i) {
        const auto &row = rows_[i];
        const QString key = Hex(row.key->text());
        if (key.size() < 32 || key.size() > 128 || key.size() % 2 || !IsHex(key)) {
            *error =
                tr("Key %1: enter 32–128 hexadecimal characters (16–64 bytes). Spaces are allowed.")
                    .arg(i + 1);
            return {};
        }
        if (row.version->currentIndex() == 1) {
            const QString descriptor = Hex(row.descriptor->text());
            if (descriptor.size() != 16 || !IsHex(descriptor)) {
                *error = tr("Key %1: the v1 descriptor must contain 16 hexadecimal characters.")
                             .arg(i + 1);
                return {};
            }
            if (descriptors.contains(descriptor)) {
                *error =
                    tr("Key %1: this v1 descriptor is already used by another key.").arg(i + 1);
                return {};
            }
            descriptors.insert(descriptor);
            keys.append(QStringLiteral("v1:") + descriptor + ':' + key);
        } else
            keys.append(QStringLiteral("v2:") + key);
    }
    return QStringLiteral("fscrypt:") + keys.join(';');
}

void FscryptKeyDialog::updateState() {
    for (qsizetype i = 0; i < rows_.size(); ++i) {
        const auto &row = rows_[i];
        row.group->setTitle(tr("Key %1").arg(i + 1));
        const bool v1 = row.version->currentIndex() == 1;
        row.descriptor->setVisible(v1);
        row.descriptorLabel->setVisible(v1);
        row.remove->setVisible(rows_.size() > 1);
    }
    QString error;
    const QString keys = serialize(&error);
    open_->setEnabled(!keys.isEmpty());
    const bool untouched = rows_.size() == 1 && rows_.first().key->text().isEmpty() &&
                           rows_.first().descriptor->text().isEmpty();
    error_->setText(untouched ? QString() : error);
}

void FscryptKeyDialog::accept() {
    QString error;
    const QString keys = serialize(&error);
    if (keys.isEmpty()) {
        error_->setText(error);
        return;
    }
    description_ = keys;
    QDialog::accept();
}
