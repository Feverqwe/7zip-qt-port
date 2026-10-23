// Property-name projection adapted from 7-Zip Console/List.cpp,
// 9128b80e3a471108678e2b3ec3c985861c8d7b0f (Igor Pavlov, LGPL-2.1-or-later).
// Console drivers/tabular output are omitted; Qt consumes native properties.
#include "List.h"
#include "7zip/UI/Common/DirItem.h"
#include "7zip/UI/Common/PropIDUtils.h"
#include "Windows/PropVariantConv.h"

static const char *const kPropIdToName[] = {"0",
                                            "1",
                                            "2",
                                            "Path",
                                            "Name",
                                            "Extension",
                                            "Folder",
                                            "Size",
                                            "Packed Size",
                                            "Attributes",
                                            "Created",
                                            "Accessed",
                                            "Modified",
                                            "Solid",
                                            "Commented",
                                            "Encrypted",
                                            "Split Before",
                                            "Split After",
                                            "Dictionary Size",
                                            "CRC",
                                            "Type",
                                            "Anti",
                                            "Method",
                                            "Host OS",
                                            "File System",
                                            "User",
                                            "Group",
                                            "Block",
                                            "Comment",
                                            "Position",
                                            "Path Prefix",
                                            "Folders",
                                            "Files",
                                            "Version",
                                            "Volume",
                                            "Multivolume",
                                            "Offset",
                                            "Links",
                                            "Blocks",
                                            "Volumes",
                                            "Time Type",
                                            "64-bit",
                                            "Big-endian",
                                            "CPU",
                                            "Physical Size",
                                            "Headers Size",
                                            "Checksum",
                                            "Characteristics",
                                            "Virtual Address",
                                            "ID",
                                            "Short Name",
                                            "Creator Application",
                                            "Sector Size",
                                            "Mode",
                                            "Symbolic Link",
                                            "Error",
                                            "Total Size",
                                            "Free Space",
                                            "Cluster Size",
                                            "Label",
                                            "Local Name",
                                            "Provider",
                                            "NT Security",
                                            "Alternate Stream",
                                            "Aux",
                                            "Deleted",
                                            "Tree",
                                            "SHA-1",
                                            "SHA-256",
                                            "Error Type",
                                            "Errors",
                                            "Errors",
                                            "Warnings",
                                            "Warning",
                                            "Streams",
                                            "Alternate Streams",
                                            "Alternate Streams Size",
                                            "Virtual Size",
                                            "Unpack Size",
                                            "Total Physical Size",
                                            "Volume Index",
                                            "SubType",
                                            "Short Comment",
                                            "Code Page",
                                            "Is not archive type",
                                            "Physical Size can't be detected",
                                            "Zeros Tail Is Allowed",
                                            "Tail Size",
                                            "Embedded Stub Size",
                                            "Link",
                                            "Hard Link",
                                            "iNode",
                                            "Stream ID",
                                            "Read-only",
                                            "Out Name",
                                            "Copy Link",
                                            "ArcFileName",
                                            "IsHash",
                                            "Metadata Changed",
                                            "User ID",
                                            "Group ID",
                                            "Device Major",
                                            "Device Minor",
                                            "Dev Major",
                                            "Dev Minor"};

static void GetPropName(PROPID propID, const wchar_t *name, AString &nameA, UString &nameU) {
    if (propID < Z7_ARRAY_SIZE(kPropIdToName)) {
        nameA = kPropIdToName[propID];
        return;
    }
    if (name)
        nameU = name;
    else {
        nameA.Empty();
        nameA.Add_UInt32(propID);
    }
}

UString GetListingPropertyName(PROPID id, const wchar_t *name) {
    AString ascii;
    UString unicode;
    GetPropName(id, name, ascii, unicode);
    if (!ascii.IsEmpty())
        unicode = ascii;
    return unicode;
}
UString GetListingPropertyValue(PROPID id, const PROPVARIANT &value) {
    UString result;
    ConvertPropertyToString2(result, value, id, 9);
    return result;
}
UString GetListingItemPropertyValue(PROPID id, const PROPVARIANT &value, bool isDir) {
    UString result;
    if (value.vt == VT_FILETIME) {
        // CFieldPrinter::PrintTime in technical mode, retaining source precision.
        CArcTime time;
        time.Set_From_Prop(value);
        if (time.IsZero())
            return result;
        int precision = kTimestampPrintLevel_NTFS;
        if (time.Prec != 0) {
            precision = time.GetNumDigits();
            if (precision < kTimestampPrintLevel_DAY)
                precision = kTimestampPrintLevel_NTFS;
        }
        char text[64];
        ConvertUtcFileTimeToString2(time.FT, time.Ns100, text, precision);
        result = text;
        return result;
    }
    // PrintItemInfo -> GetAttribString, in technical listing mode.
    if (id == kpidAttrib && (value.vt == VT_EMPTY || value.vt == VT_UI4)) {
        char text[64];
        UInt32 attributes = value.vt == VT_EMPTY ? 0 : value.ulVal;
        if (isDir)
            attributes |= FILE_ATTRIBUTE_DIRECTORY;
        ConvertWinAttribToString(text, attributes);
        result = text;
        return result;
    }
    return GetListingPropertyValue(id, value);
}
