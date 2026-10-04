// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Copied command tables, naming, query and invoke bodies. Win32 menu handles
// and process calls are adapted through presentation/typed launch boundaries.
#include "ContextMenu.h"
#include "../Common/CompressCall.h"
#include "../Common/ExtractingFilePath.h"
#include "../FileManager/FormatUtils.h"
#include "../FileManager/LangUtils.h"
#include "../FileManager/PropertyName.h"
#include "7zip/PropID.h"
#include "7zip/UI/Common/ArchiveName.h"
#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "7zip/UI/Explorer/resource.h"
#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
using namespace NWindows;
using namespace NFile;
using namespace NDir;
using namespace NQtShellMenu;
#define ODS(x)
#define ODS_U(x)
#define ODS_(x)
#define ODS_SPRF_s(x)
static const unsigned kCmfNormal = 0;
static const unsigned kCmfVerbsOnly = 2;
static const unsigned kCmfExplore = 4;
#define Z7_WIN_CMF_EXTENDEDVERBS 0x00000100
// Email dispatch stays in the typed service; platforms may refuse it explicitly.
#define EMAIL_SUPPORT 1
static LPCSTR const kMainVerb = "SevenZip";
static LPCSTR const kOpenCascadedVerb = "SevenZip.OpenWithType.";
static LPCSTR const kCheckSumCascadedVerb = "SevenZip.Checksum";

struct CContextMenuCommand {
    UInt32 flag;
    CZipContextMenu::enum_CommandInternalID CommandInternalID;
    LPCSTR Verb;
    UINT ResourceID;
};

#define CMD_REC(cns, verb, ids) {NContextMenuFlags::cns, CZipContextMenu::cns, verb, ids}

static const CContextMenuCommand g_Commands[] = {
    CMD_REC(kOpen, "Open", IDS_CONTEXT_OPEN),
    CMD_REC(kExtract, "Extract", IDS_CONTEXT_EXTRACT),
    CMD_REC(kExtractHere, "ExtractHere", IDS_CONTEXT_EXTRACT_HERE),
    CMD_REC(kExtractTo, "ExtractTo", IDS_CONTEXT_EXTRACT_TO),
    CMD_REC(kTest, "Test", IDS_CONTEXT_TEST),
    CMD_REC(kCompress, "Compress", IDS_CONTEXT_COMPRESS),
    CMD_REC(kCompressEmail, "CompressEmail", IDS_CONTEXT_COMPRESS_EMAIL),
    CMD_REC(kCompressTo7z, "CompressTo7z", IDS_CONTEXT_COMPRESS_TO),
    CMD_REC(kCompressTo7zEmail, "CompressTo7zEmail", IDS_CONTEXT_COMPRESS_TO_EMAIL),
    CMD_REC(kCompressToZip, "CompressToZip", IDS_CONTEXT_COMPRESS_TO),
    CMD_REC(kCompressToZipEmail, "CompressToZipEmail", IDS_CONTEXT_COMPRESS_TO_EMAIL)};

struct CHashCommand {
    CZipContextMenu::enum_CommandInternalID CommandInternalID;
    LPCSTR UserName;
    LPCSTR MethodName;
};

static const CHashCommand g_HashCommands[] = {
    {CZipContextMenu::kHash_CRC32, "CRC-32", "CRC32"},
    {CZipContextMenu::kHash_CRC64, "CRC-64", "CRC64"},
    {CZipContextMenu::kHash_XXH64, "XXH64", "XXH64"},
    {CZipContextMenu::kHash_MD5, "MD5", "MD5"},
    {CZipContextMenu::kHash_SHA1, "SHA-1", "SHA1"},
    {CZipContextMenu::kHash_SHA256, "SHA-256", "SHA256"},
    {CZipContextMenu::kHash_SHA384, "SHA-384", "SHA384"},
    {CZipContextMenu::kHash_SHA512, "SHA-512", "SHA512"},
    {CZipContextMenu::kHash_SHA3_256, "SHA3-256", "SHA3-256"},
    {CZipContextMenu::kHash_BLAKE2SP, "BLAKE2sp", "BLAKE2sp"},
    {CZipContextMenu::kHash_All, "*", "*"},
    {CZipContextMenu::kHash_Generate_SHA256, "SHA-256 -> file.sha256", "SHA256"},
    {CZipContextMenu::kHash_TestArc, "Checksum : Test", "Hash"}};

static int FindCommand(CZipContextMenu::enum_CommandInternalID &id) {
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_Commands); i++)
        if (g_Commands[i].CommandInternalID == id)
            return (int)i;
    return -1;
}

void CZipContextMenu::FillCommand(enum_CommandInternalID id, UString &mainString,
                                  CCommandMapItem &cmi) const {
    mainString.Empty();
    const int i = FindCommand(id);
    if (i < 0)
        throw 201908;
    const CContextMenuCommand &command = g_Commands[(unsigned)i];
    cmi.CommandInternalID = command.CommandInternalID;
    cmi.Verb = kMainVerb;
    cmi.Verb += command.Verb;
    // cmi.HelpString = cmi.Verb;
    LangString(command.ResourceID, mainString);
    cmi.UserString = mainString;
}

static UString LangStringAlt(UInt32 id, const char *altString) {
    UString s = LangString(id);
    if (s.IsEmpty())
        s = altString;
    return s;
}

void CZipContextMenu::AddCommand(enum_CommandInternalID id, UString &mainString,
                                 CCommandMapItem &cmi) {
    FillCommand(id, mainString, cmi);
    _commandMap.Add(cmi);
}

/*
note: old msdn article:
Duplicate Menu Items In the File Menu For a Shell Context Menu Extension (214477)
----------
  On systems with Shell32.dll version 4.71 or higher, a context menu extension
  for a file folder that inserts one or more pop-up menus results in duplicates
  of these menu items.
  This occurs when the file menu is activated more than once for the selected object.

CAUSE
  In a context menu extension, if pop-up menus are inserted using InsertMenu
  or AppendMenu, then the ID for the pop-up menu item cannot be specified.
  Instead, this field should take in the CMenuHandle * of the pop-up menu.
  Because the ID is not specified for the pop-up menu item, the Shell does
  not keep track of the menu item if the file menu is pulled down multiple times.
  As a result, the pop-up menu items are added multiple times in the context menu.

  This problem occurs only when the file menu is pulled down, and does not happen
  when the context menu is invoked by using the right button or the context menu key.
RESOLUTION
  To work around this problem, use InsertMenuItem and specify the ID of the
  pop-up menu  item in the wID member of the MENUITEMINFO structure.
*/

static void MyInsertMenu(CMenu &menu, unsigned pos, UINT id, const UString &s, unsigned bitmap) {
    if (!menu)
        return;
    CMenuItem mi;
    mi.fType = kString;
    mi.fMask = kType | kId;
    if (bitmap)
        mi.fMask |= kCheckmarks;
    mi.wID = id;
    mi.StringValue = s;
    mi.hbmpUnchecked = bitmap;
    // mi.hbmpChecked = bitmap; // do we need hbmpChecked ???
    if (!menu.InsertItem(pos, true, mi))
        throw 20190816;

    // SetMenuItemBitmaps also works
    // ::SetMenuItemBitmaps(menu, pos, MF_BYPOSITION, bitmap, NULL);
}

static void MyAddSubMenu(CObjectVector<CZipContextMenu::CCommandMapItem> &_commandMap,
                         const char *verb, CMenu &menu, unsigned pos, UINT id, const UString &s,
                         CMenuHandle *hSubMenu, unsigned bitmap) {
    CZipContextMenu::CCommandMapItem cmi;
    cmi.CommandInternalID = CZipContextMenu::kCommandNULL;
    cmi.Verb = verb;
    cmi.IsPopup = true;
    // cmi.HelpString = verb;
    cmi.UserString = s;
    _commandMap.Add(cmi);

    if (!menu)
        return;

    CMenuItem mi;
    mi.fType = kString;
    mi.fMask = kSubmenu | kType | kId;
    if (bitmap)
        mi.fMask |= kCheckmarks;
    mi.wID = id;
    mi.hSubMenu = hSubMenu;
    mi.hbmpUnchecked = bitmap;

    mi.StringValue = s;
    if (!menu.InsertItem(pos, true, mi))
        throw 20190817;
}

static const char *const kArcExts[] = {"7z", "bz2", "gz", "rar", "zip"};

static bool IsItArcExt(const UString &ext) {
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(kArcExts); i++)
        if (ext.IsEqualTo_Ascii_NoCase(kArcExts[i]))
            return true;
    return false;
}

UString GetSubFolderNameForExtract(const UString &arcName);
UString GetSubFolderNameForExtract(const UString &arcName) {
    int dotPos = arcName.ReverseFind_Dot();
    if (dotPos < 0)
        return Get_Correct_FsFile_Name(arcName) + L'~';

    const UString ext = arcName.Ptr(dotPos + 1);
    UString res = arcName.Left(dotPos);
    res.TrimRight();
    dotPos = res.ReverseFind_Dot();
    if (dotPos > 0) {
        const UString ext2 = res.Ptr(dotPos + 1);
        if ((ext.IsEqualTo_Ascii_NoCase("001") && IsItArcExt(ext2)) ||
            (ext.IsEqualTo_Ascii_NoCase("rar") &&
             (ext2.IsEqualTo_Ascii_NoCase("part001") || ext2.IsEqualTo_Ascii_NoCase("part01") ||
              ext2.IsEqualTo_Ascii_NoCase("part1"))))
            res.DeleteFrom(dotPos);
        res.TrimRight();
    }
    return Get_Correct_FsFile_Name(res);
}

static void ReduceString(UString &s) {
    const unsigned kMaxSize = 64;
    if (s.Len() <= kMaxSize)
        return;
    s.Delete(kMaxSize / 2, s.Len() - kMaxSize);
    s.Insert(kMaxSize / 2, L" ... ");
}

static UString GetQuotedReducedString(const UString &s) {
    UString s2 = s;
    ReduceString(s2);
    s2.Replace(L"&", L"&&");
    return GetQuotedString(s2);
}

static void MyFormatNew_ReducedName(UString &s, const UString &name) {
    s = MyFormatNew(s, GetQuotedReducedString(name));
}

static const char *const kExtractExcludeExtensions =
    " 3gp"
    " aac ans ape asc asm asp aspx avi awk"
    " bas bat bmp"
    " c cs cls clw cmd cpp csproj css ctl cxx"
    " def dep dlg dsp dsw"
    " eps"
    " f f77 f90 f95 fla flac frm"
    " gif"
    " h hpp hta htm html hxx"
    " ico idl inc ini inl"
    " java jpeg jpg js"
    " la lnk log"
    " mak manifest wmv mov mp3 mp4 mpe mpeg mpg m4a"
    " ofr ogg"
    " pac pas pdf php php3 php4 php5 phptml pl pm png ps py pyo"
    " ra rb rc reg rka rm rtf"
    " sed sh shn shtml sln sql srt swa"
    " tcl tex tiff tta txt"
    " vb vcproj vbs"
    " mkv wav webm wma wv"
    " xml xsd xsl xslt"
    " ";

/*
static const char * const kNoOpenAsExtensions =
  " 7z arj bz2 cab chm cpio flv gz lha lzh lzma rar swm tar tbz2 tgz wim xar xz z zip ";
*/

static const char *const kOpenTypes[] = {"",
                                         "*",
                                         "#",
                                         "#:e"
                                         // , "#:a"
                                         ,
                                         "7z",
                                         "zip",
                                         "cab",
                                         "rar"};

bool FindExt(const char *p, const UString &name, CStringFinder &finder);
bool FindExt(const char *p, const UString &name, CStringFinder &finder) {
    const int dotPos = name.ReverseFind_Dot();
    int len = (int)name.Len() - (dotPos + 1);
    if (len == 0 || len > 32 || dotPos < 0)
        return false;
    return finder.FindWord_In_LowCaseAsciiList_NoCase(p, name.Ptr(dotPos + 1));
}

/* returns false, if extraction of that file extension is not expected */
static bool DoNeedExtract(const UString &name, CStringFinder &finder) {
    // for (int y = 0; y < 1000; y++) FindExt(kExtractExcludeExtensions, name);
    return !FindExt(kExtractExcludeExtensions, name, finder);
}

// we must use diferent Verbs for Popup subMenu.
void CZipContextMenu::AddMapItem_ForSubMenu(const char *verb) {
    CCommandMapItem cmi;
    cmi.CommandInternalID = kCommandNULL;
    cmi.Verb = verb;
    // cmi.HelpString = verb;
    _commandMap.Add(cmi);
}

static HRESULT RETURN_WIN32_LastError_AS_HRESULT() { return E_FAIL; }

/*
  we add CCommandMapItem to _commandMap for each new Menu ID.
  so then we use _commandMap[offset].
  That way we can execute commands that have menu item.
  Another non-implemented way:
    We can return the number off all possible commands in QueryContextMenu().
    so the caller could call InvokeCommand() via string verb even
    without using menu items.
*/

HRESULT CZipContextMenu::QueryContextMenu(CMenuHandle *hMenu, UINT indexMenu, UINT commandIDFirst,
                                          UINT commandIDLast, UINT flags) {
    ODS("+ QueryContextMenu()")
    COM_TRY_BEGIN
    try {

        _commandMap.Clear();

        ODS_SPRF_s(sprintf(s, "QueryContextMenu: index=%u first=%u last=%u flags=%x _files=%u",
                           indexMenu, commandIDFirst, commandIDLast, flags, _fileNames.Size()))
        /*
        for (UInt32 i = 0; i < _fileNames.Size(); i++)
        {
          ODS_U(_fileNames[i])
        }
        */

#define MAKE_HRESULT_SUCCESS_FAC0(code) (HRESULT)(code)

            if (_fileNames.Size() == 0) {
            return MAKE_HRESULT_SUCCESS_FAC0(0);
            // return E_INVALIDARG;
        }

        if (commandIDFirst > commandIDLast)
            return E_INVALIDARG;

        UINT currentCommandID = commandIDFirst;

        if ((flags & 0x000F) != kCmfNormal && (flags & kCmfVerbsOnly) == 0 &&
            (flags & kCmfExplore) == 0)
            return MAKE_HRESULT_SUCCESS_FAC0(currentCommandID - commandIDFirst);
        // return MAKE_HRESULT_SUCCESS_FAC0(currentCommandID);
        // 19.01 : we changed from (currentCommandID) to (currentCommandID - commandIDFirst)
        // why it was so before?

#ifdef Z7_LANG
        LoadLangOneTime();
#endif

        CMenu popupMenu;
        CMenuDestroyer menuDestroyer;

        ODS("### 40")
        CContextMenuInfo ci;
        ci.Load(_settingsPath);
        ODS("### 44")

        _elimDup = ci.ElimDup;
        _writeZone = ci.WriteZone;

        unsigned bitmap = 0;
        if (ci.MenuIcons.Val) {
            ODS("### 45")
            if (!_bitmap)
                _bitmap = 1;
            bitmap = _bitmap;
        }

        UINT subIndex = indexMenu;

        ODS("### 50")

        if (ci.Cascaded.Val) {
            if (hMenu)
                if (!popupMenu.CreatePopup())
                    return RETURN_WIN32_LastError_AS_HRESULT();
            menuDestroyer.Attach(popupMenu);

            /* 9.31: we commented the following code. Probably we don't need.
            Check more systems. Maybe it was for old Windows? */
            /*
            AddMapItem_ForSubMenu();
            currentCommandID++;
            */
            subIndex = 0;
        } else {
            popupMenu.Attach(hMenu);
            CMenuItem mi;
            mi.fType = kSeparator;
            mi.fMask = kType;
            if (hMenu)
                popupMenu.InsertItem(subIndex++, true, mi);
        }

        const UInt32 contextMenuFlags = ci.Flags;

        NFind::CFileInfo fi0;
        FString folderPrefix;

        if (_fileNames.Size() > 0) {
            const UString &fileName = _fileNames.Front();

#if defined(_WIN32) && !defined(UNDER_CE)
            if (NName::IsDevicePath(us2fs(fileName))) {
                // CFileInfo::Find can be slow for device files. So we don't call it.
                // we need only name here.
                fi0.Name = us2fs(fileName.Ptr(NName::kDevicePathPrefixSize));
                folderPrefix =
#ifdef UNDER_CE
                    "\\";
#else
                    "C:\\";
#endif
            } else
#endif
            {
                if (!fi0.Find(us2fs(fileName))) {
                    throw 20190820;
                    // return RETURN_WIN32_LastError_AS_HRESULT();
                }
                GetOnlyDirPrefix(us2fs(fileName), folderPrefix);
            }
        }

        ODS("### 100")

        UString mainString;
        CStringFinder finder;
        UStringVector fileNames_Reduced;
        const unsigned k_Explorer_NumReducedItems = 16;
        const bool needReduce = !_isMenuForFM && (_fileNames.Size() >= k_Explorer_NumReducedItems);
        _fileNames_WereReduced = needReduce;
        // _fileNames_WereReduced = true; // for debug;
        const UStringVector *fileNames = &_fileNames;
        if (needReduce) {
            for (unsigned i = 0; i < k_Explorer_NumReducedItems && i < _fileNames.Size(); i++)
                fileNames_Reduced.Add(_fileNames[i]);
            fileNames = &fileNames_Reduced;
        }

        /*
        if (_fileNames.Size() == k_Explorer_NumReducedItems) // for debug
        {
          for (int i = 0; i < 10; i++)
          {
            CCommandMapItem cmi;
            AddCommand(kCompressToZipEmail, mainString, cmi);
            MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString, bitmap);
          }
        }
        */

        if (_fileNames.Size() == 1 && currentCommandID + 14 <= commandIDLast) {
            if (!fi0.IsDir() && DoNeedExtract(fs2us(fi0.Name), finder)) {
                // Open
                const bool thereIsMainOpenItem =
                    ((contextMenuFlags & NContextMenuFlags::kOpen) != 0);
                if (thereIsMainOpenItem) {
                    CCommandMapItem cmi;
                    AddCommand(kOpen, mainString, cmi);
                    MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString, bitmap);
                }
                if ((contextMenuFlags & NContextMenuFlags::kOpenAs) != 0
                    // && (!thereIsMainOpenItem || !FindExt(kNoOpenAsExtensions, fi0.Name))
                    && hMenu // we want to reduce number of menu items below 16
                ) {
                    CMenu subMenu;
                    if (!hMenu || subMenu.CreatePopup()) {
                        MyAddSubMenu(_commandMap, kOpenCascadedVerb, popupMenu, subIndex++,
                                     currentCommandID++, LangString(IDS_CONTEXT_OPEN), subMenu,
                                     bitmap);
                        _commandMap.Back().CtxCommandType = CtxCommandType_OpenRoot;

                        UINT subIndex2 = 0;
                        for (unsigned i = (thereIsMainOpenItem ? 1 : 0);
                             i < Z7_ARRAY_SIZE(kOpenTypes); i++) {
                            CCommandMapItem cmi;
                            if (i == 0)
                                FillCommand(kOpen, mainString, cmi);
                            else {
                                mainString = kOpenTypes[i];
                                cmi.CommandInternalID = kOpen;
                                cmi.Verb = kMainVerb;
                                cmi.Verb += ".Open.";
                                cmi.Verb += mainString;
                                // cmi.HelpString = cmi.Verb;
                                cmi.ArcType = mainString;
                                cmi.CtxCommandType = CtxCommandType_OpenChild;
                            }
                            _commandMap.Add(cmi);
                            Set_UserString_in_LastCommand(mainString);
                            MyInsertMenu(subMenu, subIndex2++, currentCommandID++, mainString,
                                         bitmap);
                        }

                        subMenu.Detach();
                    }
                }
            }
        }

        ODS("### 150")

        if (_fileNames.Size() > 0 && currentCommandID + 10 <= commandIDLast) {
            ODS("### needExtract list START")
            const bool needExtendedVerbs = ((flags & Z7_WIN_CMF_EXTENDEDVERBS) != 0);
            // || _isMenuForFM;
            bool needExtract = true;
            bool areDirs =
                fi0.IsDir() || (unsigned)_attribs.FirstDirIndex < k_Explorer_NumReducedItems;
            if (!needReduce)
                areDirs = areDirs || (_attribs.FirstDirIndex != -1);
            if (areDirs)
                needExtract = false;

            if (!needExtendedVerbs)
                if (needExtract) {
                    UString name;
                    const unsigned numItemsCheck = fileNames->Size();
                    for (unsigned i = 0; i < numItemsCheck; i++) {
                        const UString &a = (*fileNames)[i];
                        const int slash = a.ReverseFind_PathSepar();
                        name = a.Ptr(slash + 1);
                        // for (int y = 0; y < 600; y++) // for debug
                        const bool needExtr2 = DoNeedExtract(name, finder);
                        if (!needExtr2) {
                            needExtract = needExtr2;
                            break;
                        }
                    }
                }
            ODS("### needExtract list END")

            if (needExtract) {
                {
                    UString baseFolder = fs2us(folderPrefix);
                    if (_dropMode)
                        baseFolder = _dropPath;

                    UString specFolder('*');
                    if (_fileNames.Size() == 1)
                        specFolder = GetSubFolderNameForExtract(fs2us(fi0.Name));
                    specFolder.Add_PathSepar();

                    if ((contextMenuFlags & NContextMenuFlags::kExtract) != 0) {
                        // Extract
                        CCommandMapItem cmi;
                        cmi.Folder = baseFolder + specFolder;
                        AddCommand(kExtract, mainString, cmi);
                        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString, bitmap);
                    }

                    if ((contextMenuFlags & NContextMenuFlags::kExtractHere) != 0) {
                        // Extract Here
                        CCommandMapItem cmi;
                        cmi.Folder = baseFolder;
                        AddCommand(kExtractHere, mainString, cmi);
                        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString, bitmap);
                    }

                    if ((contextMenuFlags & NContextMenuFlags::kExtractTo) != 0) {
                        // Extract To
                        CCommandMapItem cmi;
                        UString s;
                        cmi.Folder = baseFolder + specFolder;
                        AddCommand(kExtractTo, s, cmi);
                        MyFormatNew_ReducedName(s, specFolder);
                        Set_UserString_in_LastCommand(s);
                        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s, bitmap);
                    }
                }

                if ((contextMenuFlags & NContextMenuFlags::kTest) != 0) {
                    // Test
                    CCommandMapItem cmi;
                    AddCommand(kTest, mainString, cmi);
                    // if (_fileNames.Size() == 16) mainString += "_[16]"; // for debug
                    MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString, bitmap);
                }
            }

            ODS("### CreateArchiveName START")
            UString arcName_base;
            const UString arcName =
                CreateArchiveName(*fileNames,
                                  false, // isHash
                                  fileNames->Size() == 1 ? &fi0 : NULL, arcName_base);
            ODS("### CreateArchiveName END")
            UString arcName_Show = arcName;
            if (needReduce) {
                /* we need same arcName_Show for two calls from Explorer:
                      1) reduced call (only first 16 items)
                      2) full call with all items (can be >= 16 items)
                   (fileNames) array was reduced to 16 items.
                   So we will have same (arcName) in both reduced and full calls.
                   If caller (Explorer) uses (reduce_to_first_16_items) scheme,
                   we can use (arcName) here instead of (arcName_base).
                   (arcName_base) has no number in name.
                */
                arcName_Show = arcName_base; // we can comment that line
                /* we use "_" in archive name as sign to user
                   that shows that final archive name can be changed. */
                arcName_Show += "_";
            }

            UString arcName_7z = arcName;
            arcName_7z += ".7z";
            UString arcName_7z_Show = arcName_Show;
            arcName_7z_Show += ".7z";
            UString arcName_zip = arcName;
            arcName_zip += ".zip";
            UString arcName_zip_Show = arcName_Show;
            arcName_zip_Show += ".zip";

            // Compress
            if ((contextMenuFlags & NContextMenuFlags::kCompress) != 0) {
                CCommandMapItem cmi;
                if (_dropMode)
                    cmi.Folder = _dropPath;
                else
                    cmi.Folder = fs2us(folderPrefix);
                cmi.ArcName = arcName;
                AddCommand(kCompress, mainString, cmi);
                MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString, bitmap);
            }

#ifdef EMAIL_SUPPORT
            // CompressEmail
            if ((contextMenuFlags & NContextMenuFlags::kCompressEmail) != 0 && !_dropMode) {
                CCommandMapItem cmi;
                cmi.ArcName = arcName;
                AddCommand(kCompressEmail, mainString, cmi);
                MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString, bitmap);
            }
#endif

            // CompressTo7z
            if (contextMenuFlags & NContextMenuFlags::kCompressTo7z &&
                !arcName_7z.IsEqualTo_NoCase(fs2us(fi0.Name))) {
                CCommandMapItem cmi;
                UString s;
                if (_dropMode)
                    cmi.Folder = _dropPath;
                else
                    cmi.Folder = fs2us(folderPrefix);
                cmi.ArcName = arcName_7z;
                cmi.ArcType = "7z";
                AddCommand(kCompressTo7z, s, cmi);
                MyFormatNew_ReducedName(s, arcName_7z_Show);
                Set_UserString_in_LastCommand(s);
                MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s, bitmap);
            }

#ifdef EMAIL_SUPPORT
            // CompressTo7zEmail
            if ((contextMenuFlags & NContextMenuFlags::kCompressTo7zEmail) != 0 && !_dropMode) {
                CCommandMapItem cmi;
                UString s;
                cmi.ArcName = arcName_7z;
                cmi.ArcType = "7z";
                AddCommand(kCompressTo7zEmail, s, cmi);
                MyFormatNew_ReducedName(s, arcName_7z_Show);
                Set_UserString_in_LastCommand(s);
                MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s, bitmap);
            }
#endif

            // CompressToZip
            if (contextMenuFlags & NContextMenuFlags::kCompressToZip &&
                !arcName_zip.IsEqualTo_NoCase(fs2us(fi0.Name))) {
                CCommandMapItem cmi;
                UString s;
                if (_dropMode)
                    cmi.Folder = _dropPath;
                else
                    cmi.Folder = fs2us(folderPrefix);
                cmi.ArcName = arcName_zip;
                cmi.ArcType = "zip";
                AddCommand(kCompressToZip, s, cmi);
                MyFormatNew_ReducedName(s, arcName_zip_Show);
                Set_UserString_in_LastCommand(s);
                MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s, bitmap);
            }

#ifdef EMAIL_SUPPORT
            // CompressToZipEmail
            if ((contextMenuFlags & NContextMenuFlags::kCompressToZipEmail) != 0 && !_dropMode) {
                CCommandMapItem cmi;
                UString s;
                cmi.ArcName = arcName_zip;
                cmi.ArcType = "zip";
                AddCommand(kCompressToZipEmail, s, cmi);
                MyFormatNew_ReducedName(s, arcName_zip_Show);
                Set_UserString_in_LastCommand(s);
                MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s, bitmap);
            }
#endif
        }

        ODS("### 300")

        // don't use InsertMenu:  See MSDN:
        // PRB: Duplicate Menu Items In the File Menu For a Shell Context Menu Extension
        // ID: Q214477

        if (ci.Cascaded.Val) {
            CMenu menu;
            menu.Attach(hMenu);
            menuDestroyer.Disable();
            MyAddSubMenu(_commandMap, kMainVerb, menu, indexMenu++, currentCommandID++,
                         (UString) "7-Zip",
                         popupMenu, // popupMenu.Detach(),
                         bitmap);
        } else {
            // popupMenu.Detach();
            indexMenu = subIndex;
        }

        ODS("### 350")

        const bool needCrc = ((contextMenuFlags &
                               (NContextMenuFlags::kCRC | NContextMenuFlags::kCRC_Cascaded)) != 0);

        if (
            // !_isMenuForFM && // 21.04: we don't hide CRC SHA menu in 7-Zip FM
            needCrc && currentCommandID + 1 < commandIDLast) {
            CMenu subMenu;
            // CMenuDestroyer menuDestroyer_CRC;

            UINT subIndex_CRC = 0;

            if (!hMenu || subMenu.CreatePopup()) {
                // menuDestroyer_CRC.Attach(subMenu);
                const bool insertHashMenuTo7zipMenu =
                    (ci.Cascaded.Val && (contextMenuFlags & NContextMenuFlags::kCRC_Cascaded) != 0);

                CMenu menu;
                {
                    unsigned indexInParent;
                    if (insertHashMenuTo7zipMenu) {
                        indexInParent = subIndex;
                        menu.Attach(popupMenu);
                    } else {
                        indexInParent = indexMenu;
                        menu.Attach(hMenu);
                        // menuDestroyer_CRC.Disable();
                    }
                    MyAddSubMenu(_commandMap, kCheckSumCascadedVerb, menu, indexInParent++,
                                 currentCommandID++, (UString) "CRC SHA", subMenu,
                                 /* insertHashMenuTo7zipMenu ? NULL : */ bitmap);
                    _commandMap.Back().CtxCommandType = CtxCommandType_CrcRoot;
                    if (!insertHashMenuTo7zipMenu)
                        indexMenu = indexInParent;
                }

                ODS("### HashCommands")

                for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_HashCommands); i++) {
                    if (currentCommandID >= commandIDLast)
                        break;
                    const CHashCommand &hc = g_HashCommands[i];
                    CCommandMapItem cmi;
                    cmi.CommandInternalID = hc.CommandInternalID;
                    cmi.Verb = kCheckSumCascadedVerb;
                    cmi.Verb.Add_Dot();
                    UString s;
                    s += hc.UserName;

                    if (hc.CommandInternalID == kHash_Generate_SHA256) {
                        cmi.Verb += "Generate";
                        {
                            popupMenu.Attach(hMenu);
                            CMenuItem mi;
                            mi.fType = kSeparator;
                            mi.fMask = kType;
                            subMenu.InsertItem(subIndex_CRC++, true, mi);
                        }

                        UString name;
                        UString showName;
                        ODS("### Hash CreateArchiveName Start")
                        // for (int y = 0; y < 10000; y++) // for debug
                        // if (fileNames->Size() == 1) name = fs2us(fi0.Name); else
                        name = CreateArchiveName(*fileNames,
                                                 true, // isHash
                                                 fileNames->Size() == 1 ? &fi0 : NULL, showName);
                        if (needReduce)
                            showName += "_";
                        else
                            showName = name;

                        ODS("### Hash CreateArchiveName END")
                        name += ".sha256";
                        showName += ".sha256";
                        cmi.Folder = fs2us(folderPrefix);
                        cmi.ArcName = name;
                        s = "SHA-256 -> ";
                        s += showName;
                    } else if (hc.CommandInternalID == kHash_TestArc) {
                        cmi.Verb += "Test";
                        s = LangStringAlt(IDS_CONTEXT_TEST, "Test archive");
                        s += " : ";
                        s += GetNameOfProperty(kpidChecksum, UString("Checksum"));
                    } else
                        cmi.Verb += "Calc";

                    cmi.Verb.Add_Dot();
                    cmi.Verb += hc.MethodName;

                    // cmi.HelpString = cmi.Verb;
                    cmi.UserString = s;
                    cmi.CtxCommandType = CtxCommandType_CrcChild;
                    _commandMap.Add(cmi);
                    MyInsertMenu(subMenu, subIndex_CRC++, currentCommandID++, s, bitmap);
                    ODS("### 380")
                }

                subMenu.Detach();
            }
        }

        popupMenu.Detach();
        /*
        if (!ci.Cascaded.Val)
          indexMenu = subIndex;
        */
        const unsigned numCommands = currentCommandID - commandIDFirst;
        ODS("+ QueryContextMenu() END")
        ODS_SPRF_s(sprintf(s, "Commands=%u currentCommandID - commandIDFirst = %u",
                           _commandMap.Size(),
                           numCommands)) if (_commandMap.Size() != numCommands) throw 20190818;
        /*
        FOR_VECTOR (k, _commandMap)
        {
          ODS_U(_commandMap[k].Verb);
        }
        */
    } catch (...) {
        ODS_SPRF_s(sprintf(s, "catch() exception: Commands=%u",
                           _commandMap.Size())) if (_commandMap.Size() == 0) throw;
    }
    /* we added some menu items already : num_added_menu_items,
       So we MUST return (number_of_defined_ids), where (number_of_defined_ids >=
       num_added_menu_items) This will prevent incorrect menu working, when same IDs can be assigned
       in multiple menu items from different subhandlers. And we must add items to _commandMap
       before adding to menu.
     */
    return MAKE_HRESULT_SUCCESS_FAC0(_commandMap.Size());
    COM_TRY_END
}

int CZipContextMenu::FindVerb(const UString &verb) const {
    FOR_VECTOR(i, _commandMap)
    if (_commandMap[i].Verb == verb)
        return (int)i;
    return -1;
}

HRESULT CZipContextMenu::InvokeCommand(unsigned commandOffset) {
    COM_TRY_BEGIN
    // Native/Qt facades have already decoded the integer or Unicode verb.
    if (commandOffset >= _commandMap.Size())
        return E_INVALIDARG;
    const CCommandMapItem &cmi = _commandMap[commandOffset];
    return InvokeCommandCommon(cmi);
    COM_TRY_END
}

HRESULT CZipContextMenu::InvokeCommandCommon(const CCommandMapItem &cmi) {
    if (!_services || _fileNames.IsEmpty())
        return E_INVALIDARG;
    const enum_CommandInternalID cmdID = cmi.CommandInternalID;

    try {
        switch (cmdID) {
        case kOpen: {
            _services->Open(_fileNames[0], cmi.ArcType);
            break;
        }
        case kExtract:
        case kExtractHere:
        case kExtractTo: {
            if (_attribs.FirstDirIndex != -1) {
                _services->ShowErrorMessageRes(IDS_SELECT_FILES);
                break;
            }
            _services->ExtractArchives(_fileNames, cmi.Folder,
                                       (cmdID == kExtract),                   // showDialog
                                       (cmdID == kExtractTo) && _elimDup.Val, // elimDup
                                       _writeZone);
            break;
        }
        case kTest: {
            _services->TestArchives(_fileNames);
            break;
        }
        case kCompress:
        case kCompressEmail:
        case kCompressTo7z:
        case kCompressTo7zEmail:
        case kCompressToZip:
        case kCompressToZipEmail: {
            UString arcName = cmi.ArcName;
            if (_fileNames_WereReduced) {
                UString arcName_base;
                arcName = CreateArchiveName(_fileNames,
                                            false, // isHash
                                            NULL,  // fi0
                                            arcName_base);
                const char *postfix = NULL;
                if (cmdID == kCompressTo7z || cmdID == kCompressTo7zEmail)
                    postfix = ".7z";
                else if (cmdID == kCompressToZip || cmdID == kCompressToZipEmail)
                    postfix = ".zip";
                if (postfix)
                    arcName += postfix;
            }

            const bool email = cmdID == kCompressEmail || cmdID == kCompressTo7zEmail ||
                               cmdID == kCompressToZipEmail;
            const bool showDialog = cmdID == kCompress || cmdID == kCompressEmail;
            const bool addExtension = showDialog;
            _services->CompressFiles(cmi.Folder, arcName, cmi.ArcType, addExtension, _fileNames,
                                     email, showDialog,
                                     false // waitFinish
            );
            break;
        }

        case kHash_CRC32:
        case kHash_CRC64:
        case kHash_XXH64:
        case kHash_MD5:
        case kHash_SHA1:
        case kHash_SHA256:
        case kHash_SHA384:
        case kHash_SHA512:
        case kHash_SHA3_256:
        case kHash_BLAKE2SP:
        case kHash_All:
        case kHash_Generate_SHA256:
        case kHash_TestArc: {
            for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_HashCommands); i++) {
                const CHashCommand &hc = g_HashCommands[i];
                if (hc.CommandInternalID == cmdID) {
                    if (cmdID == kHash_TestArc) {
                        _services->TestArchives(_fileNames, true); // hashMode
                        break;
                    }
                    UString generateName;
                    if (cmdID == kHash_Generate_SHA256) {
                        generateName = cmi.ArcName;
                        if (_fileNames_WereReduced) {
                            UString arcName_base;
                            generateName = CreateArchiveName(_fileNames,
                                                             true, // isHash
                                                             NULL, // fi0
                                                             arcName_base);
                            generateName += ".sha256";
                        }
                    }
                    _services->CalcChecksum(_fileNames, (UString)hc.MethodName, cmi.Folder,
                                            generateName);
                    break;
                }
            }
            break;
        }
        case kCommandNULL:
            break;
        }
    } catch (...) {
        _services->ShowErrorMessage(UString(L"Error"));
    }
    return S_OK;
}
