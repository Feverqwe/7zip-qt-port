#pragma once

#include "archivebackend.h"

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

// The controls follow the path and overwrite choices in GUI/ExtractDialog.cpp.
class ExtractDialog : public QDialog {
    Q_OBJECT

  public:
    ExtractDialog(const QString &initialPath, const QStringList &history, bool selected,
                  QWidget *parent = nullptr);

    QString destination() const { return destination_->currentText().trimmed(); }
    QString password() const { return password_->text(); }
    ExtractionOptions options() const;

  private:
    QComboBox *destination_ = nullptr;
    QComboBox *pathMode_ = nullptr;
    QComboBox *overwriteMode_ = nullptr;
    QLineEdit *password_ = nullptr;
};
