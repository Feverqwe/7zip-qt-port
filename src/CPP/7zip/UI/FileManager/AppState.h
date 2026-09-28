// Adapted from 7-Zip CPP/7zip/UI/FileManager/AppState.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// State is owned by the GUI thread; Registry and its lock are replaced by
// explicit QSettings paths. Upstream strings and vectors remain authoritative.
#pragma once

#include "Common/MyString.h"
#include "ViewSettings.h"

class CFastFolders {
  public:
    UStringVector Strings;
    void SetString(unsigned index, const UString &s) {
        while (Strings.Size() <= index)
            Strings.AddNew();
        Strings[index] = s;
    }
    UString GetString(unsigned index) {
        if (index >= Strings.Size())
            return UString();
        return Strings[index];
    }
    void Save(const QString &settingsPath) { SaveFastFolders(Strings, settingsPath); }
    void Read(const QString &settingsPath) { ReadFastFolders(Strings, settingsPath); }
};

class CFolderHistory {
    UStringVector Strings;
    void Normalize();

  public:
    void GetList(UStringVector &foldersHistory) { foldersHistory = Strings; }
    void AddString(const UString &s);
    void RemoveAll() { Strings.Clear(); }
    void Save(const QString &settingsPath) { SaveFolderHistory(Strings, settingsPath); }
    void Read(const QString &settingsPath) {
        ReadFolderHistory(Strings, settingsPath);
        Normalize();
    }
};

struct CAppState {
    CFastFolders FastFolders;
    CFolderHistory FolderHistory;

    void Save(const QString &settingsPath) {
        FastFolders.Save(settingsPath);
        FolderHistory.Save(settingsPath);
    }
    void Read(const QString &settingsPath) {
        FastFolders.Read(settingsPath);
        FolderHistory.Read(settingsPath);
    }
};
