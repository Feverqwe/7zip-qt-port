#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

class CopyDialog : public QDialog {
    Q_OBJECT

  public:
    CopyDialog(bool move, const QString &initialPath, const QStringList &sources,
               const QStringList &history, QWidget *parent = nullptr);

    QString destination() const { return destination_->currentText(); }

  private:
    QComboBox *destination_ = nullptr;
};
