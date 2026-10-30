// Adapted from 7-Zip CPP/7zip/UI/FileManager/ClassDefs.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Portable folder/stream/progress and archive/password callback GUIDs used here.
#include "Common/MyWindows.h"

#include "Common/MyInitGuid.h"

#include "7zip/Archive/IArchive.h"
#include "7zip/ICoder.h"
#include "7zip/IPassword.h"
#include "7zip/UI/FileManager/IFolder.h"
#include "archive/agent/Agent.h"
#include "archive/agent/IFolderArchive.h"
