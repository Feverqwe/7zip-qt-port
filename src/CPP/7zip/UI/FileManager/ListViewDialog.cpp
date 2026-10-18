// Adapted from 7-Zip CPP/7zip/UI/FileManager/ListViewDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt replaces Win32 controls/clipboard/OnSize and activation notifications.
#include "ListViewDialog.h"
#include "EditDialog.h"
#include "ExtractCallback.h"
#include "PropertyName.h"
#include "RegistryUtils.h"
#include "ViewSettings.h"
#include "qtdiagnostictext.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include "qtlistview.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QPushButton>
#include <QVBoxLayout>

static const unsigned kOneStringMaxSize = 1024;
static constexpr int kPropertyIDRole = Qt::UserRole + 14;
static constexpr int kPropertyFallbackRole = Qt::UserRole + 15;
static constexpr int kPropertyHasFallbackRole = Qt::UserRole + 16;
static constexpr int kDiagnosticSourceRole = Qt::UserRole + 17;
static constexpr int kArchiveValueRole = Qt::UserRole + 18;

static UString ListValueText(UString value) {
    if (value.Len() > kOneStringMaxSize) {
        value.DeleteFrom(kOneStringMaxSize);
        value += " ...";
    }
    value.Replace(L"\r\n", L" ");
    value.Replace(L"\n", L" ");
    return value;
}

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
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowTitle(ToQString(Title));
    const QtDialogUnits units(font());
    const QSize margin = units.pixels(QSize(8, 8));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(margin.width(), margin.height(), margin.width(), margin.height());
    layout->setSpacing(margin.height());
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
    FOR_VECTOR(i, Strings) {
        _listView->InsertItem(i, Strings[i]);
        const auto diagnostic = _diagnosticBindings.constFind(i);
        if (diagnostic != _diagnosticBindings.cend()) {
            auto *item = _listView->topLevelItem(int(i));
            item->setData(0, kDiagnosticSourceRole, diagnostic->first);
            item->setData(1, kDiagnosticSourceRole, diagnostic->second);
        }
        const auto binding = _propertyNameBindings.constFind(i);
        if (binding != _propertyNameBindings.cend()) {
            auto *item = _listView->topLevelItem(int(i));
            item->setData(0, kPropertyIDRole, binding->ID);
            item->setData(0, kPropertyFallbackRole, ToQString(binding->Fallback));
            item->setData(0, kPropertyHasFallbackRole, binding->HasFallback);
        }
        const auto archiveValue = _archiveValueBindings.constFind(i);
        if (archiveValue != _archiveValueBindings.cend())
            _listView->topLevelItem(int(i))->setData(1, kArchiveValueRole, *archiveValue);
        if (NumColumns > 1 && i < Values.Size()) {
            _listView->SetSubItem(i, 1, ListValueText(Values[i]));
        }
    }
    if (SelectFirst && Strings.Size() > 0)
        _listView->SetItemState_FocusedSelected(0);
    _listView->SetColumnWidthAuto(0);
    if (NumColumns > 1)
        _listView->SetColumnWidthAuto(1);
    StringsWereChanged = false;
    layout->addWidget(_listView);
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("listViewButtons");
    buttons->layout()->setSpacing(margin.width());
    for (auto *button : buttons->buttons()) {
        button->setFixedWidth(qMax(units.pixels(QSize(64, 0)).width(), button->sizeHint().width()));
        button->setMinimumHeight(units.pixels(QSize(0, 16)).height());
    }
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
    resize(units.pixels(QSize(496, 336)).expandedTo(minimumSizeHint()));
    QtNormalizeDialogSize(*this);
    _listView->setFocus();
    return true;
}

void CListViewDialog::QtBindPropertyName(unsigned row, UInt32 id, const wchar_t *fallback) {
    _propertyNameBindings.insert(
        row, {id, fallback ? UString(fallback) : UString(), fallback != nullptr});
}

void CListViewDialog::QtBindDiagnosticRow(unsigned row, const char *name, const char *value) {
    _diagnosticBindings.insert(row, {name, value});
}

void CListViewDialog::QtBindArchiveDiagnosticValue(unsigned row, const UString &source) {
    _archiveValueBindings.insert(row, ToQString(source));
}

void CListViewDialog::QtBindArchiveFlagsValue(unsigned row, UInt32 flags) {
    _archiveValueBindings.insert(row, QVariant::fromValue(quint32(flags)));
}

void CListViewDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && _listView) {
        for (int i = 0; i < _listView->topLevelItemCount(); ++i) {
            auto *item = _listView->topLevelItem(i);
            const QVariant archiveValue = item->data(1, kArchiveValueRole);
            if (archiveValue.isValid() && unsigned(i) < Values.Size()) {
                const UString value =
                    archiveValue.metaType().id() == QMetaType::UInt
                        ? GetOpenArcErrorMessage(archiveValue.toUInt())
                        : QtArchiveDiagnosticText(ToUString(archiveValue.toString()));
                Values[unsigned(i)] = value;
                item->setText(1, ToQString(ListValueText(value)));
            }
            for (int column = 0; column < 2; ++column) {
                const QByteArray key = item->data(column, kDiagnosticSourceRole).toByteArray();
                if (key.isEmpty())
                    continue;
                const UString text = QtDiagnosticText(key.constData());
                auto &values = column == 0 ? Strings : Values;
                if (unsigned(i) < values.Size())
                    values[unsigned(i)] = text;
                item->setText(column, ToQString(text));
            }
            const QVariant id = item->data(0, kPropertyIDRole);
            if (!id.isValid())
                continue;
            const UString fallback = ToUString(item->data(0, kPropertyFallbackRole).toString());
            const UString text = GetNameOfProperty(
                id.toUInt(),
                item->data(0, kPropertyHasFallbackRole).toBool() ? fallback.Ptr() : nullptr);
            if (unsigned(i) < Strings.Size())
                Strings[unsigned(i)] = text;
            item->setText(0, ToQString(text));
        }
        _listView->SetColumnWidthAuto(0);
        if (NumColumns > 1)
            _listView->SetColumnWidthAuto(1);
    }
    QDialog::changeEvent(event);
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
    if (NumColumns > 1) {
        auto *item = _listView->topLevelItem(int(index));
        const QVariant id = item->data(0, kPropertyIDRole);
        if (id.isValid() && id.toUInt() < 1000)
            QtBindLangText(&dlg, 1000 + id.toUInt(), ToQString(Strings[index]));
    }
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
