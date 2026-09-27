#pragma once

#include <QDialog>

class QPlainTextEdit;

class FileViewerDialog : public QDialog {
    Q_OBJECT

  public:
    FileViewerDialog(const QString &path, bool editable, QWidget *parent = nullptr);

  private:
    void save();
    void reject() override;

    QString path_;
    QPlainTextEdit *text_ = nullptr;
    bool editable_ = false;
    bool changedOnDisk_ = false;
    QByteArray original_;
};
