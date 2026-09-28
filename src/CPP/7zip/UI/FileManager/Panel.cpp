// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "Panel.h"
#include <QEvent>
#include <QWidget>

void CPanel::SetFocusToList() {
    if (_listView)
        _listView->setFocus();
}

void CPanel::SetFocusToLastRememberedItem() {
    if (_lastFocusedIsList)
        SetFocusToList();
    else if (_headerComboBox)
        _headerComboBox->setFocus();
}

// Presentation binding replaces the two Win32 control handles. Switching the
// displayed list must not reset the last focused control (OnCreate defaults).
void CPanel::AttachPresentation(QWidget *list, QWidget *address) {
    if (_listView)
        _listView->removeEventFilter(this);
    if (_headerComboBox)
        _headerComboBox->removeEventFilter(this);
    _listView = list;
    _headerComboBox = address;
    if (_listView)
        _listView->installEventFilter(this);
    if (_headerComboBox)
        _headerComboBox->installEventFilter(this);
}

bool CPanel::eventFilter(QObject *watched, QEvent *event) {
    // CMyListView WM_SETFOCUS and OnNotifyComboBox CBEN_BEGINEDIT.
    if (event->type() == QEvent::FocusIn) {
        if (watched == _listView)
            _lastFocusedIsList = true;
        else if (watched == _headerComboBox)
            _lastFocusedIsList = false;
    }
    return QObject::eventFilter(watched, event);
}
