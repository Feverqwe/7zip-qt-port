#include "combodialog.h"

ComboDialog::ComboDialog(const QString &title, const QString &prompt, const QString &initial,
                         QWidget *parent)
    : QDialog(parent) {
    setObjectName("comboDialog");
    setWindowTitle(title);
    setMinimumWidth(320);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(prompt, this));
    combo_ = new QComboBox(this);
    combo_->setObjectName("comboValue");
    combo_->setEditable(true);
    combo_->setEditText(initial);
    layout->addWidget(combo_);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
