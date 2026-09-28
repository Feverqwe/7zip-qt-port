// Adapted from 7-Zip UI/Agent/Agent.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "AgentProxy.h"

struct CProxyItem {
    unsigned DirIndex;
    unsigned Index;
};

class CAgentFolder {
  public:
    CAgentFolder(const CProxyArc &proxy, unsigned dirIndex, bool flatMode);
    void LoadItems();
    unsigned GetNumberOfItems() const;
    CProxyItem GetItem(unsigned index) const;

  private:
    void LoadFolder(unsigned dirIndex);
    const CProxyArc *_proxy;
    unsigned _proxyDirIndex;
    bool _flatMode;
    std::vector<CProxyItem> _items;
};
