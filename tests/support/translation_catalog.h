#pragma once
#include <QList>
#include <QPair>
#include <QString>

bool WriteTranslationCatalog(const QString &directory,
                             const QList<QPair<unsigned, QString>> &messages,
                             QString *error = nullptr);
