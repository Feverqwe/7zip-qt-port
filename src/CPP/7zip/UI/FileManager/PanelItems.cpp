// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelItems.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Property snapshot access replaces COM/variant conversion. The optional
// IFolderGetItemName fast path is absent; GetItemSize uses the original tail.
#include "7zip/PropID.h"
#include "Panel.h"
#include "ViewSettings.h"

static const UInt32 kParentIndex = (UInt32)-1;

void CPanel::Get_ItemIndices_Selected(CRecordVector<UInt32> &indices) const {
    indices.Clear();
    const bool *v = _selectedStatusVector.ConstData();
    const unsigned size = _selectedStatusVector.Size();
    for (unsigned i = 0; i < size; i++)
        if (v[i])
            indices.Add(i);
}

void CPanel::GetSelectedNames(UStringVector &selectedNames) {
    CRecordVector<UInt32> indices;
    Get_ItemIndices_Selected(indices);
    selectedNames.ClearAndReserve(indices.Size());
    FOR_VECTOR(i, indices)
    selectedNames.AddInReserved(ToUString(GetItemRelPath(indices[i])));
    selectedNames.Sort();
}

void CPanel::SaveSelectedState(CSelectedState &s) {
    // Qt selection notifications do not yet use the upstream OnNotify body.
    // Synchronize its status vector at this presentation boundary.
    const unsigned numItems = (unsigned)_selectionList.GetItemCount();
    _selectedStatusVector.ClearAndReserve(numItems);
    for (unsigned i = 0; i < numItems; i++)
        _selectedStatusVector.AddInReserved(_selectionList.IsItemSelected((int)i));

    s.FocusedName_Defined = false;
    s.FocusedName.Empty();
    s.SelectFocused = true;
    s.SelectedNames.Clear();
    s.FocusedItem = _selectionList.GetFocusedItem();
    {
        if (s.FocusedItem >= 0) {
            const unsigned realIndex = GetRealItemIndex(s.FocusedItem);
            if (realIndex != kParentIndex) {
                s.FocusedName = ToUString(GetItemRelPath(realIndex));
                s.FocusedName_Defined = true;
                s.SelectFocused = _selectionList.IsItemSelected(s.FocusedItem);
            }
        }
    }
    GetSelectedNames(s.SelectedNames);
}

void CPanel::SetFocusedSelectedItem(int index, bool select) {
    _selectionList.SetFocusedItem(index);
    if (select) {
        _selectionList.SelectItem(index);
        const unsigned realIndex = GetRealItemIndex(index);
        if (realIndex != kParentIndex)
            _selectedStatusVector[realIndex] = true;
    }
}

void CPanel::RefreshListCtrl(const CSelectedState &state) {
    int focusedPos = state.FocusedItem;
    if (focusedPos < 0)
        focusedPos = 0;
    _selectionList.ClearSelection();
    const UInt32 numItems = (UInt32)_selectionList.GetItemCount();
    _selectedStatusVector.ClearAndReserve(numItems);
    int cursorIndex = -1;
    UString relPath, focusedNamePure, selectedName;
    bool needCheckSlash = false;
    if (state.FocusedName_Defined) {
        focusedNamePure = state.FocusedName;
        const int pos = focusedNamePure.ReverseFind_PathSepar();
        if (pos >= 0) {
            focusedNamePure.DeleteFrontal((unsigned)pos + 1);
            needCheckSlash = true;
        }
    }
    if (state.SelectedNames.Size() == 1) {
        selectedName = state.SelectedNames[0];
        const int pos = selectedName.ReverseFind_PathSepar();
        if (pos >= 0) {
            selectedName.DeleteFrontal((unsigned)pos + 1);
            needCheckSlash = true;
        }
    }
    for (UInt32 i = 0; i < numItems; i++) {
        const UString name = _selectionList.GetItemName(i);
        bool selected = false;
        if (state.SelectedNames.Size() > 1 ||
            (state.SelectedNames.Size() == 1 && selectedName == name) ||
            (state.FocusedName_Defined && focusedNamePure == name) ||
            (needCheckSlash && (name.Find(L'/') >= 0 || name.Find(L'\\') >= 0))) {
            // IFolder prefix/name property access becomes one Qt row snapshot.
            relPath = ToUString(GetItemRelPath(i));
            if (relPath == state.FocusedName)
                cursorIndex = (int)i;
            if (state.SelectedNames.FindInSorted(relPath) != -1)
                selected = true;
        }
        _selectedStatusVector.AddInReserved(selected);
        if (selected)
            _selectionList.SelectItem((int)i);
    }
    if (_selectionList.GetItemCount() > 0 && cursorIndex >= 0)
        SetFocusedSelectedItem(cursorIndex, state.SelectFocused);
    // Rows are already sorted; Qt indices refer to the displayed row snapshot.
    if (cursorIndex < 0 && _selectionList.GetItemCount() > 0) {
        if (focusedPos >= _selectionList.GetItemCount())
            focusedPos = _selectionList.GetItemCount() - 1;
        // Current presentation has a parent toolbar button, no dots row.
        SetFocusedSelectedItem(focusedPos, false);
    }
    _selectionList.EnsureVisible(_selectionList.GetFocusedItem());
}

UInt64 CPanel::GetItem_UInt64Prop(UInt32 itemIndex, UInt32 propID) const {
    if (itemIndex == kParentIndex)
        return 0;
    if (itemIndex >= _folder.Size())
        throw 2723400;
    const CPanelItem &item = _folder[itemIndex];
    switch (propID) {
    case kpidSize:
        return item.SizeDefined ? item.Size : 0;
    case kpidPackSize:
        return item.PackSize;
    case kpidNumSubDirs:
        return item.NumSubDirs;
    case kpidNumSubFiles:
        return item.NumSubFiles;
    }
    return 0;
}

UInt64 CPanel::GetItemSize(UInt32 itemIndex) const {
    if (itemIndex == kParentIndex)
        return 0;
    return GetItem_UInt64Prop(itemIndex, kpidSize);
}
