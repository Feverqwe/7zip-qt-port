#include "layout_inspection.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPixmap>
#include <QWidget>

namespace UiInspection {
QRect bounds(const QWidget &widget, const QWidget &root) {
    return QRect(widget.mapTo(&root, QPoint()), widget.size());
}

static QRect exposedBounds(const QWidget &widget, const QWidget &root) {
    if (!widget.isVisible() || (&widget != &root && !root.isAncestorOf(&widget)))
        return {};
    if (&widget == &root)
        return root.rect();
    QRect exposed = bounds(widget, root);
    for (const QWidget *parent = widget.parentWidget(); parent; parent = parent->parentWidget()) {
        exposed = exposed.intersected(bounds(*parent, root));
        if (parent == &root)
            break;
    }
    return exposed.intersected(root.rect());
}

bool fullyVisible(const QWidget &widget, const QWidget &root) {
    return widget.width() > 0 && widget.height() > 0 &&
           exposedBounds(widget, root) == bounds(widget, root);
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
    QJsonObject result{
        {"class", widget.metaObject()->className()},
        {"name", widget.objectName()},
        {"visible", widget.isVisible()},
        {"enabled", widget.isEnabled()},
        {"bounds", rectData(bounds(widget, root))},
        {"exposedBounds", rectData(exposedBounds(widget, root))},
        {"fullyVisible", fullyVisible(widget, root)},
        {"sizeHint", QJsonArray{widget.sizeHint().width(), widget.sizeHint().height()}}};
    if (const auto *label = qobject_cast<const QLabel *>(&widget)) {
        result["text"] = label->text();
        result["labelFits"] = labelFits(widget);
    } else if (const auto *button = qobject_cast<const QAbstractButton *>(&widget)) {
        result["text"] = button->text();
    } else if (const auto *group = qobject_cast<const QGroupBox *>(&widget)) {
        result["text"] = group->title();
    } else if (const auto *combo = qobject_cast<const QComboBox *>(&widget)) {
        result["editable"] = combo->isEditable();
        result["itemCount"] = combo->count();
        if (!combo->isEditable())
            result["text"] = combo->currentText();
    }
    QJsonArray children;
    for (const QWidget *child :
         widget.findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
        children.append(describe(*child, root));
    result["children"] = children;
    return result;
}

QString dump(const QWidget &root) {
    return QString::fromUtf8(QJsonDocument(describe(root, root)).toJson());
}

bool writeReport(QWidget &root, const QString &tag) {
    if (tag.isEmpty() || tag.contains('/') || tag.contains('\\') || tag.contains(".."))
        return false;
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
