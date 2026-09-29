// PanelSelect.cpp — adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSelect.cpp
// at 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.

#include "../../../Common/Wildcard.h"
#include "Panel.h"

static const UInt32 kParentIndex = (UInt32)-1;

void CPanel::UpdateSelection() {
    if (!_mySelectMode) {
        CDisableNotify disableNotify(*this);
        const int numItems = _selectionList.GetItemCount();
        for (int i = 0; i < numItems; i++) {
            const unsigned realIndex = GetRealItemIndex(i);
            if (realIndex != kParentIndex)
                _selectionList.SetItemState_Selected(i, _selectedStatusVector[realIndex]);
        }
    }
    _selectionList.Invalidate();
}

// The Qt caller owns the CComboDialog; the source mask loop remains in CPanel.
void CPanel::SelectSpec(const UString &mask, bool selectMode) {
    FOR_VECTOR(i, _selectedStatusVector)
    if (DoesWildcardMatchName(mask, GetItemName(i)))
        _selectedStatusVector[i] = selectMode;
    UpdateSelection();
}

void CPanel::SelectByType(bool selectMode) {
    const int focusedItem = _selectionList.GetFocusedItem();
    if (focusedItem < 0)
        return;
    const unsigned realIndex = GetRealItemIndex(focusedItem);
    if (realIndex == kParentIndex)
        return;
    UString name = GetItemName(realIndex);
    bool isItemFolder = IsItem_Folder(realIndex);

    if (isItemFolder) {
        FOR_VECTOR(i, _selectedStatusVector)
        if (IsItem_Folder(i) == isItemFolder)
            _selectedStatusVector[i] = selectMode;
    } else {
        int pos = name.ReverseFind_Dot();
        if (pos < 0) {
            FOR_VECTOR(i, _selectedStatusVector)
            if (IsItem_Folder(i) == isItemFolder && GetItemName(i).ReverseFind_Dot() < 0)
                _selectedStatusVector[i] = selectMode;
        } else {
            UString mask('*');
            mask += name.Ptr((unsigned)pos);
            FOR_VECTOR(i, _selectedStatusVector)
            if (IsItem_Folder(i) == isItemFolder && DoesWildcardMatchName(mask, GetItemName(i)))
                _selectedStatusVector[i] = selectMode;
        }
    }

    UpdateSelection();
}

void CPanel::SelectAll(bool selectMode) {
    FOR_VECTOR(i, _selectedStatusVector)
    _selectedStatusVector[i] = selectMode;
    UpdateSelection();
}

void CPanel::InvertSelection() {
    FOR_VECTOR(i, _selectedStatusVector)
    _selectedStatusVector[i] = !_selectedStatusVector[i];
    UpdateSelection();
}
