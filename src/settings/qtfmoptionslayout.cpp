#include "localization/qtlangcontrols.h"
#include "settings/EditPage.h"
#include "settings/FoldersPage.h"
#include "settings/LangPage.h"
#include "settings/MenuPage.h"
#include "settings/SettingsPage.h"
#include "settings/SystemPage.h"
#include "settings/qtfmoptionsdialog.h"
#include "ui/controls/qtdisclosure.h"
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QVBoxLayout *fields(QWidget *widget) {
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);
    return layout;
}
QWidget *memoryPage(CSettingsPage *owner) {
    auto *body = owner->memoryControls();
    delete body->layout(); // The source controls and their connections survive.
    auto *layout = fields(body);
    auto *intro = body->findChild<QLabel *>("extractMemoryLabel");
    QtBindLocalText(
        intro, "SettingsRedesign",
        QT_TRANSLATE_NOOP("SettingsRedesign", "Limit memory when extracting large archives."));
    layout->addWidget(intro);
    intro->setForegroundRole(QPalette::PlaceholderText);
    auto *enable = body->findChild<QCheckBox *>("extractMemorySet");
    enable->setAccessibleName({});
    QtBindLocalText(enable, "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Set a memory limit"));
    layout->addWidget(enable);
    auto *dependent = new QWidget(body);
    auto *values = new QVBoxLayout(dependent);
    values->setContentsMargins(24, 0, 0, 0);
    values->setSpacing(8);
    auto *label = new QLabel(dependent);
    QtBindLocalText(label, "SettingsRedesign", QT_TRANSLATE_NOOP("SettingsRedesign", "Limit, GB"));
    auto *spin = body->findChild<QAbstractSpinBox *>("extractMemorySpin");
    label->setBuddy(spin);
    values->addWidget(label);
    values->addWidget(spin, 0, Qt::AlignLeading);
    auto *ram = body->findChild<QLabel *>("extractMemoryRam");
    const auto ramValues = ram->property("languageArguments").toStringList();
    if (!ramValues.isEmpty())
        QtBindLocalText(ram, "SettingsRedesign",
                        QT_TRANSLATE_NOOP("SettingsRedesign", "This computer: %1 GB"), false,
                        ramValues);
    ram->setVisible(!ramValues.isEmpty());
    ram->setForegroundRole(QPalette::PlaceholderText);
    values->addWidget(ram);
    layout->addWidget(dependent);
    layout->addWidget(body->findChild<QCheckBox *>("largePagesOption"));
    return body;
}
QWidget *listPage(CSettingsPage *owner) {
    auto *body = owner->listControls();
    delete body->layout();
    auto *layout = fields(body);
    for (int group = 0; group < 2; ++group) {
        auto *part = new QWidget(body);
        auto *rows = fields(part);
        rows->setSpacing(8);
        auto *heading = new QLabel(part);
        auto font = heading->font();
        font.setBold(true);
        heading->setFont(font);
        QtBindLocalText(heading, "SettingsRedesign",
                        group == 0 ? QT_TRANSLATE_NOOP("SettingsRedesign", "Display")
                                   : QT_TRANSLATE_NOOP("SettingsRedesign", "Controls"));
        rows->addWidget(heading);
        for (const QString &name :
             group == 0 ? QStringList{"showDotsOption", "realFileIconsOption", "fullRowOption",
                                      "gridOption"}
                        : QStringList{"singleClickOption", "alternativeSelectionOption",
                                      "showSystemMenuOption"})
            rows->addWidget(body->findChild<QCheckBox *>(name));
        layout->addWidget(part);
    }
    return body;
}
void section(QTabWidget *pages, const char *name, const char *title, unsigned owner, QWidget *body,
             const char *intro = nullptr, const char *hint = nullptr) {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 24);
    layout->setSpacing(20);
    auto *heading = new QLabel(page);
    heading->setObjectName("settingsHeading");
    heading->setWordWrap(true);
    auto font = heading->font();
    font.setBold(true);
    if (font.pointSizeF() > 0)
        font.setPointSizeF(font.pointSizeF() * 1.25);
    heading->setFont(font);
    QtBindLocalText(heading, "SettingsRedesign", title);
    layout->addWidget(heading);
    const auto description = [page, layout](const char *text) {
        auto *label = new QLabel(page);
        label->setWordWrap(true);
        label->setForegroundRole(QPalette::PlaceholderText);
        QtBindLocalText(label, "SettingsRedesign", text);
        layout->addWidget(label);
    };
    if (intro)
        description(intro);
    if (body->layout())
        body->layout()->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_MACOS
    // Cocoa's layout item excludes button overhang; the scroll body must also
    // contain that native frame at its last row.
    for (auto *button : body->findChildren<QAbstractButton *>())
        button->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
    body->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    layout->addWidget(body);
    if (hint)
        description(hint);
    layout->addStretch();
    auto *scroll = new QScrollArea;
    scroll->setObjectName(name);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(page);
    page->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setProperty("sourcePageID", owner);
    scroll->setProperty("sectionTitle", title);
    pages->addTab(scroll, QCoreApplication::translate("SettingsRedesign", title));
}
} // namespace

void QtFmOptionsDialog::buildSections() {
    auto *general = new QWidget;
    auto *basic = fields(general);
    basic->addWidget(_settingsPage->appearanceControls());
    _settingsPage->setFocusProxy(nullptr);
    _settingsPage->hide();
    _langPage->layout()->setContentsMargins(0, 0, 0, 0);
    auto *info = _langPage->findChild<QPlainTextEdit *>("languageInfo");
    _langPage->layout()->removeWidget(info);
    auto *metadata = new QWidget;
    auto *metadataLayout = fields(metadata);
    metadataLayout->setContentsMargins(24, 0, 0, 0);
    metadataLayout->addWidget(info);
    info->setMinimumHeight(info->fontMetrics().lineSpacing() * 4);
    _langPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    basic->addWidget(_langPage);
    QtBindLocalText(_langPage->findChild<QLabel *>("languageLabel"), "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Interface language"));
    auto *translation = new QWidget(general);
    auto *translationLayout = fields(translation);
    auto *description = new QLabel(translation);
    description->setObjectName("translationDescription");
    description->setWordWrap(true);
    description->setContentsMargins(24, 0, 0, 0);
    description->setForegroundRole(QPalette::PlaceholderText);
    QtBindLocalText(
        description, "SettingsRedesign",
        QT_TRANSLATE_NOOP("SettingsRedesign", "Information about the installed translation"));
    _translationGroup = new QtDisclosure(metadata, translation, description);
    _translationGroup->setObjectName("translationDetails");
    QtBindLocalText(_translationGroup->button(), "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "About translation"));
    translationLayout->addWidget(_translationGroup);
    basic->addWidget(translation);
    section(_sections, "generalSection", QT_TRANSLATE_NOOP("SettingsRedesign", "General"), 2101,
            general);

    _systemPage->SimplifyPresentation();
    if (_systemPage->IntegrationAvailable())
        section(_sections, "fileOpeningSection",
                QT_TRANSLATE_NOOP("SettingsRedesign", "Opening files"), 2200, _systemPage);
    else
        _systemPage->hide();
    section(_sections, "memorySection", QT_TRANSLATE_NOOP("SettingsRedesign", "Memory"), 2500,
            memoryPage(_settingsPage));
    section(_sections, "temporarySection", QT_TRANSLATE_NOOP("SettingsRedesign", "Temporary files"),
            2400, _foldersPage,
            QT_TRANSLATE_NOOP("SettingsRedesign",
                              "Choose where to keep temporary files when working with archives."));
    section(_sections, "applicationsSection", QT_TRANSLATE_NOOP("SettingsRedesign", "Applications"),
            2103, _editPage,
            QT_TRANSLATE_NOOP("SettingsRedesign",
                              "Applications for viewing, editing and comparing archived files."),
            QT_TRANSLATE_NOOP("SettingsRedesign",
                              "You can specify an application and command-line arguments."));
    section(_sections, "fileListSection", QT_TRANSLATE_NOOP("SettingsRedesign", "File list"), 2500,
            listPage(_settingsPage));
    if (_menuPage->IntegrationSupported())
        section(_sections, "shellMenuSection",
                QT_TRANSLATE_NOOP("SettingsRedesign", "File Explorer commands"), 2300, _menuPage);
    else
        _menuPage->hide();
    _sections->tabBar()->hide();
    _sections->setFocusPolicy(Qt::NoFocus);
    _sections->tabBar()->setFocusPolicy(Qt::NoFocus);
    _sections->setDocumentMode(true);

    _navigationPanel = new QWidget(this);
    _navigationPanel->setObjectName("settingsSidebar");
    auto *navigationLayout = new QVBoxLayout(_navigationPanel);
    navigationLayout->setContentsMargins(10, 16, 10, 20);
    _navigation = new QListWidget(_navigationPanel);
    _navigation->setObjectName("settingsNavigation");
    _navigation->setFrameShape(QFrame::NoFrame);
    _navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _navigation->setWordWrap(true);
    _navigation->setSpacing(4);
    QtBindLocalText(_navigation, "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Settings sections"), true);
    _navigationPanel->setBackgroundRole(QPalette::Window);
    _navigationPanel->setAutoFillBackground(true);
    navigationLayout->addWidget(_navigation);
    _compactNavigationPanel = new QWidget(this);
    auto *compactLayout = new QVBoxLayout(_compactNavigationPanel);
    compactLayout->setContentsMargins(16, 10, 16, 10);
    _compactNavigation = new QListWidget(_compactNavigationPanel);
    compactLayout->addWidget(_compactNavigation);
    _compactNavigation->setObjectName("settingsSectionChoice");
    _compactNavigation->setFrameShape(QFrame::NoFrame);
    _compactNavigation->setFlow(QListView::LeftToRight);
    _compactNavigation->setWrapping(true);
    _compactNavigation->setResizeMode(QListView::Adjust);
    _compactNavigation->setTextElideMode(Qt::ElideNone);
    _compactNavigation->setSpacing(4);
    _compactNavigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QtBindLocalText(_compactNavigation, "SettingsRedesign",
                    QT_TRANSLATE_NOOP("SettingsRedesign", "Settings sections"), true);
    for (int i = 0; i < _sections->count(); ++i) {
        _navigation->addItem(_sections->tabText(i));
        _compactNavigation->addItem(_sections->tabText(i));
    }
    connect(_navigation, &QListWidget::currentRowChanged, _sections, &QTabWidget::setCurrentIndex);
    connect(_compactNavigation, &QListWidget::currentRowChanged, _sections,
            &QTabWidget::setCurrentIndex);
    connect(_sections, &QTabWidget::currentChanged, this, [this](int index) {
        const QSignalBlocker side(_navigation), compact(_compactNavigation);
        _navigation->setCurrentRow(index);
        _compactNavigation->setCurrentRow(index);
    });
    _navigation->setCurrentRow(0);
    _compactNavigation->setCurrentRow(0);
    _compactNavigationPanel->hide();
    _navigationDivider = new QFrame(this);
    _navigationDivider->setFrameShape(QFrame::VLine);
    _frame = new QGridLayout(this);
    _frame->setContentsMargins(0, 0, 0, 0);
    _frame->setSpacing(0);
    _frame->addWidget(_navigationPanel, 0, 0, 2, 1);
    _frame->addWidget(_navigationDivider, 0, 1, 2, 1);
    _frame->addWidget(_sections, 0, 2);
    _footer = new QWidget(this);
    _footer->setObjectName("settingsFooter");
    auto *footer = new QHBoxLayout(_footer);
    footer->setContentsMargins(24, 12, 24, 20);
    footer->setSpacing(16);
    footer->addWidget(_helpButtons);
    footer->addStretch();
    footer->addWidget(_buttons);
    _frame->addWidget(_footer, 1, 2);
    _frame->setColumnStretch(2, 1);
    _frame->setRowStretch(0, 1);
    refreshSectionSummaries();
}

void QtFmOptionsDialog::refreshSectionSummaries() {
    _translationGroup->setSummary(_langPage->IsModified()
                                      ? QCoreApplication::translate("SettingsRedesign", "Changed")
                                      : QString());
}

void QtFmOptionsDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    updateNavigationGeometry();
}

void QtFmOptionsDialog::updateNavigationGeometry() {
    if (!_frame)
        return;
    auto sidebarPalette = palette();
    const QColor background = sidebarPalette.color(QPalette::Window);
    const QColor sidebar = background.darker(background.lightness() < 128 ? 110 : 103);
    sidebarPalette.setColor(QPalette::Window, sidebar);
    sidebarPalette.setColor(QPalette::Base, sidebar);
    _navigationPanel->setPalette(sidebarPalette);
    _navigation->setPalette(sidebarPalette);
    _navigationPanel->setFixedWidth(fontMetrics().averageCharWidth() * 22);
    for (int i = 0; i < _navigation->count(); ++i) {
        const QRect label =
            fontMetrics().boundingRect(QRect(0, 0, _navigationPanel->width() - 36, 10000),
                                       Qt::TextWordWrap, _navigation->item(i)->text());
        _navigation->item(i)->setSizeHint(QSize(0, label.height() + 16));
    }
    const int cellHeight = fontMetrics().height() + 16;
    const int available = qMax(1, width() - 32);
    int rows = 1, used = 0;
    for (int i = 0; i < _compactNavigation->count(); ++i) {
        auto *item = _compactNavigation->item(i);
        const int cellWidth = qMin(available, fontMetrics().horizontalAdvance(item->text()) + 24);
        item->setSizeHint(QSize(cellWidth, cellHeight));
        if (used && used + cellWidth + 8 > available) {
            ++rows;
            used = 0;
        }
        used += cellWidth + 8;
    }
    _compactNavigation->setFixedHeight(rows * (cellHeight + 8));
    const bool compact = width() <= qMax(560, fontMetrics().averageCharWidth() * 80);
    if (compact == _compact)
        return;
    _compact = compact;
    for (auto *widget : {_navigationPanel, static_cast<QWidget *>(_navigationDivider),
                         _compactNavigationPanel, static_cast<QWidget *>(_sections), _footer})
        _frame->removeWidget(widget);
    _navigationPanel->setVisible(!compact);
    _navigationDivider->setVisible(!compact);
    _compactNavigationPanel->setVisible(compact);
    if (compact) {
        _frame->addWidget(_compactNavigationPanel, 0, 0, 1, 3);
        _frame->addWidget(_sections, 1, 0, 1, 3);
        _frame->addWidget(_footer, 2, 0, 1, 3);
    } else {
        _frame->addWidget(_navigationPanel, 0, 0, 2, 1);
        _frame->addWidget(_navigationDivider, 0, 1, 2, 1);
        _frame->addWidget(_sections, 0, 2);
        _frame->addWidget(_footer, 1, 2);
    }
    _frame->setRowStretch(0, compact ? 0 : 1);
    _frame->setRowStretch(1, compact ? 1 : 0);
}

void QtFmOptionsDialog::revealControl(QWidget *control) {
    for (auto *parent = control->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto *disclosure = qobject_cast<QtDisclosure *>(parent))
            disclosure->setExpanded(true);
        if (auto *scroll = qobject_cast<QScrollArea *>(parent)) {
            if (_sections->indexOf(scroll) >= 0)
                _sections->setCurrentWidget(scroll);
            scroll->ensureWidgetVisible(control);
        }
    }
    control->setFocus(Qt::OtherFocusReason);
}
