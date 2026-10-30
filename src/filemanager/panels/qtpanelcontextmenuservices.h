#pragma once
#include <QRect>

class QWidget;
class IQtFileMenuServices;
class IQtPanelPluginMenuServices;

// Control coordinates, popup lifetime and dispatch transport for OnContextMenu.
class IQtPanelContextMenuServices {
  public:
    virtual ~IQtPanelContextMenuServices() = default;
    virtual bool IsHeader(QWidget *handle) const = 0;
    virtual bool IsList(QWidget *handle) const = 0;
    virtual void ShowColumnsContextMenu(int x, int y) = 0;
    virtual bool GetItemRect(int index, QRect &rect) const = 0;
    virtual void ClientToScreen(int &x, int &y) const = 0;
    virtual void CreatePopup() = 0;
    virtual IQtFileMenuServices &Menu() = 0;
    virtual IQtPanelPluginMenuServices &Plugins() = 0;
    virtual unsigned Track(int x, int y) = 0;
    virtual void InvokePluginCommand(unsigned id) = 0;
    virtual bool ExecuteFileCommand(unsigned id) = 0;
};
