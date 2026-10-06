// Adapted from 7-Zip AboutDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original initialization, help topic and homepage command; Qt presentation/services.
#include "AboutDialog.h"
#include "../../../../../third_party/7zip/C/CpuArch.h"
#include "../Common/LoadCodecs.h"
#include "7zip/MyVersion.h"
#include "HelpUtils.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QDesktopServices>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QShortcut>
#include <QUrl>
#include <QVBoxLayout>

static const UInt32 kLangIDs[] = {IDT_ABOUT_INFO};
#define kHomePageURL "https://www.7-zip.org/"
#define kHelpTopic "start.htm"
#define LLL_(quote) L##quote
#define LLL(quote) LLL_(quote)

static void InitAboutResources() { Q_INIT_RESOURCE(upstream_about); }

CAboutDialog::CAboutDialog(QWidget *parent, const CCodecs *codecs)
    : QDialog(parent), codecs_(codecs) {
    InitAboutResources();
    setObjectName("aboutDialog");
    resize(360, 330);
    auto *layout = new QVBoxLayout(this);
    auto *logo = new QLabel(this);
    logo->setObjectName("aboutLogo");
    logo->setPixmap(QPixmap(":/upstream/7zip/About/7zipLogo.ico")
                        .scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logo);
    for (unsigned id : {IDT_ABOUT_VERSION, IDT_ABOUT_DATE, IDT_ABOUT_INFO}) {
        auto *label = new QLabel(this);
        label->setObjectName(QString("aboutText%1").arg(id));
        label->setProperty("sourceControlID", id);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        layout->addWidget(label);
    }
    auto *copyright = new QLabel(MY_COPYRIGHT, this);
    copyright->setWordWrap(true);
    layout->insertWidget(3, copyright);
    auto *application = new QLabel(this);
    application->setObjectName("aboutApplicationVersion");
    application->setWordWrap(true);
    QtBindLocalText(application,
                    "Sevenzip Desktop " SEVENZIP_DESKTOP_VERSION
                    "\n7-Zip source port; uses the official 7-Zip engine.",
                    "Sevenzip Desktop " SEVENZIP_DESKTOP_VERSION
                    "\nПорт исходников 7-Zip; использует официальный движок 7-Zip.");
    layout->addWidget(application);
    layout->addStretch();
    auto *license = new QPushButton(this);
    license->setObjectName("aboutLicense");
    QtBindLocalText(license, "7-Zip license", "Лицензия 7-Zip");
    layout->addWidget(license);
    connect(license, &QPushButton::clicked, this,
            [this] { ShowHelpWindow("general/license.htm", this); });
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok, this);
    auto *homepage = buttons->addButton("www.7-zip.org", QDialogButtonBox::ActionRole);
    homepage->setObjectName("aboutHomepage");
    homepage->setProperty("sourceControlID", IDB_ABOUT_HOMEPAGE);
    connect(homepage, &QPushButton::clicked, this,
            [this, homepage] { OnButtonClicked(IDB_ABOUT_HOMEPAGE, homepage); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, &CAboutDialog::OnHelp);
    OnInit();
}

void CAboutDialog::SetItemText(unsigned id, const UString &text) {
    findChild<QLabel *>(QString("aboutText%1").arg(id))
        ->setText(QString::fromWCharArray(text.Ptr()));
}

bool CAboutDialog::OnInit() {
    if (codecs_) {
        UString s;
        codecs_->GetCodecsErrorMessage(s);
        if (!s.IsEmpty())
            QMessageBox::critical(parentWidget(), "7-Zip", QString::fromWCharArray(s.Ptr()));
    }
    QtBindResourceText(this, IDD_ABOUT, "About 7-Zip");
    for (UInt32 id : kLangIDs)
        QtBindResourceText(findChild<QLabel *>(QString("aboutText%1").arg(id)), id,
                           "7-Zip is free software");
    SetItemText(IDT_ABOUT_VERSION, UString("7-Zip " MY_VERSION_CPU));
    SetItemText(IDT_ABOUT_DATE, LLL(MY_DATE));
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
