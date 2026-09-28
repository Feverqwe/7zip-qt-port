// Adapted from 7-Zip CPP/7zip/UI/FileManager/ListViewDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt replaces Win32 controls/clipboard/OnSize and activation notifications.
#include "ListViewDialog.h"
#include "EditDialog.h"
#include "RegistryUtils.h"
#include "ViewSettings.h"
#include "qtlistview.h"
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QVBoxLayout>

static const unsigned kOneStringMaxSize = 1024;

static void ListView_GetSelected(CListView &listView, CUIntVector &vector) {
    vector.Clear();
    int index = -1;
    for (;;) {
        index = listView.GetNextSelectedItem(index);
        if (index < 0)
            break;
        vector.Add((unsigned)index);
    }
}

CListViewDialog::CListViewDialog(QWidget *parent) : QDialog(parent) {}

bool CListViewDialog::OnInit() {
    setObjectName("foldersHistoryDialog");
    setWindowTitle(ToQString(Title));
    resize(540, 360);
    auto *layout = new QVBoxLayout(this);
    _listView = new CListView(this);
    _listView->setObjectName(NumColumns > 1 ? "propertiesList" : "foldersHistoryList");
    _listView->setColumnCount((int)NumColumns);
    _listView->setHeaderHidden(NumColumns <= 1);
    _listView->setHeaderLabels(NumColumns > 1 ? QStringList{"", ""} : QStringList{""});
    _listView->setColumnWidth(0, NumColumns > 1 ? 100 : 400);
    if (NumColumns > 1)
        _listView->setColumnWidth(1, 300);
    CFmSettings st;
    st.Load(SettingsPath.isEmpty() ? FmSettingsPath() : SettingsPath);
    if (st.SingleClick)
        _listView->SetSingleClickActivation(true);
    _listView->installEventFilter(this);
    UString s;
    FOR_VECTOR(i, Strings) {
        _listView->InsertItem(i, Strings[i]);
        if (NumColumns > 1 && i < Values.Size()) {
            s = Values[i];
            if (s.Len() > kOneStringMaxSize) {
                s.DeleteFrom(kOneStringMaxSize);
                s += " ...";
            }
            s.Replace(L"\r\n", L" ");
            s.Replace(L"\n", L" ");
            _listView->SetSubItem(i, 1, s);
        }
    }
    if (SelectFirst && Strings.Size() > 0)
        _listView->SetItemState_FocusedSelected(0);
    _listView->SetColumnWidthAuto(0);
    if (NumColumns > 1)
        _listView->SetColumnWidthAuto(1);
    StringsWereChanged = false;
    layout->addWidget(_listView);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("listViewButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CListViewDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (st.SingleClick)
        connect(_listView, &QTreeWidget::itemClicked, this, [this] {
            const auto modifiers = QApplication::keyboardModifiers();
            if (!(modifiers & (Qt::ControlModifier | Qt::MetaModifier | Qt::ShiftModifier)))
                OnEnter();
        });
    else
        connect(_listView, &QTreeWidget::itemDoubleClicked, this, &CListViewDialog::OnEnter);
    _listView->setFocus();
    return true;
}

void CListViewDialog::CopyToClipboard() {
    CUIntVector indexes;
    ListView_GetSelected(*_listView, indexes);
    UString s;
    FOR_VECTOR(i, indexes) {
        unsigned index = indexes[i];
        s += Strings[index];
        if (NumColumns > 1 && index < Values.Size()) {
            const UString &v = Values[index];
            s += ": ";
            s += v;
        }
#ifdef _WIN32
        s += "\r\n";
#else
        s += "\n";
#endif
    }
    QApplication::clipboard()->setText(ToQString(s));
}

void CListViewDialog::ShowItemInfo() {
    CUIntVector indexes;
    ListView_GetSelected(*_listView, indexes);
    if (indexes.Size() != 1)
        return;
    unsigned index = indexes[0];
    CEditDialog dlg(this);
    if (NumColumns == 1)
        dlg.Text = Strings[index];
    else {
        dlg.Title = Strings[index];
        if (index < Values.Size())
            dlg.Text = Values[index];
    }
#ifdef _WIN32
    if (dlg.Text.Find(L'\r') < 0)
        dlg.Text.Replace(L"\n", L"\r\n");
#endif
    dlg.OnInit();
    dlg.setObjectName("listItemInfoDialog");
    dlg.exec();
}

void CListViewDialog::DeleteItems() {
    for (;;) {
        const int index = _listView->GetNextSelectedItem(-1);
        if (index < 0)
            break;
        StringsWereChanged = true;
        _listView->DeleteItem((unsigned)index);
        if ((unsigned)index < Strings.Size())
            Strings.Delete((unsigned)index);
        if ((unsigned)index < Values.Size())
            Values.Delete((unsigned)index);
    }
    const int focusedIndex = _listView->GetFocusedItem();
    if (focusedIndex >= 0)
        _listView->SetItemState_FocusedSelected(focusedIndex);
    _listView->SetColumnWidthAuto(0);
}

void CListViewDialog::OnEnter() {
    if (QApplication::keyboardModifiers().testFlag(Qt::AltModifier) || NumColumns > 1) {
        ShowItemInfo();
        return;
    }
    OnOK();
}

bool CListViewDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == _listView && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        const auto modifiers = key->modifiers();
        switch (key->key()) {
        case Qt::Key_Delete:
            if (!DeleteIsAllowed)
                return false;
            DeleteItems();
            return true;
        case Qt::Key_A:
            if (modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier)) {
                _listView->selectAll();
                return true;
            }
            break;
        case Qt::Key_Insert:
        case Qt::Key_C:
            if (modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier)) {
                CopyToClipboard();
                return true;
            }
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            OnEnter();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CListViewDialog::OnOK() {
    FocusedItemIndex = _listView->GetFocusedItem();
    QDialog::accept();
}

void CListViewDialog::accept() { OnOK(); }
