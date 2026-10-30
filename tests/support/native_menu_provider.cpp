#include "support/native_menu_provider.h"

namespace NativeMenuFixture {
BitmapProvider::BitmapProvider(ProviderReport &report, bool messages3, DWORD pixel, bool bottomUp)
    : _report(report), _messages3(messages3) {
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = 2;
    info.bmiHeader.biHeight = bottomUp ? 2 : -2;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void *pixels = nullptr;
    bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap)
        for (int i = 0; i < 4; ++i)
            static_cast<DWORD *>(pixels)[i] = pixel;
}
BitmapProvider::~BitmapProvider() {
    DeleteObject(bitmap);
    _report.destroyed = true;
}
HRESULT STDMETHODCALLTYPE BitmapProvider::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT) {
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE BitmapProvider::InvokeCommand(LPCMINVOKECOMMANDINFO) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE BitmapProvider::GetCommandString(UINT_PTR, UINT, UINT *, LPSTR, UINT) {
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE BitmapProvider::HandleMenuMsg(UINT message, WPARAM wParam,
                                                        LPARAM lParam) {
    if (message == WM_INITMENUPOPUP) {
        ++_report.initialized;
        return AppendMenuW(HMENU(wParam), MF_STRING, 103, L"Lazy data 空") ? S_OK : E_FAIL;
    }
    if (message == WM_MEASUREITEM) {
        if (_report.refuseMeasure)
            return E_NOTIMPL;
        auto &measure = *reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
        if (measure.CtlType != ODT_MENU || measure.itemID != 104)
            return E_INVALIDARG;
        ++_report.measured;
        _report.itemData = measure.itemData;
        measure.itemWidth = _report.callbackBitmap ? 2 : 140;
        measure.itemHeight = _report.callbackBitmap ? 2 : 32;
        return S_OK;
    }
    if (message == WM_DRAWITEM) {
        auto &draw = *reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
        if (draw.CtlType != ODT_MENU || draw.itemID != 104 || !IsMenu(HMENU(draw.hwndItem)))
            return E_INVALIDARG;
        ++_report.drawn;
        _report.drawState = draw.itemState;
        _report.itemData = draw.itemData;
        const HBRUSH brush = CreateSolidBrush(RGB(12, 80, 160));
        FillRect(draw.hDC, &draw.rcItem, brush);
        DeleteObject(brush);
        return S_OK;
    }
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE BitmapProvider::HandleMenuMsg2(UINT message, WPARAM wParam, LPARAM lParam,
                                                         LRESULT *result) {
    ++_report.messages3;
    if (message == WM_MENUCHAR) {
        ++_report.menuChars;
        if (LOWORD(wParam) == L'z' && HIWORD(wParam) == MF_POPUP && IsMenu(HMENU(lParam))) {
            *result = MAKELRESULT(0, MNC_EXECUTE);
            return S_OK;
        }
        return E_NOTIMPL;
    }
    *result = 0;
    return HandleMenuMsg(message, wParam, lParam);
}
} // namespace NativeMenuFixture
