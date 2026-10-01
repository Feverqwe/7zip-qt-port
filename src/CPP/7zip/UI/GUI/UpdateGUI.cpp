// Adapted from 7-Zip CPP/7zip/UI/GUI/UpdateGUI.cpp at 0766b73.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "UpdateGUI.h"

// UpdateGUI.cpp::g_UpdateMode_Pairs. The Win32 dialog's transport is replaced
// by CreateArchiveDialog; the original action-set mapping remains here.
struct C_UpdateMode_ToAction_Pair {
    NCompressDialog::NUpdateMode::EEnum UpdateMode;
    const NUpdateArchive::CActionSet *ActionSet;
};

static const C_UpdateMode_ToAction_Pair g_UpdateMode_Pairs[] = {
    {NCompressDialog::NUpdateMode::kAdd, &NUpdateArchive::k_ActionSet_Add},
    {NCompressDialog::NUpdateMode::kUpdate, &NUpdateArchive::k_ActionSet_Update},
    {NCompressDialog::NUpdateMode::kFresh, &NUpdateArchive::k_ActionSet_Fresh},
    {NCompressDialog::NUpdateMode::kSync, &NUpdateArchive::k_ActionSet_Sync}};

static int FindUpdateMode(NCompressDialog::NUpdateMode::EEnum mode) {
    for (unsigned i = 0; i < sizeof(g_UpdateMode_Pairs) / sizeof(g_UpdateMode_Pairs[0]); i++)
        if (mode == g_UpdateMode_Pairs[i].UpdateMode)
            return int(i);
    return -1;
}

const NUpdateArchive::CActionSet *NCompressDialog::FindUpdateActionSet(NUpdateMode::EEnum mode) {
    const int index = FindUpdateMode(mode);
    return index < 0 ? nullptr : g_UpdateMode_Pairs[index].ActionSet;
}
