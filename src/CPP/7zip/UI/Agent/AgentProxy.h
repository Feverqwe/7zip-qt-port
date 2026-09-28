// Adapted from 7-Zip UI/Agent/AgentProxy.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "archivebackend.h"
#include <deque>
#include <vector>

inline constexpr unsigned k_Proxy_RootDirIndex = 0;

struct CProxyFile {
    QString Name;
};

struct CProxyDir {
    QString Name;
    int ArcIndex = -1;
    int ParentDir = -1;
    std::vector<unsigned> SubDirs;
    std::vector<unsigned> SubDirs2;
    std::vector<unsigned> SubFiles;
    quint64 Size = 0;
    quint64 PackSize = 0;
    quint32 NumSubDirs = 0;
    quint32 NumSubFiles = 0;
    bool Is_Changed_LongPath = false;
    bool IsLeaf() const { return ArcIndex != -1; }
};

class CProxyArc {
  public:
    std::deque<CProxyDir> Dirs;
    std::vector<CProxyFile> Files;
    bool Are_Changed_LongPaths = false;

    void Load(const QList<ArchiveEntry> &entries);
    int FindSubDir(unsigned dirIndex, const QString &name) const;
    int FindDir(const QString &path) const;
    QString GetDirPath_as_Prefix(unsigned dirIndex) const;
    void GetDirPath_as_Prefix_from_Base(unsigned dirIndex, QString &prefix,
                                        unsigned baseDirIndex) const;
    void AddRealIndices(unsigned dirIndex, std::vector<unsigned> &realIndices) const;
    int GetRealIndex(unsigned dirIndex, unsigned index) const;

  private:
    int FindSubDir(const unsigned *items, unsigned right, const QString &name,
                   unsigned &insertPos) const;
    void MergeSubDirs(CProxyDir &baseDir);
    unsigned AddDir(unsigned dirIndex, int arcIndex, const QString &name);
    void CalculateSizes(const QList<ArchiveEntry> &entries);
};
