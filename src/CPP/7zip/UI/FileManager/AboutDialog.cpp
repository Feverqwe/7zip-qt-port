// Adapted from 7-Zip AboutDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original initialization, help topic and homepage command; Qt presentation/services.
// Modified 2026-10-18: independent application identity and upstream attribution layout.
#include "AboutDialog.h"
#include "../../../../../third_party/7zip/C/CpuArch.h"
#include "../Common/LoadCodecs.h"
#include "7zip/MyVersion.h"
#include "HelpUtils.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include "qtsemanticcolors.h"
#include <QDesktopServices>
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

static QSize IdentityMinimumSize(const QLabel &application, int width, const QString &english,
                                 const QString &russian) {
    QLabel measure;
    measure.setFont(application.font());
    measure.setTextFormat(Qt::PlainText);
    measure.setWordWrap(true);
    measure.setText(english);
    const int englishHeight = measure.heightForWidth(width);
    measure.setText(russian);
    return QSize(width, qMax(englishHeight, measure.heightForWidth(width)));
}

static void StyleAboutLink(QPushButton &button) {
    button.setFlat(true);
    button.setCursor(Qt::PointingHandCursor);
    QtSemanticColors::styleLink(button);
}

CAboutDialog::CAboutDialog(QWidget *parent, const CCodecs *codecs)
    : QDialog(parent), codecs_(codecs) {
    setObjectName("aboutDialog");
    auto *layout = new QVBoxLayout(this);
    const int gap = qMax(8, fontMetrics().height() / 2);
    layout->setContentsMargins(3 * gap, 3 * gap, 3 * gap, 3 * gap);
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
    const QString english = "An independent file manager based on 7‑Zip.";
    const QString russian = "Независимый файловый менеджер на основе 7‑Zip.";
    QtBindLocalText(description, english, russian);
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
    QtBindLocalText(license, "7-Zip license", "Лицензия 7-Zip");
    StyleAboutLink(*license);
    connect(license, &QPushButton::clicked, this,
            [this] { ShowHelpWindow("general/license.htm", this); });
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok, this);
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
    layout->addSpacing(gap);
    layout->addStretch();
    auto *project = new QPushButton(this);
    project->setObjectName("aboutProjectHomepage");
    QtBindLocalText(project, "Project website", "Сайт проекта");
    StyleAboutLink(*project);
    connect(project, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(QUrl("https://github.com/Feverqwe/7zip-qt-port")); });
    auto *footer = new QHBoxLayout;
    footer->setSpacing(QtDialogUnits(font()).pixels(QSize(8, 0)).width());
    footer->addWidget(project);
    footer->addStretch();
    footer->addWidget(buttons);
    layout->addLayout(footer);
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, &CAboutDialog::OnHelp);
    OnInit();
    ok->setDefault(true);
    ok->setFocus();
    // Fit both supported translations before initial display or a live switch.
    const int width = QtDialogUnits(font()).pixels(QSize(160, 0)).width();
    const QMargins margins = layout->contentsMargins();
    description->setMinimumSize(IdentityMinimumSize(
        *description, qMax(1, width - margins.left() - margins.right()), english, russian));
    adjustSize();
}

bool CAboutDialog::OnInit() {
    if (codecs_) {
        UString s;
        codecs_->GetCodecsErrorMessage(s);
        if (!s.IsEmpty())
            QMessageBox::critical(parentWidget(), "7-Zip", QString::fromWCharArray(s.Ptr()));
    }
    QtBindLocalText(this, "About Sevenzip", "О программе Sevenzip");
    for (UInt32 id : kLangIDs)
        QtBindResourceText(findChild<QLabel *>(QString("aboutText%1").arg(id)), id,
                           "7-Zip is free software");
    QtBindLocalText(findChild<QLabel *>(QString("aboutText%1").arg(IDT_ABOUT_VERSION)),
                    "Based on 7-Zip " MY_VERSION_CPU, "На основе 7-Zip " MY_VERSION_CPU);
    QtBindLocalText(findChild<QLabel *>(QString("aboutText%1").arg(IDT_ABOUT_DATE)),
                    "7-Zip release: " MY_DATE, "Дата выпуска 7-Zip: " MY_DATE);
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
