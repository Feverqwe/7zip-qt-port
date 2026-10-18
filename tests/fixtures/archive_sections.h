#pragma once

#include "qtdisclosure.h"

inline void ExpandArchiveSections(QWidget &dialog) {
    for (auto *section : dialog.findChildren<QtDisclosure *>())
        section->setExpanded(true);
}
