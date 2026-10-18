#include "CPP/7zip/UI/FileManager/EditPage.h"
#include "CPP/7zip/UI/FileManager/FoldersPage.h"
#include "CPP/7zip/UI/FileManager/LangPage.h"
#include "CPP/7zip/UI/FileManager/MenuPage.h"
#include "CPP/7zip/UI/FileManager/SettingsPage.h"
#include "CPP/7zip/UI/FileManager/SystemPage.h"
#include "qtdialoglayout.h"
#include "qtdisclosure.h"
#include "qtfmoptionsdialog.h"
#include "qtlangcontrols.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QScrollArea>
#include <QStyle>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QVBoxLayout *fields(QWidget *widget) {
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    return layout;
}
QtDisclosure *group(QWidget *content, const char *name, const char *title, QVBoxLayout *layout) {
    // Native layout items can exclude the outer focus/shadow padding of a
    // push button. Keep that padding inside the disclosed content's bounds.
    content->layout()->setContentsMargins(
        0, 0, 0,
        qMax(0, content->style()->pixelMetric(QStyle::PM_LayoutBottomMargin, nullptr, content)));
    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *disclosure = new QtDisclosure(content);
    disclosure->setObjectName(name);
    disclosure->findChild<QLabel *>("disclosureSummary")
        ->setForegroundRole(QPalette::PlaceholderText);
    QtBindLocalText(disclosure->button(), "SettingsRedesign", title);
    layout->addWidget(disclosure);
    return disclosure;
}
QScrollArea *section(QTabWidget *tabs, const char *name, const char *title, QWidget *body) {
    auto *scroll = new QScrollArea;
    scroll->setObjectName(name);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(body);
    body->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);
    tabs->addTab(scroll, QCoreApplication::translate("SettingsRedesign", title));
    scroll->setProperty("sectionTitle", title);
    return scroll;
}
QString title(QAbstractButton *button) { return button->text().remove('&'); }
QString modifiedSummary(QStringList values, bool changed) {
    if (values.isEmpty())
        values << QCoreApplication::translate("SettingsRedesign", "Standard settings");
    if (changed)
        values.prepend(QCoreApplication::translate("SettingsRedesign", "Changed"));
    return values.join(QStringLiteral(" · "));
}
} // namespace

void QtFmOptionsDialog::buildSections() {
    auto *general = new QWidget;
    auto *basic = fields(general);
    basic->addWidget(_settingsPage->appearanceControls());
    _settingsPage->setFocusProxy(nullptr);
    _settingsPage->hide(); // State/apply owner; its controls live in two sections.
    _langPage->layout()->setContentsMargins(0, 0, 0, 0);
    auto *info = _langPage->findChild<QPlainTextEdit *>("languageInfo");
    _langPage->layout()->removeWidget(info);
    auto *metadata = new QWidget;
    fields(metadata)->addWidget(info);
    info->setMinimumHeight(info->fontMetrics().lineSpacing() * 4);
    _langPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    basic->addWidget(_langPage);
    group(metadata, "translationDetails",
          QT_TRANSLATE_NOOP("SettingsRedesign", "About translation"), basic);
    basic->addStretch();
    section(_sections, "generalSection", QT_TRANSLATE_NOOP("SettingsRedesign", "General"), general);

    const bool associationsAvailable = _systemPage->IntegrationAvailable();
    const bool menuAvailable = _menuPage->IntegrationSupported();
    auto *integration = new QWidget;
    auto *system = fields(integration);
    _systemPage->SimplifyPresentation();
    _systemPage->setVisible(associationsAvailable);
    system->addWidget(_systemPage);
    if (menuAvailable)
        _menuGroup = group(_menuPage, "shellMenuDetails",
                           QT_TRANSLATE_NOOP("SettingsRedesign", "File Explorer commands"), system);
    else
        _menuPage->hide();
    system->addStretch();
    if (associationsAvailable || menuAvailable)
        section(_sections, "integrationSection",
                QT_TRANSLATE_NOOP("SettingsRedesign", "Integration"), integration);
    else {
        integration->setParent(this);
        integration->hide();
    }

    auto *advanced = new QWidget;
    auto *extra = fields(advanced);
    _memoryGroup = group(_settingsPage->memoryControls(), "memoryDetails",
                         QT_TRANSLATE_NOOP("SettingsRedesign", "Memory for extraction"), extra);
    _foldersGroup = group(_foldersPage, "workFolderDetails",
                          QT_TRANSLATE_NOOP("SettingsRedesign", "Temporary files"), extra);
    _editGroup = group(_editPage, "externalAppsDetails",
                       QT_TRANSLATE_NOOP("SettingsRedesign", "External applications"), extra);
    _listGroup = group(_settingsPage->listControls(), "listBehaviorDetails",
                       QT_TRANSLATE_NOOP("SettingsRedesign", "Familiar behavior"), extra);
    extra->addStretch();
    section(_sections, "advancedSection", QT_TRANSLATE_NOOP("SettingsRedesign", "Advanced"),
            advanced);
    auto *layout = QtCreateDialogLayout(*this, *_buttons);
    layout->addWidget(_sections);
    _sections->setCurrentIndex(0);
    refreshSectionSummaries();
}

void QtFmOptionsDialog::refreshSectionSummaries() {
    QStringList memory;
    if (findChild<QCheckBox *>("extractMemorySet")->isChecked())
        memory << QCoreApplication::translate("SettingsRedesign", "Memory limit: %1 GB")
                      .arg(findChild<QLineEdit *>("extractMemoryEdit")->text());
    if (auto *large = findChild<QCheckBox *>("largePagesOption");
        large->isEnabled() && large->isChecked())
        memory << title(large);
    _memoryGroup->setSummary(modifiedSummary(memory, _settingsPage->IsMemoryModified()));
    QStringList compatibility;
    for (auto *option : _settingsPage->listControls()->findChildren<QCheckBox *>())
        if (option->isChecked())
            compatibility << title(option);
    _listGroup->setSummary(modifiedSummary(compatibility, _settingsPage->IsListModified()));
    QStringList folders;
    for (auto *option : _foldersPage->findChildren<QRadioButton *>())
        if (option->isChecked())
            folders << title(option);
    if (auto *path = _foldersPage->findChild<QLineEdit *>("workPath"); path->isEnabled())
        folders << path->text();
    if (auto *removable = _foldersPage->findChild<QCheckBox *>("workRemovableOnly");
        removable->isEnabled() && removable->isChecked())
        folders << title(removable);
    _foldersGroup->setSummary(modifiedSummary(folders, _foldersPage->IsModified()));
    QStringList apps;
    for (auto *command : _editPage->findChildren<QLineEdit *>())
        if (!command->text().isEmpty())
            apps << command->text();
    _editGroup->setSummary(modifiedSummary(apps, _editPage->IsModified()));
    if (_menuGroup) {
        QStringList menu;
        for (auto *option : _menuPage->findChildren<QCheckBox *>())
            if (!option->isHidden() && option->isChecked())
                menu << title(option);
        auto *commands = _menuPage->findChild<QListWidget *>("menuItems");
        int count = 0;
        for (int i = 0; i < commands->count(); ++i)
            count += commands->item(i)->checkState() == Qt::Checked;
        menu << QCoreApplication::translate("SettingsRedesign", "%n commands", nullptr, count);
        _menuGroup->setSummary(modifiedSummary(menu, _menuPage->IsModified()));
    }
}

void QtFmOptionsDialog::revealControl(QWidget *control) {
    for (auto *parent = control->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto *disclosure = qobject_cast<QtDisclosure *>(parent))
            disclosure->setExpanded(true);
        if (auto *scroll = qobject_cast<QScrollArea *>(parent)) {
            _sections->setCurrentWidget(scroll);
            scroll->ensureWidgetVisible(control);
        }
    }
    control->setFocus(Qt::OtherFocusReason);
}
