#pragma once
#include <QScopeGuard>
#include <QStringList>
#include <UIAutomation.h>
#include <windows.h>
#include <wrl/client.h>

// Inspect only windows belonging to this disposable helper, on its desktop.
inline QStringList NativeWindowDiagnostics(DWORD pid) {
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const auto release = qScopeGuard([&] {
        if (SUCCEEDED(initialized))
            CoUninitialize();
    });
    using Microsoft::WRL::ComPtr;
    ComPtr<IUIAutomation> automation;
    if (FAILED(CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&automation))))
        return {"UI Automation unavailable"};
    struct Inspection {
        DWORD pid;
        IUIAutomation *automation;
        QStringList names;
    } inspection{pid, automation.Get(), {}};
    EnumWindows(
        [](HWND window, LPARAM parameter) -> BOOL {
            auto &context = *reinterpret_cast<Inspection *>(parameter);
            DWORD owner = 0;
            GetWindowThreadProcessId(window, &owner);
            if (owner != context.pid)
                return TRUE;
            wchar_t title[512]{};
            GetWindowTextW(window, title, 512);
            context.names << QString("window visible=%1 title=%2")
                                 .arg(bool(IsWindowVisible(window)))
                                 .arg(QString::fromWCharArray(title));
            ComPtr<IUIAutomationElement> root;
            ComPtr<IUIAutomationCondition> condition;
            ComPtr<IUIAutomationElementArray> elements;
            if (FAILED(context.automation->ElementFromHandle(window, &root)) ||
                FAILED(context.automation->CreateTrueCondition(&condition)) ||
                FAILED(root->FindAll(TreeScope_Descendants, condition.Get(), &elements)))
                return TRUE;
            int length = 0;
            if (FAILED(elements->get_Length(&length)))
                return TRUE;
            for (int i = 0; i < length; ++i) {
                ComPtr<IUIAutomationElement> element;
                if (FAILED(elements->GetElement(i, &element)))
                    continue;
                BSTR name = nullptr;
                if (SUCCEEDED(element->get_CurrentName(&name)) && name)
                    context.names << QString::fromWCharArray(name, int(SysStringLen(name)));
                SysFreeString(name);
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&inspection));
    return inspection.names;
}
