// FilePlugins.h

#ifndef ZIP7_INC_FILE_PLUGINS_H
#define ZIP7_INC_FILE_PLUGINS_H

#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyString.h"

struct CPluginToIcon {
    // unsigned PluginIndex;
    int IconIndex;
    UString IconPath;

    CPluginToIcon() : IconIndex(-1) {}
};

struct CExtPlugins {
    UString Ext;
    CObjectVector<CPluginToIcon> Plugins;
};

class CExtDatabase {
    int FindExt(const UString &ext) const;

  public:
    CObjectVector<CExtPlugins> Exts;
    // CObjectVector<CPluginInfo> Plugins;

    void Read(IFolderManager *folderManager);
};

#endif
