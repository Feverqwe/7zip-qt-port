// Adapted from 7-Zip AboutDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original initialization, help topic and homepage command; Qt presentation/services.
#include "AboutDialog.h"
#include "../../../../../third_party/7zip/C/CpuArch.h"
#include "../Common/LoadCodecs.h"
#include "7zip/MyVersion.h"
#include "HelpUtils.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QDesktopServices>
#include <QHBoxLayout>
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

CAboutDialog::CAboutDialog(QWidget *parent, const CCodecs *codecs)
    : QDialog(parent), codecs_(codecs) {
    InitAboutResources();
    setObjectName("aboutDialog");
    auto *layout = new QVBoxLayout(this);
    auto *logo = new QLabel(this);
    logo->setObjectName("aboutLogo");
    logo->setPixmap(QPixmap(":/upstream/7zip/About/7zipLogo.ico"));
    logo->setMinimumSize(logo->pixmap().deviceIndependentSize().toSize());
    layout->addWidget(logo);
    for (unsigned id : {IDT_ABOUT_VERSION, IDT_ABOUT_DATE, IDT_ABOUT_INFO}) {
        auto *label = new QLabel(this);
        label->setObjectName(QString("aboutText%1").arg(id));
        label->setProperty("sourceControlID", id);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        if (id == IDT_ABOUT_INFO) {
            label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            label->setMinimumHeight(QtDialogUnits(font()).pixels(QSize(0, 42)).height());
        }
        layout->addWidget(label);
    }
    auto *copyright = new QLabel(MY_COPYRIGHT, this);
    copyright->setWordWrap(true);
    layout->insertWidget(3, copyright);
    auto *application = new QLabel(this);
    application->setObjectName("aboutApplicationVersion");
    application->setWordWrap(true);
    application->setTextFormat(Qt::PlainText);
    const QString english = "Sevenzip " SEVENZIP_DESKTOP_DISPLAY_VERSION
                            "\nBuilt from 7-Zip source code; uses the official 7-Zip engine.";
    const QString russian = "Sevenzip " SEVENZIP_DESKTOP_DISPLAY_VERSION
                            "\nПостроен на исходниках 7-Zip; использует официальный движок 7-Zip.";
    QtBindLocalText(application, english, russian);
    layout->addWidget(application);
    layout->addStretch();
    auto *license = new QPushButton(this);
    license->setObjectName("aboutLicense");
    QtBindLocalText(license, "7-Zip license", "Лицензия 7-Zip");
    layout->addWidget(license);
    connect(license, &QPushButton::clicked, this,
            [this] { ShowHelpWindow("general/license.htm", this); });
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok, this);
    auto *homepage = new QPushButton("www.7-zip.org", this);
    homepage->setObjectName("aboutHomepage");
    homepage->setProperty("sourceControlID", IDB_ABOUT_HOMEPAGE);
    connect(homepage, &QPushButton::clicked, this,
            [this, homepage] { OnButtonClicked(IDB_ABOUT_HOMEPAGE, homepage); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    auto *ok = buttons->button(QDialogButtonBox::Ok);
    const int buttonWidth = qMax(QtDialogUnits(font()).pixels(QSize(64, 0)).width(),
                                 qMax(homepage->sizeHint().width(), ok->sizeHint().width()));
    homepage->setFixedWidth(buttonWidth);
    ok->setFixedWidth(buttonWidth);
    auto *footer = new QHBoxLayout;
    footer->setSpacing(QtDialogUnits(font()).pixels(QSize(8, 0)).width());
    footer->addStretch();
    footer->addWidget(homepage);
    footer->addWidget(buttons);
    layout->addLayout(footer);
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, &CAboutDialog::OnHelp);
    OnInit();
    ok->setDefault(true);
    ok->setFocus();
    // Preserve the source client template and extend it for the explicit
    // application identity/license controls added by this port.
    QSize frame = QtDialogUnits(font()).pixels(QSize(160, 160));
    const QMargins margins = layout->contentsMargins();
    application->setMinimumSize(IdentityMinimumSize(
        *application, qMax(1, frame.width() - margins.left() - margins.right()), english, russian));
    frame.rheight() +=
        application->minimumHeight() + license->sizeHint().height() + 2 * layout->spacing();
    resize(frame.expandedTo(minimumSizeHint()));
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
