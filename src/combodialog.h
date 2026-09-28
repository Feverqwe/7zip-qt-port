#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

class ComboDialog : public QDialog {
  public:
    ComboDialog(const QString &title, const QString &prompt, const QString &initial,
                QWidget *parent = nullptr);

    QString value() const { return combo_->currentText(); }

  private:
    QComboBox *combo_ = nullptr;
};
