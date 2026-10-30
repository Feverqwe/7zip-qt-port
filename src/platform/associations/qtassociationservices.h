#pragma once
#include "settings/RegistryAssociations.h"
#include <QIcon>
#include <QString>

// Platform services borrowed by the source System page. Registration is called
// only by an explicit Apply; tests inject a service without system side effects.
class IQtAssociationServices {
  public:
    virtual ~IQtAssociationServices() = default;
    virtual bool CanApply(unsigned group) const = 0;
    virtual bool CanApplyFormat(unsigned group, const UString &) const { return CanApply(group); }
    virtual QString UnavailableReason(unsigned group) const = 0;
    virtual QString ApplyExplanation(unsigned) const { return {}; }
    virtual QString CurrentUserName() const { return {}; }
    virtual bool RefreshNativeStateAfterApply() const { return false; }
    virtual UString OwnIconPath() const = 0;
    virtual QIcon OwnIcon() const { return {}; }
    virtual QIcon AssociationIcon(unsigned, const UString &) const { return {}; }
    virtual bool Read(unsigned group, const UString &ext, NRegistryAssoc::CShellExtInfo &info) = 0;
    virtual LONG Add(unsigned group, const UString &ext,
                     const NRegistryAssoc::CShellExtInfo &expected, const UString &title,
                     const UString &command, const UString &iconPath, int iconIndex) = 0;
    virtual LONG Delete(unsigned group, const UString &ext,
                        const NRegistryAssoc::CShellExtInfo &expected) = 0;
    virtual void NotifyChanged() = 0;
};
