#pragma once
#include "archivemountdecision.h"
#include <QDialog>

// One decision for one impending extraction, never a persistent permission.
class QtMountExtractionPrompt final : public QDialog {
    Q_OBJECT
    ArchiveMountDecisionPtr decision_;

  public:
    QtMountExtractionPrompt(const QString &source, const QString &path, quint64 size,
                            ArchiveMountDecisionPtr decision, QWidget *parent = nullptr,
                            bool unverified = false);
    ~QtMountExtractionPrompt() override;
};
