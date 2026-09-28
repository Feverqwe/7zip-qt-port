// Adapted from 7-Zip CPP/7zip/UI/FileManager/CopyDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include <QDialog>
#include <QStringList>

class QComboBox;

const int kCopyDialog_NumInfoLines = 11;

class CCopyDialog : public QDialog {
    Q_OBJECT

  public:
    CCopyDialog(bool move, const QString &initialPath, const QStringList &sources,
                const QStringList &history, QWidget *parent = nullptr,
                const QString &info = QString());
    QString Title;
    QString Static;
    QString Value;
    QString Info;
    QStringList Strings;

    QString destination() const;
    void accept() override;

  private:
    QComboBox *_path = nullptr;
    bool OnInit();
    void OnButtonSetPath();
    void OnOK();
};
