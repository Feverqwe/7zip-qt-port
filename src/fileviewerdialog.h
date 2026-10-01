#pragma once

#include <QDialog>

class QPlainTextEdit;

class FileViewerDialog : public QDialog {
    Q_OBJECT

  public:
    FileViewerDialog(const QString &path, bool editable, QWidget *parent = nullptr);
    bool wasSaved() const { return wasSaved_; }

  private:
    void save();
    void reject() override;
    void changeEvent(QEvent *event) override;
    void setReadOnlyMessage(const QString &english, const QString &russian,
                            const QString &detail = {});

    QString path_;
    QPlainTextEdit *text_ = nullptr;
    bool editable_ = false;
    bool wasSaved_ = false;
    QByteArray original_;
    QString messageEnglish_;
    QString messageRussian_;
    QString messageDetail_;
};
