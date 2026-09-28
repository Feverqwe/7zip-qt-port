// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSelect.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// CPanelSelection extracts CPanel's selection vector and rules. QString and
// std::vector replace UString and the folder's indexed properties; dialog,
// focus and UpdateSelection belong to the presentation adapter.

#include "PanelSelect.h"
#include "../../../Common/Wildcard.h"

void CPanelSelection::SelectSpec(QStringView mask, bool selectMode) {
    for (auto &item : Items)
        if (DoesWildcardMatchName(mask, QStringView(item.Name)))
            item.Selected = selectMode;
}

void CPanelSelection::SelectByType(int focusedItem, bool selectMode) {
    if (focusedItem < 0 || size_t(focusedItem) >= Items.size())
        return;
    const QString &name = Items[size_t(focusedItem)].Name;
    const bool isItemFolder = Items[size_t(focusedItem)].IsFolder;
    if (isItemFolder) {
        for (auto &item : Items)
            if (item.IsFolder == isItemFolder)
                item.Selected = selectMode;
    } else {
        const qsizetype pos = name.lastIndexOf('.');
        if (pos < 0) {
            for (auto &item : Items)
                if (item.IsFolder == isItemFolder && item.Name.lastIndexOf('.') < 0)
                    item.Selected = selectMode;
        } else {
            const QString mask = QStringLiteral("*") + name.mid(pos);
            for (auto &item : Items)
                if (item.IsFolder == isItemFolder &&
                    DoesWildcardMatchName(QStringView(mask), QStringView(item.Name)))
                    item.Selected = selectMode;
        }
    }
}

void CPanelSelection::SelectAll(bool selectMode) {
    for (auto &item : Items)
        item.Selected = selectMode;
}

void CPanelSelection::InvertSelection() {
    for (auto &item : Items)
        item.Selected = !item.Selected;
}
