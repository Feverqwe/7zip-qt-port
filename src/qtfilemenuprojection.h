#pragma once
#include "qtfilemenuservices.h"
#include <QList>

class IQtDialogScreenServices;

// A menu-independent projection of CFileMenu::Load, shared by command checks
// and Qt menu presentation. Platform registry/screen inputs are explicit.
class QtFileMenuProjection final : public IQtFileMenuServices {
  public:
    struct Item {
        QtFileMenuItem source;
        bool enabled = true;
    };
    QList<QtFileMenuItem> sourceItems;
    QList<Item> items;
    UString diffPath;
    UString versionControlPath;
    bool normalScreen = true;
    const IQtDialogScreenServices *screenServices = nullptr;
    bool IsEnabled(unsigned id) const;
    void ReadRegDiff(UString &path) const override;
    void ReadReg_VerCtrlPath(UString &path) const override;
    bool IsDialogSizeOK(unsigned width, unsigned height) const override;
    bool GetItem(unsigned index, QtFileMenuItem &item) const override;
    void CopyPopMenu_IfRequired(QtFileMenuItem &item) override;
    bool InsertItem(unsigned position, const QtFileMenuItem &item) override;
    void EnableItem(unsigned position, bool enabled) override;
    bool AppendItem(unsigned id, const UString &text) override;
    void RemoveAllItemsFrom(unsigned position) override;
};
