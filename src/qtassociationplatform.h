#pragma once
#include "qtassociationservices.h"
#include <QByteArray>
#include <memory>

struct QtAssociationRecord {
    NRegistryAssoc::CShellExtInfo info;
    QIcon icon;
    QByteArray revision;
    // Extensions can share one native MIME type/UTType mutation domain.
    // Empty means this extension has independent registration state.
    QByteArray scope;
};

// Native service boundary; platform writes are reached only from explicit Apply.
// Read revisions include the native state relevant to the subsequent mutation.
class IQtAssociationPlatform {
  public:
    virtual ~IQtAssociationPlatform() = default;
    virtual bool CanApply(unsigned group) const = 0;
    virtual QString UnavailableReason(unsigned group) const = 0;
    virtual QString ApplyExplanation(unsigned) const { return {}; }
    virtual QString CurrentUserName() const { return {}; }
    virtual UString OwnIconPath() const = 0;
    virtual QIcon OwnIcon() const { return {}; }
    virtual LONG Read(unsigned group, const UString &ext, QtAssociationRecord &record) = 0;
    virtual LONG Write(unsigned group, const UString &ext, bool associate,
                       const QtAssociationRecord &expected, const UString &title,
                       const UString &command, const UString &iconPath, int iconIndex) = 0;
    virtual void NotifyChanged() = 0;
};

std::shared_ptr<IQtAssociationServices>
CreateQtAssociationServices(std::shared_ptr<IQtAssociationPlatform> platform);
std::shared_ptr<IQtAssociationPlatform> CreateNativeAssociationPlatform();
