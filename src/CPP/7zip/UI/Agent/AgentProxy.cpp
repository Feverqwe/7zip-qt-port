// Adapted from 7-Zip CPP/7zip/UI/Agent/AgentProxy.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// QString / standard containers replace owned wchar_t arrays and 7-Zip vectors.
// ArchiveBackend metadata replaces IInArchive property access; CRC/raw properties
// and CProxyArc2 alternate streams are not available in the process adapter.
#include "AgentProxy.h"
#include <cstring>

int CProxyArc::FindSubDir(const unsigned *items, unsigned right, const QString &name,
                          unsigned &insertPos) const {
    unsigned left = 0;
    for (;;) {
        if (left == right) {
            insertPos = left;
            return -1;
        }
        const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
        const unsigned dirIndex2 = items[mid];
        const int comp = QString::compare(name, Dirs[dirIndex2].Name, Qt::CaseInsensitive);
        if (comp == 0) {
            insertPos = mid; // not used, but we set it to suppress compiler warnings
            return (int)dirIndex2;
        }
        if (comp < 0)
            right = mid;
        else
            left = mid + 1;
    }
}

int CProxyArc::FindSubDir(unsigned dirIndex, const QString &name) const {
    unsigned insertPos;
    const std::vector<unsigned> &dirs = Dirs[dirIndex].SubDirs;
    return FindSubDir(dirs.data(), dirs.size(), name, insertPos);
}

void CProxyArc::MergeSubDirs(CProxyDir &baseDir) {
    std::vector<unsigned> &dirs2 = baseDir.SubDirs2;
    unsigned i = dirs2.size();
    if (i == 0)
        return;
    std::vector<unsigned> &dirs = baseDir.SubDirs;
    unsigned right = dirs.size();
    unsigned total = right + i;
    dirs.resize(total);
    unsigned *const items = dirs.data();
    do {
        const unsigned index = dirs2[--i];
        unsigned insertPos;
        if (FindSubDir(items, right, Dirs[index].Name, insertPos) != -1)
            throw 1;
        const unsigned numItems = right - insertPos;
        right = insertPos;
        total -= numItems;
        memmove(items + total, items + insertPos, numItems * sizeof(*items));
        items[--total] = index;
    } while (i);
    dirs2.clear();
}

unsigned CProxyArc::AddDir(const unsigned dirIndex, const int arcIndex, const QString &name) {
    CProxyDir &baseDir = Dirs[dirIndex];
    std::vector<unsigned> &dirs = baseDir.SubDirs;
    std::vector<unsigned> &dirs2 = baseDir.SubDirs2;
    {
        const unsigned numSmallDirs = dirs2.size();
        if (numSmallDirs * numSmallDirs > dirs.size())
            MergeSubDirs(baseDir);
    }
    unsigned insertPos;
    int subDirIndex = FindSubDir(dirs.data(), dirs.size(), name, insertPos);
    if (subDirIndex == -1) {
        unsigned insertPos2;
        subDirIndex = FindSubDir(dirs2.data(), dirs2.size(), name, insertPos2);
        if (subDirIndex == -1) {
            std::vector<unsigned> *subDirsWork = &dirs;
            if (dirs.size() - insertPos >= (1 << 12)) {
                subDirsWork = &dirs2;
                insertPos = insertPos2;
            }
            subDirIndex = (int)Dirs.size();
            subDirsWork->insert(subDirsWork->begin() + insertPos, (unsigned)subDirIndex);
            Dirs.emplace_back();
            CProxyDir &item = Dirs.back();
            item.Name = name;
            item.ArcIndex = arcIndex;
            item.ParentDir = (int)dirIndex;
            return (unsigned)subDirIndex;
        }
    }

    CProxyDir &item = Dirs[(unsigned)subDirIndex];
    if (item.ArcIndex == -1)
        item.ArcIndex = arcIndex;
    return (unsigned)subDirIndex;
}

void CProxyArc::CalculateSizes(const QList<ArchiveEntry> &entries) {
    std::vector<unsigned> vec;
    unsigned dirIndex = 0, i = 0;
    for (;;) {
        CProxyDir &dir = Dirs[dirIndex];
        if (i == 0) {
            dir.Size = dir.PackSize = 0;
            dir.NumSubDirs = dir.SubDirs.size();
            dir.NumSubFiles = dir.SubFiles.size();
            for (unsigned k = 0; k < dir.SubFiles.size(); ++k) {
                const unsigned index = dir.SubFiles[k];
                dir.Size += entries[index].size;
                dir.PackSize += entries[index].packedSize;
            }
        }

        if (i == dir.SubDirs.size()) {
            const unsigned num = vec.size();
            if (num < 2)
                return;
            dirIndex = vec[num - 2];
            i = vec[num - 1];
            vec.resize(num - 2);

            CProxyDir &dir2 = Dirs[dirIndex];
            dir2.Size += dir.Size;
            dir2.PackSize += dir.PackSize;
            dir2.NumSubFiles += dir.NumSubFiles;
            dir2.NumSubDirs += dir.NumSubDirs;
            continue;
        }

        vec.push_back(dirIndex);
        vec.push_back(i + 1);
        dirIndex = dir.SubDirs[i];
        i = 0;
    }
}

void CProxyArc::AddRealIndices(const unsigned dirIndex, std::vector<unsigned> &realIndices) const {
    const CProxyDir &dir = Dirs[dirIndex];
    if (dir.IsLeaf())
        realIndices.push_back((unsigned)dir.ArcIndex);
    unsigned i;
    for (i = 0; i < dir.SubDirs.size(); i++)
        AddRealIndices(dir.SubDirs[i], realIndices);
    for (i = 0; i < dir.SubFiles.size(); i++)
        realIndices.push_back(dir.SubFiles[i]);
}

int CProxyArc::GetRealIndex(const unsigned dirIndex, const unsigned index) const {
    const CProxyDir &dir = Dirs[dirIndex];
    const unsigned numDirItems = dir.SubDirs.size();
    if (index < numDirItems) {
        const CProxyDir &f = Dirs[dir.SubDirs[index]];
        if (f.IsLeaf())
            return f.ArcIndex;
        return -1;
    }
    return (int)dir.SubFiles[index - numDirItems];
}

int CProxyArc::FindDir(const QString &path) const {
    if (Dirs.empty())
        return -1;
    unsigned dirIndex = k_Proxy_RootDirIndex;
    if (path.isEmpty())
        return int(dirIndex);
    for (const QString &part : path.split('/')) {
        const int subDir = FindSubDir(dirIndex, part);
        if (subDir < 0)
            return -1;
        dirIndex = unsigned(subDir);
    }
    return int(dirIndex);
}

void CProxyArc::GetDirPath_as_Prefix_from_Base(unsigned dirIndex, QString &prefix,
                                               unsigned baseDirIndex) const {
    prefix.clear();
    if (dirIndex == baseDirIndex)
        return;
    unsigned len = 0;
    unsigned i = dirIndex;
    do {
        const CProxyDir &dir = Dirs[i];
        len += dir.Name.size() + 1;
        i = unsigned(dir.ParentDir);
    } while (i != baseDirIndex);
    prefix.resize(len);
    i = dirIndex;
    do {
        const CProxyDir &dir = Dirs[i];
        prefix[--len] = '/';
        len -= dir.Name.size();
        for (qsizetype n = 0; n < dir.Name.size(); ++n)
            prefix[len + n] = dir.Name[n];
        i = unsigned(dir.ParentDir);
    } while (i != baseDirIndex);
}

QString CProxyArc::GetDirPath_as_Prefix(unsigned dirIndex) const {
    QString s;
    GetDirPath_as_Prefix_from_Base(dirIndex, s, k_Proxy_RootDirIndex);
    return s;
}

void CProxyArc::Load(const QList<ArchiveEntry> &entries) {
    Files.clear();
    Dirs.clear();
    Are_Changed_LongPaths = false;
    Dirs.emplace_back();
    Files.resize(entries.size());
    for (unsigned i = 0; i < unsigned(entries.size()); ++i) {
        QString path = entries[i].path;
        const bool isDir = entries[i].directory;
        if (isDir && path.endsWith('/'))
            path.chop(1);
        unsigned len = path.size();
        unsigned curItem = 0;
        unsigned namePos = 0;
        unsigned numLevels = 0;
        bool is_Changed_LongPath = false;
        if (len >= (1 << 15)) {
            Are_Changed_LongPaths = true;
            is_Changed_LongPath = true;
            path = (isDir ? "[LONG_PATH_DIR_" : "[LONG_PATH_FILE_") + QString::number(i) + ']';
            len = path.size();
        } else
            for (unsigned j = 0; j < len; ++j) {
                if (path[j] == '/') {
                    const unsigned kLevelLimit = 1 << 10;
                    if (numLevels <= kLevelLimit) {
                        QString name;
                        if (numLevels == kLevelLimit) {
                            name = "[LONG_PATH]";
                            Are_Changed_LongPaths = true;
                            is_Changed_LongPath = true;
                        } else
                            name = path.mid(namePos, j - namePos);
                        curItem = AddDir(curItem, -1, name);
                        if (is_Changed_LongPath)
                            Dirs[curItem].Is_Changed_LongPath = true;
                    }
                    namePos = j + 1;
                    ++numLevels;
                }
            }
        CProxyFile &f = Files[i];
        f.Name = path.mid(namePos);
        if (isDir) {
            curItem = AddDir(curItem, int(i), f.Name);
            if (is_Changed_LongPath)
                Dirs[curItem].Is_Changed_LongPath = true;
        } else
            Dirs[curItem].SubFiles.push_back(i);
    }
    for (CProxyDir &dir : Dirs) {
        MergeSubDirs(dir);
        dir.SubDirs2.clear();
    }
    CalculateSizes(entries);
}
