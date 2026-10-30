#pragma once
#include "Common/MyCom.h"
#include "settings/SystemPage.h"

namespace OptionsPageFixture {
class COptionsManager final : public IFolderManager, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderManager)
  public:
    UString Extensions = UString(L"zip");
    Z7_COM7F_IMF(OpenFolderFile(IInStream *, const wchar_t *, const wchar_t *, IFolderFolder **,
                                IProgress *))
    override { return E_NOTIMPL; }
    Z7_COM7F_IMF(GetExtensions(BSTR *extensions)) override {
        return StringToBstr(Extensions, extensions);
    }
    Z7_COM7F_IMF(GetIconPath(const wchar_t *, BSTR *path, Int32 *index)) override {
        *index = 0;
        return StringToBstr(UString(L"/fixture/sevenzip"), path);
    }
};
class COptionsAssociations final : public IQtAssociationServices {
  public:
    int writes = 0;
    int notifications = 0;
    LONG failure = 0;
    bool available[2] = {true, true};
    bool otherHandler = false;
    bool CanApply(unsigned group) const override { return group < 2 && available[group]; }
    QString UnavailableReason(unsigned) const override { return {}; }
    UString OwnIconPath() const override { return UString(L"/fixture/sevenzip"); }
    bool Read(unsigned, const UString &, NRegistryAssoc::CShellExtInfo &info) override {
        info = NRegistryAssoc::CShellExtInfo();
        if (otherHandler)
            info.ProgramKey = CSysString("Other archive application");
        return true;
    }
    LONG Add(unsigned, const UString &, const NRegistryAssoc::CShellExtInfo &, const UString &,
             const UString &, const UString &, int) override {
        ++writes;
        return failure;
    }
    LONG Delete(unsigned, const UString &, const NRegistryAssoc::CShellExtInfo &) override {
        ++writes;
        return failure;
    }
    void NotifyChanged() override { ++notifications; }
};
} // namespace OptionsPageFixture
