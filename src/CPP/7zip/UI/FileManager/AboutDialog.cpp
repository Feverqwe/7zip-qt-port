// Adapted from 7-Zip AboutDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original initialization, help topic and homepage command; Qt presentation/services.
// Modified 2026-10-18: independent application identity and upstream attribution layout.
#include "AboutDialog.h"
#include "../../../../../third_party/7zip/C/CpuArch.h"
#include "../Common/LoadCodecs.h"
#include "7zip/MyVersion.h"
#include "HelpUtils.h"
#include "qtdialoglayout.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include "qtsemanticcolors.h"
#include <QDesktopServices>
#include <QEvent>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QUrl>
#include <QVBoxLayout>

static const UInt32 kLangIDs[] = {IDT_ABOUT_INFO};
#define kHomePageURL "https://www.7-zip.org/"
#define kHelpTopic "start.htm"

static void StyleAboutLink(QPushButton &button) {
    button.setFlat(true);
    button.setCursor(Qt::PointingHandCursor);
    QtSemanticColors::styleLink(button);
}

CAboutDialog::CAboutDialog(QWidget *parent, const CCodecs *codecs)
    : QDialog(parent), codecs_(codecs) {
    setObjectName("aboutDialog");
    auto *footerWidget = new QWidget(this);
    footerWidget->setObjectName("aboutActions");
    auto *layout = QtCreateDialogLayout(*this, *footerWidget);
    const int gap = qMax(8, fontMetrics().height() / 2);
    layout->setSpacing(gap);
    auto *heading = new QHBoxLayout;
    auto *title = new QLabel("Sevenzip", this);
    title->setObjectName("aboutApplicationName");
    QFont titleFont = title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.8);
    titleFont.setBold(true);
    title->setFont(titleFont);
    heading->addWidget(title);
    heading->addStretch();
    auto *application = new QLabel(SEVENZIP_DESKTOP_VERSION, this);
    application->setObjectName("aboutApplicationVersion");
    application->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    application->setMargin(gap / 2);
    QtSemanticColors::styleSurface(*application, 5);
    heading->addWidget(application, 0, Qt::AlignVCenter);
    layout->addLayout(heading);
    auto *description = new QLabel(this);
    description->setObjectName("aboutDescription");
    description->setWordWrap(true);
    description->setTextFormat(Qt::PlainText);
    QtBindLocalText(
        description, "AboutDialog",
        QT_TRANSLATE_NOOP("AboutDialog", "An independent file manager based on 7‑Zip."));
    layout->addWidget(description);
    QFont captionFont = font();
    captionFont.setPointSizeF(captionFont.pointSizeF() * 0.9);
    layout->addSpacing(gap);
    auto *upstream = new QFrame(this);
    upstream->setObjectName("aboutUpstream");
    upstream->setAccessibleName("7-Zip");
    QtSemanticColors::styleSurface(*upstream, 10);
    auto *upstreamLayout = new QVBoxLayout(upstream);
    upstreamLayout->setContentsMargins(2 * gap, 2 * gap, 2 * gap, 2 * gap);
    upstreamLayout->setSpacing(gap / 2);
    for (unsigned id : {IDT_ABOUT_VERSION, IDT_ABOUT_DATE, IDT_ABOUT_INFO}) {
        auto *label = new QLabel(upstream);
        label->setObjectName(QString("aboutText%1").arg(id));
        label->setProperty("sourceControlID", id);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        if (id == IDT_ABOUT_VERSION) {
            QFont versionFont = font();
            versionFont.setBold(true);
            label->setFont(versionFont);
        } else
            label->setFont(captionFont);
        if (id == IDT_ABOUT_INFO)
            label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        upstreamLayout->addWidget(label);
    }
    auto *copyright = new QLabel(MY_COPYRIGHT, upstream);
    copyright->setObjectName("aboutUpstreamCopyright");
    copyright->setWordWrap(true);
    copyright->setFont(captionFont);
    upstreamLayout->insertWidget(2, copyright);
    auto *license = new QPushButton(upstream);
    license->setObjectName("aboutLicense");
    QtBindLocalText(license, "AboutDialog", QT_TRANSLATE_NOOP("AboutDialog", "7-Zip license"));
    StyleAboutLink(*license);
    connect(license, &QPushButton::clicked, this,
            [this] { ShowHelpWindow("general/license.htm", this); });
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok, this);
    buttons->SetAcceptText("DialogRedesign", QT_TRANSLATE_NOOP("DialogRedesign", "Close"));
    auto *homepage = new QPushButton("www.7-zip.org", upstream);
    homepage->setObjectName("aboutHomepage");
    StyleAboutLink(*homepage);
    homepage->setProperty("sourceControlID", IDB_ABOUT_HOMEPAGE);
    connect(homepage, &QPushButton::clicked, this,
            [this, homepage] { OnButtonClicked(IDB_ABOUT_HOMEPAGE, homepage); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    auto *ok = buttons->button(QDialogButtonBox::Ok);
    auto *upstreamLinks = new QHBoxLayout;
    upstreamLinks->addWidget(license);
    upstreamLinks->addWidget(homepage);
    upstreamLayout->addSpacing(gap / 2);
    upstreamLayout->addLayout(upstreamLinks);
    layout->addWidget(upstream);
    layout->addStretch();
    auto *project = new QPushButton(footerWidget);
    project->setObjectName("aboutProjectHomepage");
    QtBindLocalText(project, "AboutDialog", QT_TRANSLATE_NOOP("AboutDialog", "Project website"));
    StyleAboutLink(*project);
    connect(project, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(QUrl("https://github.com/Feverqwe/7zip-qt-port")); });
    auto *footer = new QHBoxLayout(footerWidget);
    footer->setContentsMargins(0, 0, 0, 0);
    footer->setSpacing(QtDialogUnits(font()).pixels(QSize(8, 0)).width());
    footer->addWidget(project);
    footer->addStretch();
    footer->addWidget(buttons);
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, &CAboutDialog::OnHelp);
    OnInit();
    ok->setDefault(true);
    ok->setFocus();
    // The wrapping label lets the layout derive its height from the active translation.
    const int width = QtDialogUnits(font()).pixels(QSize(160, 0)).width();
    const QMargins margins = this->layout()->contentsMargins();
    description->setMinimumWidth(qMax(1, width - margins.left() - margins.right()));
    adjustSize();
}

void CAboutDialog::UpdateMinimumHeight() {
    if (layout() && layout()->hasHeightForWidth()) {
        layout()->invalidate();
        setMinimumHeight(layout()->totalHeightForWidth(width()));
    }
}

void CAboutDialog::changeEvent(QEvent *event) {
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange || event->type() == QEvent::FontChange)
        UpdateMinimumHeight();
}

void CAboutDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    UpdateMinimumHeight();
}

bool CAboutDialog::OnInit() {
    if (codecs_) {
        UString s;
        codecs_->GetCodecsErrorMessage(s);
        if (!s.IsEmpty())
            QMessageBox::critical(parentWidget(), "7-Zip", QString::fromWCharArray(s.Ptr()));
    }
    QtBindLocalText(this, "AboutDialog", QT_TRANSLATE_NOOP("AboutDialog", "About Sevenzip"));
    for (UInt32 id : kLangIDs)
        QtBindResourceText(findChild<QLabel *>(QString("aboutText%1").arg(id)), id,
                           "7-Zip is free software");
    QtBindLocalText(findChild<QLabel *>(QString("aboutText%1").arg(IDT_ABOUT_VERSION)),
                    "AboutDialog", QT_TRANSLATE_NOOP("AboutDialog", "Based on 7-Zip %1"), false,
                    {MY_VERSION_CPU});
    QtBindLocalText(findChild<QLabel *>(QString("aboutText%1").arg(IDT_ABOUT_DATE)), "AboutDialog",
                    QT_TRANSLATE_NOOP("AboutDialog", "7-Zip release: %1"), false, {MY_DATE});
    // Qt positions the modal child relative to its parent.
    return true;
}

void CAboutDialog::OnHelp() { ShowHelpWindow(kHelpTopic, this); }

bool CAboutDialog::OnButtonClicked(unsigned buttonID, QWidget *buttonHWND) {
    Q_UNUSED(buttonHWND);
    const char *url;
    switch (buttonID) {
    case IDB_ABOUT_HOMEPAGE:
        url = kHomePageURL;
        break;
    default:
        return false;
    }
    QDesktopServices::openUrl(QUrl(url));
    return true;
}
