#pragma once
#include "Common/MyCom.h"
#include <shlobj.h>

namespace NativeMenuFixture {
struct ProviderReport {
    unsigned initialized = 0;
    bool destroyed = false;
    unsigned measured = 0;
    unsigned drawn = 0;
    UINT drawState = 0;
    ULONG_PTR itemData = 0;
    unsigned messages3 = 0;
    bool refuseMeasure = false;
    unsigned menuChars = 0;
    bool callbackBitmap = false;
};
class BitmapProvider final : public IContextMenu3, public CMyUnknownImp {
    ProviderReport &_report;
    const bool _messages3;
    Z7_COM_QI_BEGIN
    Z7_COM_QI_ENTRY_UNKNOWN(IContextMenu3)
    Z7_COM_QI_ENTRY(IContextMenu)
    Z7_COM_QI_ENTRY(IContextMenu2)
    else if (_messages3 && iid == IID_IContextMenu3) {
        *outObject = static_cast<IContextMenu3 *>(this);
    }
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    HBITMAP bitmap = nullptr;
    explicit BitmapProvider(ProviderReport &report, bool messages3 = false,
                            DWORD pixel = 0xffdc280a, bool bottomUp = false);
    ~BitmapProvider();
    HRESULT STDMETHODCALLTYPE QueryContextMenu(HMENU, UINT, UINT, UINT, UINT) override;
    HRESULT STDMETHODCALLTYPE InvokeCommand(LPCMINVOKECOMMANDINFO) override;
    HRESULT STDMETHODCALLTYPE GetCommandString(UINT_PTR, UINT, UINT *, LPSTR, UINT) override;
    HRESULT STDMETHODCALLTYPE HandleMenuMsg(UINT message, WPARAM wParam, LPARAM lParam) override;
    HRESULT STDMETHODCALLTYPE HandleMenuMsg2(UINT message, WPARAM wParam, LPARAM lParam,
                                             LRESULT *result) override;
};
} // namespace NativeMenuFixture
