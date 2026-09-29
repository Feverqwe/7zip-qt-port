// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelItems.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// IFolder/variant property bodies retain upstream control flow. Qt list rows
// remain a presentation snapshot pending the full folder-load port.
#include "7zip/PropID.h"
#include "Panel.h"
#include "ViewSettings.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "Windows/WinDefs.h"

static const UInt32 kParentIndex = (UInt32)-1;

unsigned CPanel::GetRealItemIndex(int indexInListView) const {
    quintptr param;
    if (!_selectionList.GetItemParam((unsigned)indexInListView, param))
        throw 1;
    return (unsigned)param;
}

UInt32 CPanel::GetFolderItemCount() const {
    if (!_folder)
        return UInt32(_selectionList.GetItemCount());
    UInt32 numItems;
    if (_folder->GetNumberOfItems(&numItems) != S_OK)
        throw 2723400;
    return numItems;
}

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
    selectedNames.AddInReserved(GetItemRelPath(indices[i]));
    selectedNames.Sort();
}

void CPanel::Get_ItemIndices_Operated(CRecordVector<UInt32> &indices) const {
    Get_ItemIndices_Selected(indices);
    if (!indices.IsEmpty())
        return;
    if (_selectionList.GetSelectedCount() == 0)
        return;
    const int focusedItem = _selectionList.GetFocusedItem();
    if (focusedItem >= 0) {
        if (_selectionList.IsItemSelected(focusedItem)) {
            const unsigned realIndex = GetRealItemIndex(focusedItem);
            if (realIndex != kParentIndex)
                indices.Add(realIndex);
        }
    }
}

void CPanel::SaveSelectedState(CSelectedState &s) {
    // Qt selection notifications do not yet use the upstream OnNotify body.
    // Synchronize its status vector at this presentation boundary.
    const unsigned numItems = GetFolderItemCount();
    _selectedStatusVector.ClearAndReserve(numItems);
    for (unsigned i = 0; i < numItems; i++)
        _selectedStatusVector.AddInReserved(false);
    for (int row = 0; row < _selectionList.GetItemCount(); ++row) {
        const unsigned realIndex = GetRealItemIndex(row);
        if (realIndex == kParentIndex)
            continue;
        if (realIndex >= numItems)
            throw 1;
        _selectedStatusVector[realIndex] = _selectionList.IsItemSelected(row);
    }

    s.FocusedName_Defined = false;
    s.FocusedName.Empty();
    s.SelectFocused = true;
    s.SelectedNames.Clear();
    s.FocusedItem = _selectionList.GetFocusedItem();
    {
        if (s.FocusedItem >= 0) {
            const unsigned realIndex = GetRealItemIndex(s.FocusedItem);
            if (realIndex != kParentIndex) {
                s.FocusedName = GetItemRelPath(realIndex);
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
    CDisableTimerProcessing timerProcessing(*this);
    CDisableNotify disableNotify(*this);
    int focusedPos = state.FocusedItem;
    if (focusedPos < 0)
        focusedPos = 0;
    _selectionList.ClearSelection();
    const UInt32 numItems = (UInt32)_selectionList.GetItemCount();
    const UInt32 numFolderItems = GetFolderItemCount();
    _selectedStatusVector.ClearAndReserve(numFolderItems);
    for (UInt32 i = 0; i < numFolderItems; ++i)
        _selectedStatusVector.AddInReserved(false);
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
        const unsigned realIndex = GetRealItemIndex(int(i));
        if (realIndex == kParentIndex)
            continue;
        if (realIndex >= numFolderItems)
            throw 1;
        const UString name = _folder ? GetItemName(realIndex) : _selectionList.GetItemName(i);
        bool selected = false;
        if (state.SelectedNames.Size() > 1 ||
            (state.SelectedNames.Size() == 1 && selectedName == name) ||
            (state.FocusedName_Defined && focusedNamePure == name) ||
            (needCheckSlash && (name.Find(L'/') >= 0 || name.Find(L'\\') >= 0))) {
            relPath = GetItemRelPath(realIndex);
            if (relPath == state.FocusedName)
                cursorIndex = (int)i;
            if (state.SelectedNames.FindInSorted(relPath) != -1)
                selected = true;
        }
        _selectedStatusVector[realIndex] = selected;
        if (selected)
            _selectionList.SelectItem((int)i);
    }
    if (_selectionList.GetItemCount() > 0 && cursorIndex >= 0)
        SetFocusedSelectedItem(cursorIndex, state.SelectFocused);
    // Qt rows are sorted separately; item params preserve source folder indices.
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
    NWindows::NCOM::CPropVariant prop;
    if (_folder->GetProperty(itemIndex, propID, &prop) != S_OK)
        throw 2723400;
    UInt64 val = 0;
    if (ConvertPropVariantToUInt64(prop, val))
        return val;
    return 0;
}

UInt64 CPanel::GetItemSize(UInt32 itemIndex) const {
    if (itemIndex == kParentIndex)
        return 0;
    if (_folderGetItemName)
        return _folderGetItemName->GetItemSize(itemIndex);
    return GetItem_UInt64Prop(itemIndex, kpidSize);
}

bool CPanel::GetItem_BoolProp(UInt32 itemIndex, PROPID propID) const {
    NWindows::NCOM::CPropVariant prop;
    if (_folder->GetProperty(itemIndex, propID, &prop) != S_OK)
        throw 2723400;
    if (prop.vt == VT_BOOL)
        return VARIANT_BOOLToBool(prop.boolVal);
    if (prop.vt == VT_EMPTY)
        return false;
    throw 2723401;
}

bool CPanel::IsItem_Folder(UInt32 itemIndex) const {
    if (itemIndex == kParentIndex)
        return true;
    if (itemIndex < _isDirVector.Size())
        return _isDirVector[itemIndex];
    return GetItem_BoolProp(itemIndex, kpidIsDir);
}

UString CPanel::GetItemName(unsigned itemIndex) const {
    if (itemIndex == kParentIndex)
        return L"..";
    NWindows::NCOM::CPropVariant prop;
    if (_folder->GetProperty(itemIndex, kpidName, &prop) != S_OK)
        throw 2723400;
    if (prop.vt != VT_BSTR)
        throw 2723401;
    return prop.bstrVal;
}

UString CPanel::GetItemPrefix(unsigned itemIndex) const {
    UString prefix;
    if (itemIndex != kParentIndex) {
        NWindows::NCOM::CPropVariant prop;
        if (_folder->GetProperty(itemIndex, kpidPrefix, &prop) != S_OK)
            throw 2723400;
        if (prop.vt == VT_BSTR)
            prefix.SetFromBstr(prop.bstrVal);
    }
    return prefix;
}

UString CPanel::GetItemRelPath(unsigned itemIndex) const {
    return GetItemPrefix(itemIndex) + GetItemName(itemIndex);
}

UString CPanel::GetItemRelPath2(unsigned itemIndex) const {
    UString s = GetItemRelPath(itemIndex);
#if defined(_WIN32) && !defined(UNDER_CE)
    if (s.Len() == 2 && NWindows::NFile::NName::IsDrivePath2(s)) {
        if (IsFSDrivesFolder() && !IsDeviceDrivesPrefix())
            s.Add_PathSepar();
    }
#endif
    return s;
}

UString CPanel::GetItemFullPath(unsigned itemIndex) const {
    return GetFsPath() + GetItemRelPath2(itemIndex);
}
