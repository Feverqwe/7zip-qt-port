// Adapted from 7-Zip CPP/7zip/UI/FileManager/ListViewDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Single-column history use. Qt replaces Win32 controls/clipboard/OnSize;
// two-column Values and Settings-driven hover activation are not ported yet.
#include "ListViewDialog.h"
#include "ViewSettings.h"
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QListWidget>
#include <QTextEdit>
#include <QVBoxLayout>

static void ListView_GetSelected(QListWidget &listView, CUIntVector &vector) {
    vector.Clear();
    for (int index = 0; index < listView.count(); ++index)
        if (listView.item(index)->isSelected())
            vector.Add((unsigned)index);
}

CListViewDialog::CListViewDialog(QWidget *parent) : QDialog(parent) {}

bool CListViewDialog::OnInit() {
    setObjectName("foldersHistoryDialog");
    setWindowTitle(ToQString(Title));
    resize(540, 360);
    auto *layout = new QVBoxLayout(this);
    _listView = new QListWidget(this);
    _listView->setObjectName("foldersHistoryList");
    _listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _listView->setTextElideMode(Qt::ElideNone);
    _listView->installEventFilter(this);
    FOR_VECTOR(i, Strings)
    _listView->addItem(ToQString(Strings[i]));
    if (SelectFirst && Strings.Size() > 0)
        _listView->setCurrentRow(0);
    StringsWereChanged = false;
    layout->addWidget(_listView);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("listViewButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CListViewDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(_listView, &QListWidget::itemDoubleClicked, this, &CListViewDialog::OnEnter);
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
    QDialog dlg(this);
    dlg.setObjectName("listItemInfoDialog");
    dlg.resize(480, 240);
    auto *layout = new QVBoxLayout(&dlg);
    auto *text = new QTextEdit(&dlg);
    text->setReadOnly(true);
    text->setPlainText(ToQString(Strings[index]));
    layout->addWidget(text);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
}

void CListViewDialog::DeleteItems() {
    for (;;) {
        CUIntVector indexes;
        ListView_GetSelected(*_listView, indexes);
        if (indexes.IsEmpty())
            break;
        const unsigned index = indexes[0];
        StringsWereChanged = true;
        delete _listView->takeItem((int)index);
        if (index < Strings.Size())
            Strings.Delete(index);
    }
    const int focusedIndex = _listView->currentRow();
    if (focusedIndex >= 0)
        _listView->setCurrentRow(focusedIndex, QItemSelectionModel::ClearAndSelect);
}

void CListViewDialog::OnEnter() {
    if (QApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
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
    FocusedItemIndex = _listView->currentRow();
    QDialog::accept();
}

void CListViewDialog::accept() { OnOK(); }
