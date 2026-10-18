#include "CPP/7zip/UI/FileManager/SystemPage.h"
#include "qtdisclosure.h"
#include "qtlangcontrols.h"
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

bool CSystemPage::IntegrationAvailable() const {
    return _services && (_services->CanApply(0) || _services->CanApply(1));
}

void CSystemPage::SelectFormat(unsigned index, bool selected) {
    if ((_items[index].Pair[0].State == kExtState_7Zip) == selected)
        return;
    CUIntVector indices;
    indices.Add(index);
    // Use the source Clear/Other/Own cycle, including restoration of an Other
    // handler. A checkbox asking for Own can cross the intermediate Other state.
    ChangeState(0, indices);
    if (selected && _items[index].Pair[0].State != kExtState_7Zip)
        ChangeState(0, indices);
}

void CSystemPage::RefreshFormatChoices() {
    auto *other = findChild<QtDisclosure *>("otherFormats");
    int selected = 0;
    bool otherChanged = false;
    bool scopeChanged = false;
    for (int i = 0; i < _formatChoices.size(); ++i) {
        auto *choice = _formatChoices[i];
        const QSignalBlocker blocked(choice);
        const auto &ext = _extDB.Exts[unsigned(i)].Ext;
        choice->setChecked(_items[unsigned(i)].Pair[0].State == kExtState_7Zip);
        choice->setEnabled(_services && _services->CanApplyFormat(0, ext));
        choice->setToolTip(_listView->topLevelItem(i)->toolTip(1));
        if (other && other->isAncestorOf(choice)) {
            selected += choice->isChecked();
            otherChanged = otherChanged || _items[unsigned(i)].Pair[0].State !=
                                               _items[unsigned(i)].Pair[0].OldState;
        }
        for (const auto &state : _items[unsigned(i)].Pair)
            scopeChanged = scopeChanged || state.State != state.OldState;
    }
    if (other)
        other->setSummary((otherChanged
                               ? QCoreApplication::translate("SettingsRedesign", "Changed") + " · "
                               : QString()) +
                          QCoreApplication::translate("SettingsRedesign", "%n selected formats",
                                                      nullptr, selected));
    if (auto *scope = findChild<QtDisclosure *>("associationDetails"))
        scope->setSummary(scopeChanged ? QCoreApplication::translate("SettingsRedesign", "Changed")
                                       : QString());
}

void CSystemPage::SimplifyPresentation() {
    auto *details = new QWidget(this);
    details->setLayout(layout());
    details->layout()->setContentsMargins(0, 0, 0, 0);
    _label->hide();
    _listView->setColumnHidden(2, !_services || !_services->CanApply(1));
    _listView->setMinimumHeight(fontMetrics().lineSpacing() * 8);
    QtBindLocalText(_current, "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Change selected"));
    QtBindLocalText(_all, "SettingsRedesign", QT_TRANSLATE_NOOP("SettingsRedesign", "All users"));
    auto *fields = new QVBoxLayout(this);
    fields->setContentsMargins(0, 0, 0, 0);
    fields->setSpacing(8);
    auto *heading = new QLabel(this);
    heading->setWordWrap(true);
    QtBindLocalText(heading, "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Open archives with Sevenzip"));
    fields->addWidget(heading);
    auto *other = new QWidget(this);
    auto *otherFields = new QVBoxLayout(other);
    otherFields->setContentsMargins(0, 0, 0, 0);
    otherFields->setSpacing(8);
    FOR_VECTOR(i, _extDB.Exts) {
        const QString ext = QString::fromWCharArray(_extDB.Exts[i].Ext.Ptr());
        auto *choice = new QCheckBox('.' + ext, this);
        choice->setObjectName("association-" + ext);
        choice->setAccessibleName(choice->text());
        _formatChoices.append(choice);
        if (ext.compare("7z", Qt::CaseInsensitive) == 0 ||
            ext.compare("zip", Qt::CaseInsensitive) == 0)
            fields->addWidget(choice);
        else
            otherFields->addWidget(choice);
        connect(choice, &QCheckBox::clicked, this,
                [this, i](bool selected) { SelectFormat(i, selected); });
    }
    auto *otherGroup = new QtDisclosure(other, this);
    otherGroup->setObjectName("otherFormats");
    QtBindLocalText(otherGroup->button(), "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Other formats"));
    fields->addWidget(otherGroup);
    otherGroup->setVisible(otherFields->count() > 0);
    auto *scope = new QtDisclosure(details, this);
    scope->setObjectName("associationDetails");
    QtBindLocalText(scope->button(), "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Advanced associations"));
    fields->addWidget(scope);
    RefreshFormatChoices();
}
