#pragma once

#include "volumeoperations.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class VolumeDialog : public QDialog {
    Q_OBJECT

  public:
    VolumeDialog(VolumeOperation::Mode mode, const QString &source, QWidget *parent = nullptr,
                 const VolumeParts &parts = {});

    QString destination() const { return destination_->text(); }
    QList<qint64> volumeSizes() const {
        return size_ ? VolumeOperation::parseSizes(size_->currentText()) : QList<qint64>{};
    }
    qint64 volumeSize() const {
        return size_ ? VolumeOperation::parseSize(size_->currentText()) : 0;
    }

  private:
    VolumeOperation::Mode mode_;
    QLineEdit *destination_ = nullptr;
    QComboBox *size_ = nullptr;
    QLabel *error_ = nullptr;
};
