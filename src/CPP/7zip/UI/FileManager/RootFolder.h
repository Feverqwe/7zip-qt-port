// Adapted from 7-Zip CPP/7zip/UI/FileManager/RootFolder.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.

#ifndef ZIP7_INC_ROOT_FOLDER_H
#define ZIP7_INC_ROOT_FOLDER_H

#include "Common/MyCom.h"
#include "Common/MyString.h"

#include "7zip/UI/FileManager/IFolder.h"

const unsigned kNumRootFolderItems_Max = 4;

Z7_CLASS_IMP_NOQIB_2(CRootFolder, IFolderFolder, IFolderGetSystemIconIndex)
UString _names[kNumRootFolderItems_Max];
int _iconIndices[kNumRootFolderItems_Max];

public:
void Init();
}
;

#endif
