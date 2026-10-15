#include "qtarchivesourcerename.h"
#include "7zip/Archive/Common/ItemNameUtils.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "CPP/7zip/UI/Common/UpdateCallback.h"

HRESULT ReadSourceRenamePaths(IArchiveUpdateCallback *callback,
                              const QList<SourceRenamedItem> &renamed, QStringList &paths) {
    if (!callback || renamed.isEmpty())
        return E_INVALIDARG;
    QString oldRoot, newRoot;
    if (const auto *source = dynamic_cast<const CArchiveUpdateCallback *>(callback);
        source && source->SourceRenamePathsDefined) {
        UString oldPath = source->SourceRenameOldPath;
        UString newPath = source->SourceRenameNewPath;
        NArchive::NItemName::ReplaceSlashes_OsToUnix(oldPath);
        NArchive::NItemName::ReplaceSlashes_OsToUnix(newPath);
        oldRoot = QString::fromWCharArray(oldPath.Ptr(), oldPath.Len());
        newRoot = QString::fromWCharArray(newPath.Ptr(), newPath.Len());
    } else {
        const QStringList oldParts = renamed.first().OldPath.split('/');
        const QStringList newParts = renamed.first().NewPath.split('/');
        if (oldParts.size() != newParts.size())
            return E_NOTIMPL;
        int changed = -1;
        for (int i = 0; i < oldParts.size(); ++i)
            if (oldParts[i] != newParts[i]) {
                if (changed != -1)
                    return E_NOTIMPL;
                changed = i;
            }
        if (changed < 0)
            return E_NOTIMPL;
        oldRoot = oldParts.mid(0, changed + 1).join('/');
        newRoot = newParts.mid(0, changed + 1).join('/');
    }
    // Validate the literal CLI transport with the original rename matcher,
    // rather than another Qt implementation of prefix/case/path-separator rules.
    CRenamePair pair;
    pair.OldName = UString(oldRoot.toStdWString().c_str());
    pair.NewName = UString(newRoot.toStdWString().c_str());
    pair.WildcardParsing = false;
    if (!pair.Prepare())
        return E_NOTIMPL;
    for (const auto &item : renamed) {
        UString requested;
        if (!pair.GetNewPath(item.Directory, UString(item.OldPath.toStdWString().c_str()),
                             requested) ||
            QString::fromWCharArray(requested.Ptr(), requested.Len()) != item.NewPath)
            return E_NOTIMPL;
    }
    paths = {oldRoot, newRoot};
    return S_OK;
}
