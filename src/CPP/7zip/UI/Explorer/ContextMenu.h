// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial source command owner; COM/native interfaces use separate adapters.
#pragma once
#include "../Common/ZipRegistry.h"
#include "Windows/Shell.h"
#include "qtcontextmenuservices.h"
#include "qtshellmenupresentation.h"
#include <QString>
enum enum_CtxCommandType {
    CtxCommandType_Normal,
    CtxCommandType_OpenRoot,
    CtxCommandType_OpenChild,
    CtxCommandType_CrcRoot,
    CtxCommandType_CrcChild
};
class CZipContextMenu {
  public:
    enum enum_CommandInternalID {
        kCommandNULL,
        kOpen,
        kExtract,
        kExtractHere,
        kExtractTo,
        kTest,
        kCompress,
        kCompressEmail,
        kCompressTo7z,
        kCompressTo7zEmail,
        kCompressToZip,
        kCompressToZipEmail,
        kHash_CRC32,
        kHash_CRC64,
        kHash_XXH64,
        kHash_MD5,
        kHash_SHA1,
        kHash_SHA256,
        kHash_SHA384,
        kHash_SHA512,
        kHash_SHA3_256,
        kHash_BLAKE2SP,
        kHash_All,
        kHash_Generate_SHA256,
        kHash_TestArc
    };

    struct CCommandMapItem {
        enum_CommandInternalID CommandInternalID;
        UString Verb;
        UString UserString;
        // UString HelpString;
        UString Folder;
        UString ArcName;
        UString ArcType;
        bool IsPopup;
        enum_CtxCommandType CtxCommandType;

        CCommandMapItem() : IsPopup(false), CtxCommandType(CtxCommandType_Normal) {}

        bool IsSubMenu() const {
            return CtxCommandType == CtxCommandType_CrcRoot ||
                   CtxCommandType == CtxCommandType_OpenRoot;
        }
    };

    UStringVector _fileNames;
    NWindows::NShell::CFileAttribs _attribs;

  private:
    bool _isMenuForFM = true;
    bool _fileNames_WereReduced = true;
    bool _dropMode = false;
    UString _dropPath;
    CObjectVector<CCommandMapItem> _commandMap;
    unsigned _bitmap = 0;
    UInt32 _writeZone = UInt32(-1);
    CBoolPair _elimDup;
    QString _settingsPath;
    IQtContextMenuServices *_services;
    void Set_UserString_in_LastCommand(const UString &s) { _commandMap.Back().UserString = s; }
    void FillCommand(enum_CommandInternalID id, UString &mainString, CCommandMapItem &cmi) const;
    void AddCommand(enum_CommandInternalID id, UString &mainString, CCommandMapItem &cmi);
    void AddMapItem_ForSubMenu(const char *verb);

  public:
    CZipContextMenu(const QString &settingsPath, IQtContextMenuServices *services)
        : _settingsPath(settingsPath), _services(services) {}
    void Init_For_7zFM() { _isMenuForFM = true; }
    void Init_For_Shell() { _isMenuForFM = false; }
    void SetDropPath(const UString &path) {
        _dropPath = path;
        _dropMode = !path.IsEmpty();
    }
    HRESULT QueryContextMenu(NQtShellMenu::CMenuHandle *menu, unsigned indexMenu, unsigned first,
                             unsigned last, unsigned flags);
    int FindVerb(const UString &verb) const;
    HRESULT InvokeCommand(unsigned commandOffset);
    HRESULT InvokeCommandCommon(const CCommandMapItem &cmi);
    const CObjectVector<CCommandMapItem> &Commands() const { return _commandMap; }
};
