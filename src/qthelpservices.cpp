#include "qthelpservices.h"
#include "Common/StringConvert.h"
#include "qtlangcontrols.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QHash>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>

static void InitHelpResources() { Q_INIT_RESOURCE(upstream_help); }

namespace {
// Qt presentation adapter for the original CHM pages. Retain their stylesheet's
// layout and typography, replacing only its fixed light-theme color tokens.
class HelpBrowser final : public QTextBrowser {
  public:
    HelpBrowser(QWidget *parent, const QUrl &root) : QTextBrowser(parent) {
        connect(this, &QTextBrowser::sourceChanged, this, [this, root] { resolveHelpLinks(root); });
    }

  protected:
    QVariant loadResource(int type, const QUrl &url) override {
        const QVariant resource = QTextBrowser::loadResource(type, url);
        if (type != QTextDocument::StyleSheetResource || url.fileName() != "style.css" ||
            !resource.isValid())
            return resource;
        const QByteArray css = resource.toByteArray();
        styles.insert(url, css);
        return themedStyle(css);
    }

    void changeEvent(QEvent *event) override {
        QTextBrowser::changeEvent(event);
        if (event->type() != QEvent::PaletteChange || source().isEmpty())
            return;
        for (auto it = styles.cbegin(); it != styles.cend(); ++it)
            document()->addResource(QTextDocument::StyleSheetResource, it.key(),
                                    themedStyle(it.value()));
        const int scroll = verticalScrollBar()->value();
        const int horizontalScroll = horizontalScrollBar()->value();
        reload();
        verticalScrollBar()->setValue(scroll);
        horizontalScrollBar()->setValue(horizontalScroll);
    }

  private:
    void resolveHelpLinks(const QUrl &root) {
        // QTextBrowser resolves hrefs before emitting anchorClicked, so resolve
        // them inside the CHM namespace before mouse/keyboard activation.
        QUrl member;
        member.setScheme("help");
        member.setPath('/' + source().path().mid(root.path().size()));
        struct Link {
            int start;
            int length;
            QString href;
        };
        QList<Link> links;
        for (auto block = document()->begin(); block.isValid(); block = block.next()) {
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                const auto fragment = it.fragment();
                const auto format = fragment.charFormat();
                const QUrl href(format.anchorHref());
                if (!format.isAnchor() || href.isEmpty() || !href.isRelative() ||
                    !href.authority().isEmpty())
                    continue;
                const QUrl resolved = member.resolved(href);
                QUrl target = root;
                target.setPath(root.path() + resolved.path().mid(1).toLower());
                target.setFragment(resolved.fragment());
                target.setQuery(resolved.query());
                links.append({fragment.position(), fragment.length(), target.toString()});
            }
        }
        for (const auto &link : links) {
            QTextCursor cursor(document());
            cursor.setPosition(link.start);
            cursor.setPosition(link.start + link.length, QTextCursor::KeepAnchor);
            QTextCharFormat format;
            format.setAnchorHref(link.href);
            cursor.mergeCharFormat(format);
        }
    }

    QByteArray themedStyle(const QByteArray &css) const {
        const QPalette colors = palette();
        const bool dark =
            colors.color(QPalette::Base).lightness() < colors.color(QPalette::Text).lightness();
        // Substitute in one pass: palette colors may equal another source token.
        const QHash<QByteArray, QColor> replacements = {
            {"#ffffff", colors.color(QPalette::Base)},
            {"#000000", colors.color(QPalette::Text)},
            {"#999999", colors.color(QPalette::Mid)},
            {"#cccccc", colors.color(QPalette::AlternateBase)},
            {"#dddddd", colors.color(QPalette::AlternateBase)},
            {"#0066ff", colors.color(QPalette::Link)},
            {"#996600", colors.color(QPalette::LinkVisited)},
            {"#cc9900", colors.color(QPalette::Link)},
            {"#C00000", QColor(dark ? "#ff9b8e" : "#a52a21")},
            {"#F00000", QColor(dark ? "#ff9b8e" : "#a52a21")},
            {"#228B22", QColor(dark ? "#8fc99a" : "#26723b")}};
        QByteArray result;
        for (qsizetype i = 0; i < css.size();) {
            const auto color = replacements.constFind(css.mid(i, 7));
            if (css[i] == '#' && color != replacements.cend()) {
                result += color.value().name().toLatin1();
                i += 7;
            } else {
                result += css[i++];
            }
        }
        return result;
    }

    QHash<QUrl, QByteArray> styles;
};
} // namespace

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
    const QUrl helpRoot = root.isEmpty() ? QUrl("qrc:/upstream/7zip/Help/")
                                         : QUrl::fromLocalFile(QDir(root).absolutePath() + '/');
    auto *browser = new HelpBrowser(window, helpRoot);
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
