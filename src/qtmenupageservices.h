#pragma once
#include "Common/MyString.h"
#include "Common/MyWindows.h"
class QWidget;

// Module/registration/error boundary for source MenuPage CShellDll state.
class IQtMenuPageServices {
  public:
    virtual ~IQtMenuPageServices() = default;
    virtual bool CommandsAvailable() const = 0;
    virtual bool Is64Bit() const = 0;
    virtual bool OtherBitnessAvailable() const = 0;
    virtual FString GetModuleDirPrefix() const = 0;
    virtual const char *GetDllName(unsigned index) const = 0;
    virtual bool DoesFileExist(const FString &path) const = 0;
    virtual bool CheckContextMenuHandler(const UString &path, UInt32 wow) = 0;
    virtual LONG SetContextMenuHandler(bool setMode, const UString &path, UInt32 wow) = 0;
    virtual void ShowMenuErrorMessage(LONG error, QWidget *parent) = 0;
};
