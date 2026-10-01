// Adapted from 7-Zip CPP/7zip/UI/GUI/ExtractDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "../Common/ZipRegistry.h"
#include "archivebackend.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>

class CExtractDialog : public QDialog {
    Q_OBJECT
    QComboBox *_path = nullptr;
    QLineEdit *_pathName = nullptr;
    QCheckBox *_nameEnable = nullptr;
    QLineEdit *_passwordControl = nullptr;
    QComboBox *_pathMode = nullptr;
    QComboBox *_overwriteMode = nullptr;
    QCheckBox *_showPassword = nullptr;
    NExtract::CInfo _info;
    void OnInit(const QStringList &history);
    void OnOK();
    void UpdatePasswordControl();
    void OnButtonSetPath();
    void changeEvent(QEvent *event) override;
    static QString settingsPath();
    bool IsShowPasswordChecked() const { return _showPassword->isChecked(); }

  public:
    CExtractDialog(const QString &initialPath, const QStringList &history, bool selected,
                   QWidget *parent = nullptr);
    QString DirPath;
    QString Password;
    bool PathMode_Force = false;
    bool OverwriteMode_Force = false;
    NExtract::NPathMode::EEnum PathMode = NExtract::NPathMode::kFullPaths;
    NExtract::NOverwriteMode::EEnum OverwriteMode = NExtract::NOverwriteMode::kAsk;
    QString destination() const;
    QString password() const { return _passwordControl->text(); }
    ExtractionOptions options() const;
    void SetInitialOptions(NExtract::NPathMode::EEnum pathMode,
                           NExtract::NOverwriteMode::EEnum overwriteMode, const QString &password);
  public slots:
    void accept() override { OnOK(); }
};
