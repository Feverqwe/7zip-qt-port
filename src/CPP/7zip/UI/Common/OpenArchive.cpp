// Adapted from 7-Zip CPP/7zip/UI/Common/OpenArchive.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source error flag and item-property/default-name bodies. Full codec opening
// implementations remain mandatory; see the port audit for the compiled scope.
#include "OpenArchive.h"
#include "../../../../../third_party/7zip/C/CpuArch.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/LimitedStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/PropID.h"
#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include "CPP/Common/Wildcard.h"
#include "CPP/Windows/FileDir.h"
#include "Common/UTFConvert.h"

using namespace NWindows;
static const UInt64 kMaxCheckStartPosition = 1 << 23;

HRESULT CArc::ReOpen(const COpenOptions &op, IArchiveOpenCallback *openCallback_Additional) {
    ErrorInfo.ClearErrors();
    ErrorInfo.ErrorFormatIndex = -1;

    UInt64 fileSize = 0;
    if (op.stream) {
        RINOK(InStream_SeekToBegin(op.stream))
        RINOK(InStream_AtBegin_GetSize(op.stream, fileSize))
        // RINOK(InStream_GetSize_SeekToBegin(op.stream, fileSize))
    }
    FileSize = fileSize;

    CMyComPtr<IInStream> stream2;
    Int64 globalOffset = GetGlobalOffset();
    if (globalOffset <= 0)
        stream2 = op.stream;
    else {
        CTailInStream *tailStreamSpec = new CTailInStream;
        stream2 = tailStreamSpec;
        tailStreamSpec->Stream = op.stream;
        tailStreamSpec->Offset = (UInt64)globalOffset;
        tailStreamSpec->Init();
        RINOK(tailStreamSpec->SeekToStart())
    }

    // There are archives with embedded STUBs (like ZIP), so we must support signature scanning
    // But for another archives we can use 0 here. So the code can be fixed !!!
    UInt64 maxStartPosition = kMaxCheckStartPosition;
    IArchiveOpenCallback *openCallback = openCallback_Additional;
    if (!openCallback)
        openCallback = op.callback;
    HRESULT res = Archive->Open(stream2, &maxStartPosition, openCallback);

    if (res == S_OK) {
        RINOK(ReadBasicProps(Archive, (UInt64)globalOffset, res))
        ArcStreamOffset = (UInt64)globalOffset;
        if (ArcStreamOffset != 0)
            InStream = op.stream;
    }
    return res;
}

HRESULT CArchiveLink::ReOpen(COpenOptions &op) {
    if (Arcs.Size() > 1)
        return E_NOTIMPL;

    CObjectVector<COpenType> inc;
    CIntVector excl;

    op.types = &inc;
    op.excludedFormats = &excl;
    op.stdInMode = false;
    op.stream = NULL;
    if (Arcs.Size() == 0) // ???
        return E_NOTIMPL; // Empty-owner Open2 codec path remains mandatory.

    /* if archive is multivolume (unsupported here still)
       COpenCallbackImp object will exist after Open stage. */
    COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
    CMyComPtr<IArchiveOpenCallback> openCallbackNew = openCallbackSpec;

    openCallbackSpec->Callback = NULL;
    openCallbackSpec->ReOpenCallback = op.callback;
    {
        FString dirPrefix, fileName;
        NFile::NDir::GetFullPathAndSplit(us2fs(op.filePath), dirPrefix, fileName);
        RINOK(openCallbackSpec->Init2(dirPrefix, fileName))
    }

    CInFileStream *fileStreamSpec = new CInFileStream;
    CMyComPtr<IInStream> stream(fileStreamSpec);
    if (!fileStreamSpec->Open(us2fs(op.filePath)))
        return GetLastError_noZero_HRESULT();
    op.stream = stream;

    CArc &arc = Arcs[0];
    const HRESULT res = arc.ReOpen(op, openCallbackNew);

    openCallbackSpec->ReOpenCallback = NULL;

    PasswordWasAsked = openCallbackSpec->PasswordWasAsked;
    // Password = openCallbackSpec->Password;

    IsOpen = (res == S_OK);
    return res;
}

static HRESULT Archive_GetArcProp_UInt(IInArchive *arc, PROPID propid, UInt64 &result,
                                       bool &defined) {
    defined = false;
    NCOM::CPropVariant prop;
    RINOK(arc->GetArchiveProperty(propid, &prop))
    switch (prop.vt) {
    case VT_UI4:
        result = prop.ulVal;
        break;
    case VT_I4:
        result = (UInt64)(Int64)prop.lVal;
        break;
    case VT_UI8:
        result = (UInt64)prop.uhVal.QuadPart;
        break;
    case VT_I8:
        result = (UInt64)prop.hVal.QuadPart;
        break;
    case VT_EMPTY:
        return S_OK;
    default:
        return E_FAIL;
    }
    defined = true;
    return S_OK;
}

static HRESULT Archive_GetArcProp_Int(IInArchive *arc, PROPID propid, Int64 &result,
                                      bool &defined) {
    defined = false;
    NCOM::CPropVariant prop;
    RINOK(arc->GetArchiveProperty(propid, &prop))
    switch (prop.vt) {
    case VT_UI4:
        result = prop.ulVal;
        break;
    case VT_I4:
        result = prop.lVal;
        break;
    case VT_UI8:
        result = (Int64)prop.uhVal.QuadPart;
        break;
    case VT_I8:
        result = (Int64)prop.hVal.QuadPart;
        break;
    case VT_EMPTY:
        return S_OK;
    default:
        return E_FAIL;
    }
    defined = true;
    return S_OK;
}

HRESULT CArc::ReadBasicProps(IInArchive *archive, UInt64 startPos, HRESULT openRes) {
    // OkPhySize_Defined = false;
    PhySize_Defined = false;
    PhySize = 0;
    Offset = 0;
    AvailPhySize = FileSize - startPos;

    ErrorInfo.ClearErrors();
    {
        NCOM::CPropVariant prop;
        RINOK(archive->GetArchiveProperty(kpidErrorFlags, &prop))
        ErrorInfo.ErrorFlags = GetOpenArcErrorFlags(prop, &ErrorInfo.ErrorFlags_Defined);
    }
    {
        NCOM::CPropVariant prop;
        RINOK(archive->GetArchiveProperty(kpidWarningFlags, &prop))
        ErrorInfo.WarningFlags = GetOpenArcErrorFlags(prop);
    }

    {
        NCOM::CPropVariant prop;
        RINOK(archive->GetArchiveProperty(kpidError, &prop))
        if (prop.vt != VT_EMPTY)
            ErrorInfo.ErrorMessage = (prop.vt == VT_BSTR ? prop.bstrVal : L"Unknown error");
    }

    {
        NCOM::CPropVariant prop;
        RINOK(archive->GetArchiveProperty(kpidWarning, &prop))
        if (prop.vt != VT_EMPTY)
            ErrorInfo.WarningMessage = (prop.vt == VT_BSTR ? prop.bstrVal : L"Unknown warning");
    }

    if (openRes == S_OK || ErrorInfo.IsArc_After_NonOpen()) {
        RINOK(Archive_GetArcProp_UInt(archive, kpidPhySize, PhySize, PhySize_Defined))
        /*
        RINOK(Archive_GetArcProp_UInt(archive, kpidOkPhySize, OkPhySize, OkPhySize_Defined));
        if (!OkPhySize_Defined)
        {
          OkPhySize_Defined = PhySize_Defined;
          OkPhySize = PhySize;
        }
        */

        bool offsetDefined;
        RINOK(Archive_GetArcProp_Int(archive, kpidOffset, Offset, offsetDefined))

        Int64 globalOffset = (Int64)startPos + Offset;
        AvailPhySize = (UInt64)((Int64)FileSize - globalOffset);
        if (PhySize_Defined) {
            UInt64 endPos = (UInt64)(globalOffset + (Int64)PhySize);
            if (endPos < FileSize) {
                AvailPhySize = PhySize;
                ErrorInfo.ThereIsTail = true;
                ErrorInfo.TailSize = FileSize - endPos;
            } else if (endPos > FileSize)
                ErrorInfo.UnexpecedEnd = true;
        }
    }

    return S_OK;
}

void CArcErrorInfo::ClearErrors() {
    // ErrorFormatIndex = -1; // we don't need to clear ErrorFormatIndex here !!!
    ThereIsTail = false;
    UnexpecedEnd = false;
    IgnoreTail = false;
    // NonZerosTail = false;
    ErrorFlags_Defined = false;
    ErrorFlags = 0;
    WarningFlags = 0;
    TailSize = 0;
    ErrorMessage.Empty();
    WarningMessage.Empty();
}

void CArchiveLink::KeepModeForNextOpen() {
    for (unsigned i = Arcs.Size(); i != 0;) {
        i--;
        CMyComPtr<IArchiveKeepModeForNextOpen> keep;
        Arcs[i].Archive->QueryInterface(IID_IArchiveKeepModeForNextOpen, (void **)&keep);
        if (keep)
            keep->KeepModeForNextOpen();
    }
}

HRESULT CArchiveLink::Close() {
    for (unsigned i = Arcs.Size(); i != 0;) {
        i--;
        RINOK(Arcs[i].Close())
    }
    IsOpen = false;
    // ErrorsText.Empty();
    return S_OK;
}

void CArchiveLink::Release() {
    // NonOpenErrorFormatIndex = -1;
    NonOpen_ErrorInfo.ClearErrors();
    NonOpen_ArcPath.Empty();
    while (!Arcs.IsEmpty())
        Arcs.DeleteBack();
}

HRESULT CArchiveLink::Open2(COpenOptions &op, IOpenCallbackUI *callbackUI) {
    return op.QtOpenService ? op.QtOpenService->Open(*this, op, callbackUI) : E_NOTIMPL;
}

HRESULT CArchiveLink::Open3(COpenOptions &op, IOpenCallbackUI *callbackUI) {
    HRESULT res = Open2(op, callbackUI);
    if (callbackUI) {
        RINOK(callbackUI->Open_Finished())
    }
    return res;
}

HRESULT Archive_GetItemBoolProp(IInArchive *arc, UInt32 index, PROPID propID,
                                bool &result) throw() {
    NCOM::CPropVariant prop;
    result = false;
    RINOK(arc->GetProperty(index, propID, &prop))
    if (prop.vt == VT_BOOL)
        result = VARIANT_BOOLToBool(prop.boolVal);
    else if (prop.vt != VT_EMPTY)
        return E_FAIL;
    return S_OK;
}

HRESULT Archive_IsItem_Dir(IInArchive *arc, UInt32 index, bool &result) throw() {
    return Archive_GetItemBoolProp(arc, index, kpidIsDir, result);
}

HRESULT Archive_IsItem_Aux(IInArchive *arc, UInt32 index, bool &result) throw() {
    return Archive_GetItemBoolProp(arc, index, kpidIsAux, result);
}

HRESULT Archive_IsItem_AltStream(IInArchive *arc, UInt32 index, bool &result) throw() {
    return Archive_GetItemBoolProp(arc, index, kpidIsAltStream, result);
}

HRESULT Archive_IsItem_Deleted(IInArchive *arc, UInt32 index, bool &result) throw() {
    return Archive_GetItemBoolProp(arc, index, kpidIsDeleted, result);
}

#ifndef Z7_SFX

HRESULT CArc::GetItem_PathToParent(UInt32 index, const UInt32 parent, UStringVector &parts) const {
    if (!GetRawProps)
        return E_FAIL;
    if (index == parent)
        return S_OK;

    CRecordVector<UInt32> vec;
    UString s;
    bool isAltStream = false;

    for (;;) {
        vec.Add(index);
        UInt32 curParent = (UInt32)(Int32)-1;
        UInt32 parentType = 0;
        RINOK(GetRawProps->GetParent(index, &curParent, &parentType))

        if (parentType == NParentType::kAltStream) {
            // that case is not expected because
            // we don't call GetItem_PathToParent() function for alt stream items
            if (vec.Size() != 1)
                return E_FAIL;
            isAltStream = true;
        }
        if (parent == curParent)
            break;
        if (curParent == (UInt32)(Int32)-1)
            return E_FAIL;
        index = curParent;
    }

    unsigned k = vec.Size();
    do {
        index = vec[--k];
#ifdef MY_CPU_LE
        const void *p;
        UInt32 size;
        UInt32 propType;
        RINOK(GetRawProps->GetRawProp(index, kpidName, &p, &size, &propType))
        if (p && propType == PROP_DATA_TYPE_wchar_t_PTR_Z_LE)
            s = (const wchar_t *)p;
        else
#endif
        {
            NCOM::CPropVariant prop;
            RINOK(Archive->GetProperty(index, kpidName, &prop))
            if (prop.vt == VT_BSTR && prop.bstrVal)
                s.SetFromBstr(prop.bstrVal);
            else if (prop.vt == VT_EMPTY)
                s.Empty();
            else
                return E_FAIL;
        }
        if (isAltStream && k == 0 && !parts.IsEmpty()) {
            UString &s2 = parts.Back();
            s2.Add_Colon();
            s2 += s;
        } else
            parts.Add(s);
    } while (k);
    return S_OK;
}

#endif

HRESULT CArc::GetItem_Path(UInt32 index, UString &result) const {
#ifdef MY_CPU_LE
    if (GetRawProps) {
        const void *p;
        UInt32 size;
        UInt32 propType;
        if (!IsTree) {
            if (GetRawProps->GetRawProp(index, kpidPath, &p, &size, &propType) == S_OK &&
                propType == NPropDataType::kUtf16z) {
                unsigned len = size / 2 - 1;
                // (len) doesn't include null terminator

                /*
                #if WCHAR_MAX > 0xffff
                len = (unsigned)Utf16LE__Get_Num_WCHARs(p, len);

                wchar_t *s = result.GetBuf(len);
                wchar_t *sEnd = Utf16LE__To_WCHARs_Sep(p, len, s);
                if (s + len != sEnd) return E_FAIL;
                *sEnd = 0;

                #else
                */

                wchar_t *s = result.GetBuf(len);
                for (unsigned i = 0; i < len; i++) {
                    wchar_t c = GetUi16(p);
                    p = (const void *)((const Byte *)p + 2);

#if WCHAR_PATH_SEPARATOR != L'/'
                    if (c == L'/')
                        c = WCHAR_PATH_SEPARATOR;
                    else if (c == L'\\')
                        c = WCHAR_IN_FILE_NAME_BACKSLASH_REPLACEMENT; // WSL scheme
#endif

                    *s++ = c;
                }
                *s = 0;

                // #endif

                result.ReleaseBuf_SetLen(len);

                Convert_UnicodeEsc16_To_UnicodeEscHigh(result);
                if (len != 0)
                    return S_OK;
            }
        }
        /*
        else if (GetRawProps->GetRawProp(index, kpidName, &p, &size, &propType) == S_OK &&
            p && propType == NPropDataType::kUtf16z)
        {
          size -= 2;
          UInt32 totalSize = size;
          bool isOK = false;

          {
            UInt32 index2 = index;
            for (;;)
            {
              UInt32 parent = (UInt32)(Int32)-1;
              UInt32 parentType = 0;
              if (GetRawProps->GetParent(index2, &parent, &parentType) != S_OK)
                break;
              if (parent == (UInt32)(Int32)-1)
              {
                if (parentType != 0)
                  totalSize += 2;
                isOK = true;
                break;
              }
              index2 = parent;
              UInt32 size2;
              const void *p2;
              if (GetRawProps->GetRawProp(index2, kpidName, &p2, &size2, &propType) != S_OK &&
                  p2 && propType == NPropDataType::kUtf16z)
                break;
              totalSize += size2;
            }
          }

          if (isOK)
          {
            wchar_t *sz = result.GetBuf_SetEnd(totalSize / 2);
            UInt32 pos = totalSize - size;
            memcpy((Byte *)sz + pos, p, size);
            UInt32 index2 = index;
            for (;;)
            {
              UInt32 parent = (UInt32)(Int32)-1;
              UInt32 parentType = 0;
              if (GetRawProps->GetParent(index2, &parent, &parentType) != S_OK)
                break;
              if (parent == (UInt32)(Int32)-1)
              {
                if (parentType != 0)
                  sz[pos / 2 - 1] = L':';
                break;
              }
              index2 = parent;
              UInt32 size2;
              const void *p2;
              if (GetRawProps->GetRawProp(index2, kpidName, &p2, &size2, &propType) != S_OK)
                break;
              pos -= size2;
              memcpy((Byte *)sz + pos, p2, size2);
              sz[(pos + size2 - 2) / 2] = (parentType == 0) ? WCHAR_PATH_SEPARATOR : L':';
            }
            #ifdef _WIN32
            // result.Replace(L'/', WCHAR_PATH_SEPARATOR);
            #endif
            return S_OK;
          }
        }
        */
    }
#endif

    {
        NCOM::CPropVariant prop;
        RINOK(Archive->GetProperty(index, kpidPath, &prop))
        if (prop.vt == VT_BSTR && prop.bstrVal)
            result.SetFromBstr(prop.bstrVal);
        else if (prop.vt == VT_EMPTY)
            result.Empty();
        else
            return E_FAIL;
    }

    if (result.IsEmpty())
        return GetItem_DefaultPath(index, result);

    Convert_UnicodeEsc16_To_UnicodeEscHigh(result);
    return S_OK;
}

HRESULT CArc::GetItem_DefaultPath(UInt32 index, UString &result) const {
    result.Empty();
    bool isDir;
    RINOK(Archive_IsItem_Dir(Archive, index, isDir))
    if (!isDir) {
        result = DefaultName;
        NCOM::CPropVariant prop;
        RINOK(Archive->GetProperty(index, kpidExtension, &prop))
        if (prop.vt == VT_BSTR) {
            result.Add_Dot();
            result += prop.bstrVal;
        } else if (prop.vt != VT_EMPTY)
            return E_FAIL;
    }
    return S_OK;
}

HRESULT CArc::GetItem_Path2(UInt32 index, UString &result) const {
    RINOK(GetItem_Path(index, result))
    if (Ask_Deleted) {
        bool isDeleted = false;
        RINOK(Archive_IsItem_Deleted(Archive, index, isDeleted))
        if (isDeleted)
            result.Insert(0, L"[DELETED]" WSTRING_PATH_SEPARATOR);
    }
    return S_OK;
}

#ifdef SUPPORT_ALT_STREAMS

int FindAltStreamColon_in_Path(const wchar_t *path) {
    unsigned i = 0;
    int colonPos = -1;
    for (;; i++) {
        wchar_t c = path[i];
        if (c == 0)
            return colonPos;
        if (c == ':') {
            if (colonPos < 0)
                colonPos = (int)i;
            continue;
        }
        if (c == WCHAR_PATH_SEPARATOR)
            colonPos = -1;
    }
}

#endif

HRESULT CArc::GetItem(UInt32 index, CReadArcItem &item) const {
#ifdef SUPPORT_ALT_STREAMS
    item.IsAltStream = false;
    item.AltStreamName.Empty();
    item.MainPath.Empty();
#endif

    item.IsDir = false;
    item.Path.Empty();
    item.ParentIndex = (UInt32)(Int32)-1;

    item.PathParts.Clear();

    RINOK(Archive_IsItem_Dir(Archive, index, item.IsDir))
    item.MainIsDir = item.IsDir;

    RINOK(GetItem_Path2(index, item.Path))

#ifndef Z7_SFX
    UInt32 mainIndex = index;
#endif

#ifdef SUPPORT_ALT_STREAMS

    item.MainPath = item.Path;
    if (Ask_AltStream) {
        RINOK(Archive_IsItem_AltStream(Archive, index, item.IsAltStream))
    }

    bool needFindAltStream = false;

    if (item.IsAltStream) {
        needFindAltStream = true;
        if (GetRawProps) {
            UInt32 parentType = 0;
            UInt32 parentIndex;
            RINOK(GetRawProps->GetParent(index, &parentIndex, &parentType))
            if (parentType == NParentType::kAltStream) {
                NCOM::CPropVariant prop;
                RINOK(Archive->GetProperty(index, kpidName, &prop))
                if (prop.vt == VT_BSTR && prop.bstrVal)
                    item.AltStreamName.SetFromBstr(prop.bstrVal);
                else if (prop.vt != VT_EMPTY)
                    return E_FAIL;
                else {
                    // item.IsAltStream = false;
                }
                /*
                if (item.AltStreamName.IsEmpty())
                  item.IsAltStream = false;
                */

                needFindAltStream = false;
                item.ParentIndex = parentIndex;
                mainIndex = parentIndex;

                if (parentIndex == (UInt32)(Int32)-1) {
                    item.MainPath.Empty();
                    item.MainIsDir = true;
                } else {
                    RINOK(GetItem_Path2(parentIndex, item.MainPath))
                    RINOK(Archive_IsItem_Dir(Archive, parentIndex, item.MainIsDir))
                }
            }
        }
    }

    if (item.WriteToAltStreamIfColon || needFindAltStream) {
        /* Good handler must support GetRawProps::GetParent for alt streams.
           So the following code currently is not used */
        const int colon = FindAltStreamColon_in_Path(item.Path);
        if (colon >= 0) {
            item.MainPath.DeleteFrom((unsigned)colon);
            item.AltStreamName = item.Path.Ptr((unsigned)(colon + 1));
            item.MainIsDir = (colon == 0 || IsPathSepar(item.Path[(unsigned)colon - 1]));
            item.IsAltStream = true;
        }
    }

#endif

#ifndef Z7_SFX
    if (item._use_baseParentFolder_mode) {
        RINOK(GetItem_PathToParent(mainIndex, (unsigned)item._baseParentFolder, item.PathParts))

#ifdef SUPPORT_ALT_STREAMS
        if ((item.WriteToAltStreamIfColon || needFindAltStream) && !item.PathParts.IsEmpty()) {
            int colon;
            {
                UString &s = item.PathParts.Back();
                colon = FindAltStreamColon_in_Path(s);
                if (colon >= 0) {
                    item.AltStreamName = s.Ptr((unsigned)(colon + 1));
                    item.MainIsDir = (colon == 0 || IsPathSepar(s[(unsigned)colon - 1]));
                    item.IsAltStream = true;
                    s.DeleteFrom((unsigned)colon);
                }
            }
            if (colon == 0)
                item.PathParts.DeleteBack();
        }
#endif

    } else
#endif
        SplitPathToParts(
#ifdef SUPPORT_ALT_STREAMS
            item.MainPath
#else
        item.Path
#endif
            ,
            item.PathParts);

    return S_OK;
}

HRESULT CArc::GetItem_Size(UInt32 index, UInt64 &size, bool &defined) const {
    NCOM::CPropVariant prop;
    defined = false;
    size = 0;
    RINOK(Archive->GetProperty(index, kpidSize, &prop))
    switch (prop.vt) {
    case VT_UI1:
        size = prop.bVal;
        break;
    case VT_UI2:
        size = prop.uiVal;
        break;
    case VT_UI4:
        size = prop.ulVal;
        break;
    case VT_UI8:
        size = (UInt64)prop.uhVal.QuadPart;
        break;
    case VT_EMPTY:
        return S_OK;
    default:
        return E_FAIL;
    }
    defined = true;
    return S_OK;
}

HRESULT CArc::GetItem_MTime(UInt32 index, CArcTime &at) const {
    at.Clear();
    NCOM::CPropVariant prop;
    RINOK(Archive->GetProperty(index, kpidMTime, &prop))

    if (prop.vt == VT_FILETIME) {
        /*
        // for debug
        if (FILETIME_IsZero(prop.at) && MTime.Def)
        {
          at = MTime;
          return S_OK;
        }
        */
        at.Set_From_Prop(prop);
        if (at.Prec == 0) {
            // (at.Prec == 0) before version 22.
            // so kpidTimeType is required for that code
            prop.Clear();
            RINOK(Archive->GetProperty(index, kpidTimeType, &prop))
            if (prop.vt == VT_UI4) {
                UInt32 val = prop.ulVal;
                if (val == NFileTimeType::kWindows)
                    val = k_PropVar_TimePrec_100ns;
                /*
                else if (val > k_PropVar_TimePrec_1ns)
                {
                  val = k_PropVar_TimePrec_100ns;
                  // val = k_PropVar_TimePrec_1ns;
                  // return E_FAIL; // for debug
                }
                */
                at.Prec = (UInt16)val;
            }
        }
        return S_OK;
    }

    if (prop.vt != VT_EMPTY)
        return E_FAIL;
    if (MTime.Def)
        at = MTime;
    return S_OK;
}

UInt32 GetOpenArcErrorFlags(const NCOM::CPropVariant &prop, bool *isDefinedProp) {
    if (isDefinedProp != NULL)
        *isDefinedProp = false;

    switch (prop.vt) {
    case VT_UI8:
        if (isDefinedProp)
            *isDefinedProp = true;
        return (UInt32)prop.uhVal.QuadPart;
    case VT_UI4:
        if (isDefinedProp)
            *isDefinedProp = true;
        return prop.ulVal;
    case VT_EMPTY:
        return 0;
    default:
        throw 151199;
    }
}
