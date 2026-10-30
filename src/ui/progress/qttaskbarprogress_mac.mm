#include "ui/progress/qttaskbarprogress.h"
#import <AppKit/AppKit.h>
#include <QString>
#include <algorithm>
#include <cerrno>

// Keep MyWindows.h out of Objective-C++: its Win32 BOOL typedef conflicts
// with Apple's Objective-C BOOL. Native 7zTypes supplies the HRESULT ABI.
namespace {
constexpr HRESULT S_OK = 0;
}

class CQtTaskbarProgress::CImpl {
  public:
    NSDockTile *Tile = nil;
    NSString *Original = nil;
    NSString *Applied = nil;
    TBPFLAG State = TBPF_NOPROGRESS;
    UInt64 Completed = 0;
    UInt64 Total = 0;
    bool OwnsBadge = false;

    HRESULT Check() const {
        if (![NSThread isMainThread])
            return HRESULT_FROM_WIN32(EPERM);
        return Tile ? S_OK : HRESULT_FROM_WIN32(ENOTSUP);
    }
    void Restore() {
        // Another owner may have replaced our badge since the last update.
        if (OwnsBadge && (Tile.badgeLabel == Applied || [Tile.badgeLabel isEqualToString:Applied]))
            Tile.badgeLabel = Original;
        OwnsBadge = false;
    }
    ~CImpl() {
        if ([NSThread isMainThread])
            Restore();
        [Applied release];
        [Original release];
        [Tile release];
    }
    HRESULT Display() {
        if (State == TBPF_NOPROGRESS) {
            Restore();
            return S_OK;
        }
        QString label;
        if (State == TBPF_PAUSED)
            label = QStringLiteral("Ⅱ ");
        else if (State == TBPF_ERROR)
            label = QStringLiteral("! ");
        if (State == TBPF_INDETERMINATE || Total == 0)
            label += QStringLiteral("…");
        else {
            const auto percent = UInt64(100.0L * std::min(Completed, Total) / Total);
            label += QString::number(qulonglong(percent)) + QLatin1Char('%');
        }
        const QByteArray utf8 = label.toUtf8();
        NSString *text = [[NSString alloc] initWithBytes:utf8.constData()
                                                  length:std::size_t(utf8.size())
                                                encoding:NSUTF8StringEncoding];
        if (!text)
            return E_OUTOFMEMORY;
        Tile.badgeLabel = text;
        [Applied release];
        Applied = text;
        OwnsBadge = true;
        return S_OK;
    }
};

CQtTaskbarProgress::CQtTaskbarProgress() : _impl(new CImpl) {}
CQtTaskbarProgress::~CQtTaskbarProgress() = default;

HRESULT CQtTaskbarProgress::HrInit() {
    if (![NSThread isMainThread])
        return HRESULT_FROM_WIN32(EPERM);
    if (_impl->Tile)
        return S_OK;
    // Do not create an application or advertise a capability on an offscreen
    // Qt platform that has no Cocoa application instance.
    NSDockTile *tile = NSApp.dockTile;
    if (!tile)
        return HRESULT_FROM_WIN32(ENOTSUP);
    _impl->Tile = [tile retain];
    _impl->Original = [tile.badgeLabel copy];
    return S_OK;
}
HRESULT CQtTaskbarProgress::SetProgressState(QWidget *window, TBPFLAG state) {
    if (!window || (state != TBPF_NOPROGRESS && state != TBPF_INDETERMINATE &&
                    state != TBPF_NORMAL && state != TBPF_ERROR && state != TBPF_PAUSED))
        return E_INVALIDARG;
    const HRESULT result = _impl->Check();
    if (result != S_OK)
        return result;
    _impl->State = state;
    return _impl->Display();
}
HRESULT CQtTaskbarProgress::SetProgressValue(QWidget *window, UInt64 completed, UInt64 total) {
    if (!window || total == 0)
        return E_INVALIDARG;
    const HRESULT result = _impl->Check();
    if (result != S_OK)
        return result;
    _impl->Completed = completed;
    _impl->Total = total;
    // SetProgressValue clears indeterminate/hidden, retaining blocking states
    // like native ITaskbarList3. Source paused/error selection stays in dialog.
    if (_impl->State != TBPF_PAUSED && _impl->State != TBPF_ERROR)
        _impl->State = TBPF_NORMAL;
    return _impl->Display();
}
