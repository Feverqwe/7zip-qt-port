#pragma once

#include <QDialog>

class QCheckBox;

struct FileListSettings {
    bool realFileIcons = false;
    bool grid = false;
    bool singleClick = false;
};

class SettingsDialog : public QDialog {
  public:
    explicit SettingsDialog(const FileListSettings &settings, QWidget *parent = nullptr);
    FileListSettings settings() const;

  private:
    QCheckBox *realFileIcons_;
    QCheckBox *grid_;
    QCheckBox *singleClick_;
};
