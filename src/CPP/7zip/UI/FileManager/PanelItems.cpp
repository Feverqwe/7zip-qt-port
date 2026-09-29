// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelItems.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// IFolder/variant property bodies retain upstream control flow. Qt list rows
// remain a presentation snapshot pending the full folder-load port.
#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resource.h"
#include "Panel.h"
#include "PropertyName.h"
#include "ViewSettings.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "Windows/WinDefs.h"

static const UInt32 kParentIndex = (UInt32)-1;

static bool GetColumnVisible(PROPID propID, bool isFsFolder) {
    if (isFsFolder) {
        switch (propID) {
        case kpidATime:
        case kpidChangeTime:
        case kpidAttrib:
        case kpidPackSize:
        case kpidINode:
        case kpidLinks:
        case kpidNtReparse:
            return false;
        }
    }
    return true;
}

static unsigned GetColumnWidth(PROPID propID, VARTYPE /* varType */) {
    switch (propID) {
    case kpidName:
        return 160;
    }
    return 100;
}

Qt::Alignment GetColumnAlign(PROPID propID, VARTYPE varType) {
    switch (propID) {
    case kpidCTime:
    case kpidATime:
    case kpidMTime:
    case kpidChangeTime:
        return Qt::AlignLeft | Qt::AlignVCenter;
    }
    switch (varType) {
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_INT:
    case VT_UI4:
    case VT_UINT:
    case VT_I8:
    case VT_UI8:
    case VT_BOOL:
        return Qt::AlignRight | Qt::AlignVCenter;
    case VT_EMPTY:
    case VT_I1:
    case VT_FILETIME:
    case VT_BSTR:
        return Qt::AlignLeft | Qt::AlignVCenter;
    default:
        return Qt::AlignHCenter | Qt::AlignVCenter;
    }
}

static int ItemProperty_Compare_NameFirst(void *const *a1, void *const *a2, void * /* param */) {
    return (*(*((const CPropColumn *const *)a1)))
        .Compare_NameFirst(*(*((const CPropColumn *const *)a2)));
}

HRESULT CPanel::InitColumns() {
    SaveListViewInfo(_visibleColumns);
    if (!_folder)
        return E_FAIL;
    const UString oldType = _typeIDString;
    _typeIDString = GetFolderTypeID();
    if (!_needSaveInfo || _typeIDString != oldType)
        _listViewInfo.Read(_typeIDString, _settingsPath);
    _ascending = _listViewInfo.Ascending;
    _columns.Clear();
    const bool isFsFolder = IsFSFolder() || IsAltStreamsFolder();
    UInt32 numProps = 0;
    if (_folder->GetNumberOfProperties(&numProps) == S_OK) {
        for (UInt32 i = 0; i < numProps; i++) {
            CMyComBSTR name;
            PROPID propID;
            VARTYPE varType;
            const HRESULT res = _folder->GetPropertyInfo(i, &name, &propID, &varType);
            if (res != S_OK || propID == kpidIsDir)
                continue;
            CPropColumn prop;
            prop.Type = varType;
            prop.ID = propID;
            prop.Name = GetNameOfProperty(propID, name);
            prop.Order = -1;
            prop.IsVisible = GetColumnVisible(propID, isFsFolder);
            prop.Width = GetColumnWidth(propID, varType);
            prop.IsRawProp = false;
            _columns.Add(prop);
        }
    }
    if (_folderRawProps) {
        UInt32 numRawProps = 0;
        if (_folderRawProps->GetNumRawProps(&numRawProps) == S_OK) {
            for (UInt32 i = 0; i < numRawProps; i++) {
                CMyComBSTR name;
                PROPID propID;
                if (_folderRawProps->GetRawPropInfo(i, &name, &propID) != S_OK)
                    continue;
                CPropColumn prop;
                prop.Type = VT_EMPTY;
                prop.ID = propID;
                prop.Name = GetNameOfProperty(propID, name);
                prop.Order = -1;
                prop.IsVisible = GetColumnVisible(propID, isFsFolder);
                prop.Width = GetColumnWidth(propID, VT_BSTR);
                prop.IsRawProp = true;
                _columns.Add(prop);
            }
        }
    }
    unsigned order = 0;
    unsigned i;
    for (i = 0; i < _listViewInfo.Columns.Size(); i++) {
        const CColumnInfo &columnInfo = _listViewInfo.Columns[i];
        const int index = _columns.FindItem_for_PropID(columnInfo.PropID);
        if (index >= 0) {
            CPropColumn &item = _columns[index];
            if (item.Order >= 0)
                continue;
            bool isVisible = columnInfo.IsVisible;
            if (columnInfo.PropID == kpidName)
                isVisible = true;
            item.IsVisible = isVisible;
            item.Width = columnInfo.Width;
            if (isVisible)
                item.Order = (int)(order++);
        }
    }
    for (i = 0; i < _columns.Size(); i++) {
        CPropColumn &item = _columns[i];
        if (item.IsVisible && item.Order < 0)
            item.Order = (int)(order++);
    }
    for (i = 0; i < _columns.Size(); i++) {
        CPropColumn &item = _columns[i];
        if (item.Order < 0)
            item.Order = (int)(order++);
    }
    if (_listViewInfo.IsLoaded)
        _sortID = _listViewInfo.SortID;
    else {
        _sortID = 0;
        if (IsFSFolder() || IsAltStreamsFolder() || IsArcFolder())
            _sortID = kpidName;
    }
    CPropColumns newColumns;
    for (i = 0; i < _columns.Size(); i++) {
        const CPropColumn &prop = _columns[i];
        if (prop.IsVisible)
            newColumns.Add(prop);
    }
    newColumns.Sort(ItemProperty_Compare_NameFirst, NULL);
    _visibleColumns.Clear();
    FOR_VECTOR(columnIndex, newColumns)
    _visibleColumns.Add(newColumns[columnIndex]);
    _needSaveInfo = true;
    return S_OK;
}

void CPanel::SaveListViewInfo(const CPropColumns &visibleColumns) {
    if (!_needSaveInfo)
        return;

    _visibleColumns = visibleColumns;
    FOR_VECTOR(i, _columns) {
        CPropColumn &prop = _columns[i];
        const int index = _visibleColumns.FindItem_for_PropID(prop.ID);
        prop.IsVisible = index >= 0;
        if (index >= 0) {
            prop.Order = _visibleColumns[index].Order;
            prop.Width = _visibleColumns[index].Width;
        }
    }

    CListViewInfo viewInfo;
    const PROPID sortPropID = _sortID;
    CPropColumns sortedProperties = _visibleColumns;
    sortedProperties.Sort();
    FOR_VECTOR(i, sortedProperties) {
        const CPropColumn &prop = sortedProperties[i];
        CColumnInfo columnInfo;
        columnInfo.IsVisible = prop.IsVisible;
        columnInfo.PropID = prop.ID;
        columnInfo.Width = prop.Width;
        viewInfo.Columns.Add(columnInfo);
    }
    FOR_VECTOR(i, _columns) {
        const CPropColumn &prop = _columns[i];
        if (sortedProperties.FindItem_for_PropID(prop.ID) < 0) {
            CColumnInfo columnInfo;
            columnInfo.IsVisible = false;
            columnInfo.PropID = prop.ID;
            columnInfo.Width = prop.Width;
            viewInfo.Columns.Add(columnInfo);
        }
    }
    viewInfo.SortID = sortPropID;
    viewInfo.Ascending = _ascending;
    viewInfo.IsLoaded = true;
    if (!_listViewInfo.IsEqual(viewInfo)) {
        viewInfo.Save(_typeIDString, _settingsPath);
        _listViewInfo = viewInfo;
    }
}

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
    if (_selectionList.GetItemCount() == 0)
        _refreshSelection = state;
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

void CPanel::RefreshListCtrl() {
    CSelectedState state;
    RefreshListCtrl(state);
}

HRESULT CPanel::RefreshListCtrl_SaveFocused(bool onTimer) {
    CSelectedState state;
    SaveSelectedState(state);
    state.CalledFromTimer = onTimer;
    return _reloadPresentation ? _reloadPresentation(state) : E_NOTIMPL;
}

void CPanel::OnReload(bool onTimer) {
    const HRESULT result = RefreshListCtrl_SaveFocused(onTimer);
    if (result != S_OK)
        MessageBox_Error_HRESULT(result);
}

void CPanel::OnTimer() {
    if (!_processTimer)
        return;
    if (!AutoRefresh_Mode)
        return;
    if (!_folder)
        return;
    CMyComPtr<IFolderWasChanged> folderWasChanged;
    _folder.QueryInterface(IID_IFolderWasChanged, &folderWasChanged);
    if (!folderWasChanged)
        return;
    Int32 wasChanged = 0;
    if (folderWasChanged->WasChanged(&wasChanged) != S_OK)
        return;
    if (wasChanged == 0 && !_portableFolderChanged)
        return;
    _portableFolderChanged = false;
    OnReload(true);
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

bool CPanel::IsItem_AltStream(unsigned itemIndex) const {
    if (itemIndex == kParentIndex)
        return false;
    return GetItem_BoolProp(itemIndex, kpidIsAltStream);
}

void CPanel::OpenFocusedItemAsInternal(const wchar_t *type) {
    const int focusedItem = _selectionList.GetFocusedItem();
    if (focusedItem < 0)
        return;
    const unsigned realIndex = GetRealItemIndex(focusedItem);
    if (IsItem_Folder(realIndex))
        OpenFolder(realIndex);
    else
        OpenItem(realIndex, true, false, type);
}

void CPanel::OpenSelectedItems(bool tryInternal) {
    CRecordVector<UInt32> indices;
    Get_ItemIndices_Operated(indices);
    if (indices.Size() > 20) {
        MessageBox_Error_LangID(IDS_TOO_MANY_ITEMS);
        return;
    }
    const int focusedItem = _selectionList.GetFocusedItem();
    if (focusedItem >= 0) {
        const unsigned realIndex = GetRealItemIndex(focusedItem);
        if (realIndex == kParentIndex && (tryInternal || indices.Size() == 0) &&
            _selectionList.IsItemSelected(focusedItem))
            indices.Insert(0, realIndex);
    }
    FOR_VECTOR(i, indices) {
        UInt32 index = indices[i];
        if (IsItem_Folder(index)) {
            if (tryInternal) {
                OpenFolder(index);
                break;
            } else
                OpenFolderExternal(index);
        } else
            OpenItem(index, (tryInternal && indices.Size() == 1), true);
    }
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
