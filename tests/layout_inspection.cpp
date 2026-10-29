#include "layout_inspection.h"
#include <QScreen>

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPixmap>
#include <QStyle>
#include <QStyleHints>
#include <QSysInfo>
#include <QToolButton>
#include <QWidget>

namespace UiInspection {
void configureReviewAppearance() {
    const QString requested = qEnvironmentVariable("SEVENZIP_REVIEW_COLOR_SCHEME");
    if (requested.isEmpty())
        return;
    if (requested != "light" && requested != "dark" && requested != "system")
        qFatal("SEVENZIP_REVIEW_COLOR_SCHEME must be light, dark or system");
    const auto scheme = requested == "dark"    ? Qt::ColorScheme::Dark
                        : requested == "light" ? Qt::ColorScheme::Light
                                               : Qt::ColorScheme::Unknown;
    QApplication::styleHints()->setColorScheme(scheme);
    // Native theme updates are queued. No test fixture/timer exists yet.
    QApplication::processEvents();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
}
QSize normalizedInitialClientSize(const QWidget &widget, const QSize &resourceClient) {
    QSize expected = resourceClient.expandedTo(widget.minimumSizeHint());
    const QRect work = widget.screen()->availableGeometry();
    if (widget.frameGeometry().intersects(work)) {
        const QSize frame = widget.frameGeometry().size() - widget.size();
        expected = expected.boundedTo(work.size() - frame).expandedTo(widget.minimumSizeHint());
    }
    return expected;
}
QRect bounds(const QWidget &widget, const QWidget &root) {
    return QRect(widget.mapTo(&root, QPoint()), widget.size());
}

static QRect exposedBounds(const QWidget &widget, const QWidget &root, const QRect &rect) {
    if (!widget.isVisible() || (&widget != &root && !root.isAncestorOf(&widget)))
        return {};
    if (&widget == &root)
        return root.rect();
    QRect exposed = rect;
    for (const QWidget *parent = widget.parentWidget(); parent; parent = parent->parentWidget()) {
        exposed = exposed.intersected(bounds(*parent, root));
        if (parent == &root)
            break;
    }
    return exposed.intersected(root.rect());
}

static bool fullyVisible(const QRect &rect, const QRect &exposed) {
    return rect.width() > 0 && rect.height() > 0 && exposed == rect;
}

bool fullyVisible(const QWidget &widget, const QWidget &root) {
    const QRect rect = bounds(widget, root);
    return fullyVisible(rect, exposedBounds(widget, root, rect));
}

static QRect lineEditIconBounds(const QWidget &widget, const QWidget &root) {
    const auto *edit = qobject_cast<const QLineEdit *>(widget.parentWidget());
    const auto *button = qobject_cast<const QToolButton *>(&widget);
    if (!edit || !button || button->icon().isNull())
        return {};
    // Qt 6.12 QLineEditIconButton::paintEvent centers this logical glyph;
    // native style may clip transparent side-widget padding, not the icon.
    const int size = edit->style()->pixelMetric(QStyle::PM_LineEditIconSize, nullptr, edit);
    QRect glyph(QPoint(), QSize(size, size));
    glyph.moveCenter(widget.rect().center());
    if (!widget.rect().contains(glyph))
        return {};
    return glyph.translated(widget.mapTo(&root, QPoint()));
}

bool lineEditIconVisible(const QWidget &widget, const QWidget &root) {
    const QRect glyph = lineEditIconBounds(widget, root);
    return fullyVisible(glyph, exposedBounds(widget, root, glyph));
}

bool separate(const QWidget &first, const QWidget &second, const QWidget &root) {
    return !bounds(first, root).intersects(bounds(second, root));
}

bool labelFits(const QWidget &widget) {
    const auto *label = qobject_cast<const QLabel *>(&widget);
    if (!label)
        return false;
    if (label->wordWrap())
        return label->heightForWidth(label->width()) <= label->height();
    return label->sizeHint().width() <= label->width() &&
           label->sizeHint().height() <= label->height();
}

static QJsonArray rectData(const QRect &rect) {
    return {rect.x(), rect.y(), rect.width(), rect.height()};
}

static QJsonObject describe(const QWidget &widget, const QWidget &root) {
    const QRect rect = bounds(widget, root);
    const QRect exposed = exposedBounds(widget, root, rect);
    const QSize hint = widget.sizeHint();
    QJsonObject result{{"class", widget.metaObject()->className()},
                       {"name", widget.objectName()},
                       {"visible", widget.isVisible()},
                       {"enabled", widget.isEnabled()},
                       {"bounds", rectData(rect)},
                       {"exposedBounds", rectData(exposed)},
                       {"fullyVisible", fullyVisible(rect, exposed)},
                       {"sizeHint", QJsonArray{hint.width(), hint.height()}}};
    if (const auto *label = qobject_cast<const QLabel *>(&widget)) {
        result["text"] = label->text();
        result["labelFits"] = labelFits(widget);
    } else if (const auto *button = qobject_cast<const QAbstractButton *>(&widget)) {
        result["text"] = button->text();
        if (qobject_cast<const QToolButton *>(&widget) &&
            qobject_cast<const QLineEdit *>(widget.parentWidget())) {
            result["iconBounds"] = rectData(lineEditIconBounds(widget, root));
            result["iconFullyVisible"] = lineEditIconVisible(widget, root);
        }
    } else if (const auto *group = qobject_cast<const QGroupBox *>(&widget)) {
        result["text"] = group->title();
    } else if (const auto *combo = qobject_cast<const QComboBox *>(&widget)) {
        result["editable"] = combo->isEditable();
        result["itemCount"] = combo->count();
        if (!combo->isEditable())
            result["text"] = combo->currentText();
    } else if (const auto *menu = qobject_cast<const QMenu *>(&widget)) {
        QJsonArray actions;
        for (auto *action : menu->actions()) {
            const QRect rect = menu->actionGeometry(action);
            actions.append(QJsonObject{{"text", action->text()},
                                       {"separator", action->isSeparator()},
                                       {"enabled", action->isEnabled()},
                                       {"submenu", action->menu() != nullptr},
                                       {"bounds", rectData(rect)},
                                       {"fullyVisible", menu->isVisible() && !rect.isEmpty() &&
                                                            menu->rect().contains(rect)},
                                       {"shellVerb", action->property("shellVerb").toString()}});
        }
        result["actions"] = actions;
    }
    QJsonArray children;
    for (const QWidget *child :
         widget.findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
        children.append(describe(*child, root));
    result["children"] = children;
    return result;
}

QString dump(const QWidget &root) {
    auto report = describe(root, root);
    report["capture"] = QJsonObject{{"schema", 1},
                                    {"platform", QSysInfo::kernelType()},
                                    {"qpa", QApplication::platformName()},
                                    {"qtVersion", qVersion()},
                                    {"devicePixelRatio", root.devicePixelRatioF()},
                                    {"logicalDpi", root.logicalDpiX()},
                                    {"fontFamily", root.font().family()},
                                    {"fontPointSize", root.font().pointSizeF()},
                                    {"coordinateSpace", "root-client-logical-pixels"}};
    auto capture = report["capture"].toObject();
    const auto scheme = QApplication::styleHints()->colorScheme();
    capture["colorScheme"] = scheme == Qt::ColorScheme::Dark    ? "dark"
                             : scheme == Qt::ColorScheme::Light ? "light"
                                                                : "unknown";
    capture["requestedColorScheme"] = qEnvironmentVariable("SEVENZIP_REVIEW_COLOR_SCHEME");
    QJsonObject palette;
    for (const auto &role : {QPalette::Window, QPalette::WindowText, QPalette::Base, QPalette::Text,
                             QPalette::Highlight, QPalette::HighlightedText, QPalette::Link})
        palette[QString::number(role)] = root.palette().color(role).name();
    capture["palette"] = palette;
    report["capture"] = capture;
    for (const QString &key : {QStringLiteral("language"), QStringLiteral("state")}) {
        const QVariant value = root.property(qPrintable("uiInspection_" + key));
        if (value.isValid()) {
            auto capture = report["capture"].toObject();
            capture[key] = value.toString();
            report["capture"] = capture;
        }
    }
    return QString::fromUtf8(QJsonDocument(report).toJson(QJsonDocument::Compact));
}

bool writeReport(QWidget &root, const QString &tag) {
    if (tag.isEmpty() || tag.contains('/') || tag.contains('\\') || tag.contains(".."))
        return false;
    if (qEnvironmentVariable("SEVENZIP_UI_REPORTS") != "ON" &&
        qEnvironmentVariable("SEVENZIP_CAPTURE_SCREENSHOTS") != "1")
        return true;
    const QString directory =
        qEnvironmentVariable("SEVENZIP_UI_REPORT_DIR", QDir::currentPath() + "/ui-reports");
    if (!QDir().mkpath(directory))
        return false;
    QFile file(directory + "/" + tag + ".json");
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray data = dump(root).toUtf8();
    if (file.write(data) != data.size() || !file.flush())
        return false;
    return qEnvironmentVariable("SEVENZIP_CAPTURE_SCREENSHOTS") != "1" ||
           root.grab().save(directory + "/" + tag + ".png");
}
} // namespace UiInspection
