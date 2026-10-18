#include "qtlinkcontrols.h"
#include "7zip/UI/FileManager/LinkDialogRes.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtlangcontrols.h"
#include "qtlinkservices.h"
#include "qtmodalresult.h"
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

QWidget *CQtLinkWindow::Register(unsigned id, const char *name, QWidget *view,
                                 const QString &caption) {
    controls_.insert(id, view);
    view->setObjectName(QString::fromLatin1(name));
    if (!caption.isEmpty()) {
        if (id == IDCANCEL)
            QtBindStandardResourceText(view, id, caption);
        else
            QtBindResourceText(view, id, caption);
    }
    return view;
}
void CQtLinkWindow::AddPathRow(QVBoxLayout *layout, bool to) {
    const QtDialogUnits units(font());
    const unsigned labelID = to ? IDT_LINK_PATH_TO : IDT_LINK_PATH_FROM;
    const unsigned comboID = to ? IDC_LINK_PATH_TO : IDC_LINK_PATH_FROM;
    const unsigned buttonID = to ? IDB_LINK_PATH_TO : IDB_LINK_PATH_FROM;
    auto *label = new QLabel(this);
    Register(labelID, to ? "IDT_LINK_PATH_TO" : "IDT_LINK_PATH_FROM", label,
             to ? "Link to:" : "Link from:");
    label->setMinimumHeight(units.pixels(QSize(0, 8)).height());
    layout->addWidget(label);
    layout->addSpacing(units.pixels(QSize(0, 2)).height());
    auto *row = new QHBoxLayout;
    row->setSpacing(units.pixels(QSize(8, 0)).width());
    auto *combo = new QComboBox(this);
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);
    combo->setMinimumContentsLength(12);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    Register(comboID, to ? "IDC_LINK_PATH_TO" : "IDC_LINK_PATH_FROM", combo);
    label->setBuddy(combo);
    row->addWidget(combo, 1);
    auto *browse = new QPushButton("...", this);
    browse->setFixedWidth(units.pixels(QSize(20, 0)).width());
    browse->setMinimumHeight(units.pixels(QSize(0, 16)).height());
    Register(buttonID, to ? "IDB_LINK_PATH_TO" : "IDB_LINK_PATH_FROM", browse);
    connect(browse, &QPushButton::clicked, this,
            [this, buttonID, browse] { OnButtonClicked(buttonID, browse); });
    row->addWidget(browse);
    layout->addLayout(row);
}
QWidget *CQtLinkWindow::BuildLinkTypes() {
    const QtDialogUnits units(font());
    auto *group = new QGroupBox(this);
    group->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    Register(IDG_LINK_TYPE, "IDG_LINK_TYPE", group, "Link Type");
    auto *types = new QVBoxLayout(group);
    const unsigned ids[] = {IDR_LINK_TYPE_HARD, IDR_LINK_TYPE_SYM_FILE, IDR_LINK_TYPE_SYM_DIR,
                            IDR_LINK_TYPE_JUNCTION, IDR_LINK_TYPE_WSL};
    const char *names[] = {"IDR_LINK_TYPE_HARD", "IDR_LINK_TYPE_SYM_FILE", "IDR_LINK_TYPE_SYM_DIR",
                           "IDR_LINK_TYPE_JUNCTION", "IDR_LINK_TYPE_WSL"};
    const char *captions[] = {"Hard Link", "File Symbolic Link", "Directory Symbolic Link",
                              "Directory Junction", "WSL"};
    for (int i = 0; i < 5; ++i) {
        auto *radio = new QRadioButton(group);
        radio->setMinimumHeight(units.pixels(QSize(0, 10)).height());
        // Keep translated captions inside the compact viewport at native font metrics.
        radio->setProperty("languageWrapWidth", 220);
        Register(ids[i], names[i], radio, QString::fromLatin1(captions[i]));
#ifndef _WIN32
        if (i >= 3) {
            radio->setEnabled(false);
            QtBindLocalToolTip(radio, "This link type is available only on Windows.",
                               "Этот тип ссылки доступен только в Windows.");
        }
#endif
        types->addWidget(radio);
    }
    return group;
}
void CQtLinkWindow::BuildControls() {
    const QtDialogUnits units(font());
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setObjectName("IDD_LINK");
    QtBindResourceText(this, IDD_LINK, "Link");
    auto *outer = new QVBoxLayout(this);
    const QSize margin = units.pixels(QSize(8, 8));
    outer->setContentsMargins(margin.width(), margin.height(), margin.width(), margin.height());
    outer->setSpacing(margin.height());
    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, style()->pixelMetric(QStyle::PM_ScrollBarExtent), 0);
    layout->setSpacing(0);
    AddPathRow(layout, false);
    layout->addSpacing(units.pixels(QSize(0, 6)).height());
    AddPathRow(layout, true);
    layout->addSpacing(units.pixels(QSize(0, 4)).height());
    auto *current = new QLabel(this);
    current->setWordWrap(true);
    current->setTextFormat(Qt::PlainText);
    current->hide();
    Register(IDT_LINK_PATH_TO_CUR, "IDT_LINK_PATH_TO_CUR", current);
    layout->addWidget(current);
    layout->addSpacing(units.pixels(QSize(0, 6)).height());
    layout->addWidget(BuildLinkTypes());
    layout->addStretch();
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("linkControlsScroll");
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);
    scroll->setMinimumHeight(content->minimumSizeHint().height());
    outer->addWidget(scroll, 1);
    auto *footer = new QHBoxLayout;
    footer->setSpacing(margin.width());
    footer->addStretch();
    auto *link = new QPushButton(this);
    Register(IDB_LINK_LINK, "IDB_LINK_LINK", link, "Link");
    link->setDefault(true);
    connect(link, &QPushButton::clicked, this,
            [this, link] { OnButtonClicked(IDB_LINK_LINK, link); });
    footer->addWidget(link);
    auto *cancel = new QPushButton(this);
    Register(2, "IDCANCEL", cancel, "Cancel");
    for (auto *button : {link, cancel}) {
        button->setFixedWidth(qMax(units.pixels(QSize(64, 0)).width(), button->sizeHint().width()));
        button->setMinimumHeight(units.pixels(QSize(0, 16)).height());
    }
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(cancel);
    outer->addLayout(footer);
    resize(QSize(qMax(360, sizeHint().width()), sizeHint().height()).expandedTo(minimumSizeHint()));
}
void CQtLinkWindow::SetItemText(unsigned id, const UString &text) {
    if (auto *label = qobject_cast<QLabel *>(GetItem(id))) {
        label->setText(QString::fromWCharArray(text.Ptr()));
        if (id == IDT_LINK_PATH_TO_CUR)
            label->setVisible(!text.IsEmpty());
    }
}
void CQtLinkWindow::CheckRadioButton(unsigned first, unsigned last, unsigned checked) {
    for (unsigned id = first; id <= last; ++id)
        if (auto *radio = qobject_cast<QRadioButton *>(GetItem(id)))
            radio->setChecked(id == checked);
}
bool CQtLinkWindow::IsButtonCheckedBool(unsigned id) const {
    const auto *radio = qobject_cast<QRadioButton *>(GetItem(id));
    return radio && radio->isEnabled() && radio->isChecked();
}
void CQtLinkWindow::NormalizeSize() { QtNormalizeDialogSize(*this); }
qintptr CQtLinkWindow::Create(unsigned, QWidget *parent) {
    QtInitializeLinkPlatform();
    if (parent)
        setParent(parent, windowFlags());
    BuildControls();
    OnInit();
    return exec();
}
void CQtLinkWindow::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    OnSize(0, event->size().width(), event->size().height());
}
