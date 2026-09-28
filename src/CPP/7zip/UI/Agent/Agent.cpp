// Adapted from 7-Zip CPP/7zip/UI/Agent/Agent.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// COM lifecycle and CProxyArc2 alternate streams are supplied by neither the
// 7zz listing nor this Qt adapter. The CProxyArc folder walk is retained.
#include "Agent.h"

CAgentFolder::CAgentFolder(const CProxyArc &proxy, unsigned dirIndex, bool flatMode)
    : _proxy(&proxy), _proxyDirIndex(dirIndex), _flatMode(flatMode) {}

void CAgentFolder::LoadFolder(unsigned dirIndex) {
    CProxyItem item;
    item.DirIndex = dirIndex;
    const CProxyDir &dir = _proxy->Dirs[dirIndex];
    unsigned i;
    for (i = 0; i < dir.SubDirs.size(); i++) {
        item.Index = i;
        _items.push_back(item);
        LoadFolder(dir.SubDirs[i]);
    }
    const unsigned start = dir.SubDirs.size();
    for (i = 0; i < dir.SubFiles.size(); i++) {
        item.Index = start + i;
        _items.push_back(item);
    }
}

void CAgentFolder::LoadItems() {
    _items.clear();
    if (_flatMode)
        LoadFolder(_proxyDirIndex);
}

unsigned CAgentFolder::GetNumberOfItems() const {
    if (_flatMode)
        return _items.size();
    const CProxyDir &dir = _proxy->Dirs[_proxyDirIndex];
    return dir.SubDirs.size() + dir.SubFiles.size();
}

CProxyItem CAgentFolder::GetItem(unsigned index) const {
    if (_flatMode)
        return _items[index];
    return {_proxyDirIndex, index};
}
