#include "qthelpservices.h"
#include "Common/StringConvert.h"
#include "qtlangcontrols.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

static void InitHelpResources() { Q_INIT_RESOURCE(upstream_help); }

FString QtHelpModuleDirPrefix() {
    const auto path = (QCoreApplication::applicationDirPath() + '/').toStdWString();
    return us2fs(UString(path.c_str()));
}

void QtDisplayHelpTopic(const FString &path, QWidget *parent) {
    const QString value = QString::fromWCharArray(fs2us(path).Ptr());
    const int marker = value.indexOf("7-zip.chm::/");
    QtOpenHelpTopic(marker < 0 ? QString() : value.mid(marker + 12), parent);
}

void QtOpenHelpTopic(const QString &topic, QWidget *parent, const QString &root) {
    InitHelpResources();
    const QUrl relative(topic);
    QString fileName = relative.path().toLower(); // Native CHM paths are case insensitive.
    if (fileName.isEmpty())
        fileName = "start.htm";
    const QString base = root.isEmpty() ? ":/upstream/7zip/Help" : root;
    const QString path = base + '/' + fileName;
    if (!relative.isRelative() || fileName.startsWith('/') || fileName.contains("..") ||
        !QFile::exists(path)) {
        QMessageBox::warning(
            parent, QtLocalText("7-Zip Help", "Справка 7-Zip"),
            QtLocalText("Help topic is unavailable: %1", "Раздел справки недоступен: %1")
                .arg(topic));
        return;
    }
    auto *window = new QDialog(parent);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->setObjectName("helpWindow");
    QtBindLocalText(window, "7-Zip Help", "Справка 7-Zip");
    window->resize(760, 560);
    auto *layout = new QVBoxLayout(window);
    auto *browser = new QTextBrowser(window);
    browser->setObjectName("helpBrowser");
    browser->setOpenLinks(false);
    layout->addWidget(browser, 1);
    QUrl source =
        root.isEmpty() ? QUrl("qrc:/upstream/7zip/Help/" + fileName) : QUrl::fromLocalFile(path);
    source.setFragment(relative.fragment());
    browser->setSource(source);
    QObject::connect(browser, &QTextBrowser::anchorClicked, window, [browser](const QUrl &url) {
        const QUrl target = browser->source().resolved(url);
        if (target.scheme() == "http" || target.scheme() == "https")
            QDesktopServices::openUrl(target);
        else if (target.scheme() == browser->source().scheme())
            browser->setSource(target);
    });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, window);
    auto *back = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
    QtBindLocalText(back, "Back", "Назад");
    back->setEnabled(false);
    QObject::connect(browser, &QTextBrowser::backwardAvailable, back, &QPushButton::setEnabled);
    QObject::connect(back, &QPushButton::clicked, browser, &QTextBrowser::backward);
    QObject::connect(buttons, &QDialogButtonBox::rejected, window, &QDialog::close);
    layout->addWidget(buttons);
    window->show();
}
