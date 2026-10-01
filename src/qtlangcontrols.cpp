#include "qtlangcontrols.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextLayout>
#include <QVariant>
#include <QWidget>

QString QtLangText(UInt32 id, const QString &englishFallback) {
    const UString s = LangString(id);
    return s.IsEmpty() ? englishFallback : QString::fromWCharArray(s);
}

namespace {
void SetText(QObject *object) {
    if (auto *widget = qobject_cast<QWidget *>(object))
        if (object->property("languageToolTipEnglish").isValid())
            widget->setToolTip(QtLocalText(object->property("languageToolTipEnglish").toString(),
                                           object->property("languageToolTipRussian").toString()));
    const QVariant id = object->property("languageID");
    const QVariant local = object->property("languageLocalEnglish");
    if (!id.isValid() && !local.isValid())
        return;
    QString text =
        id.isValid()
            ? QtLangText(id.toUInt(), object->property("languageFallback").toString())
            : QtLocalText(local.toString(), object->property("languageLocalRussian").toString());
    if (id.isValid() && object->property("languageResourceCaption").isValid()) {
        UString translated;
        LangString_OnlyFromLangFile(id.toUInt(), translated);
        text = translated.IsEmpty() ? object->property("languageResourceCaption").toString()
                                    : QString::fromWCharArray(translated);
    }
    if (object->property("languageColon").toBool())
        text += ':';
    text += object->property("languageSuffix").toString();
    const int wrapWidth = object->property("languageWrapWidth").toInt();
    if (wrapWidth > 0)
        if (auto *widget = qobject_cast<QWidget *>(object)) {
            QTextLayout layout(text, widget->font());
            QTextOption option;
            option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            layout.setTextOption(option);
            QStringList lines;
            layout.beginLayout();
            for (;;) {
                QTextLine line = layout.createLine();
                if (!line.isValid())
                    break;
                line.setLineWidth(wrapWidth);
                lines.append(text.mid(line.textStart(), line.textLength()).trimmed());
            }
            layout.endLayout();
            text = lines.join('\n');
        }
    if (auto *widget = qobject_cast<QWidget *>(object)) {
        if (object->property("languageToolTip").toBool())
            widget->setToolTip(QString(text).remove('&'));
        if (object->property("languagePlaceholder").toBool())
            if (auto *edit = qobject_cast<QLineEdit *>(widget))
                edit->setPlaceholderText(QString(text).remove('&'));
    }
    if (object->property("languageAccessibleOnly").toBool()) {
        if (auto *widget = qobject_cast<QWidget *>(object)) {
            QString name = text;
            name.remove('&');
            widget->setAccessibleName(name);
        }
        return;
    }
    if (auto *action = qobject_cast<QAction *>(object))
        action->setText(text);
    else if (auto *label = qobject_cast<QLabel *>(object))
        label->setText(text);
    else if (auto *button = qobject_cast<QAbstractButton *>(object))
        button->setText(text);
    else if (auto *group = qobject_cast<QGroupBox *>(object))
        group->setTitle(text);
    else if (auto *widget = qobject_cast<QWidget *>(object))
        widget->setWindowTitle(text);
}
class QtLangTextFilter final : public QObject {
  public:
    explicit QtLangTextFilter(QWidget *window) : QObject(window) {
        setObjectName("qtLangTextFilter");
        window->installEventFilter(this);
    }

  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::LanguageChange)
            if (auto *window = qobject_cast<QWidget *>(object))
                QtReloadLangTexts(*window);
        return QObject::eventFilter(object, event);
    }
};
void InstallLanguageFilter(QObject *object) {
    if (auto *widget = qobject_cast<QWidget *>(object)) {
        QWidget *window = widget->window();
        if (!window->findChild<QObject *>("qtLangTextFilter", Qt::FindDirectChildrenOnly))
            new QtLangTextFilter(window);
    }
}
} // namespace

void QtBindLangText(QObject *object, UInt32 id, const QString &englishFallback,
                    bool accessibleOnly) {
    object->setProperty("languageID", id);
    object->setProperty("languageResourceCaption", QVariant());
    object->setProperty("languageFallback", englishFallback);
    object->setProperty("languageAccessibleOnly", accessibleOnly);
    InstallLanguageFilter(object);
    SetText(object);
}

void QtBindResourceText(QObject *object, UInt32 id, const QString &resourceCaption,
                        bool accessibleOnly) {
    QtBindLangText(object, id, resourceCaption, accessibleOnly);
    object->setProperty("languageResourceCaption", resourceCaption);
    SetText(object);
}

void QtBindLocalText(QObject *object, const QString &english, const QString &russian,
                     bool accessibleOnly) {
    object->setProperty("languageLocalEnglish", english);
    object->setProperty("languageLocalRussian", russian);
    object->setProperty("languageID", QVariant());
    object->setProperty("languageAccessibleOnly", accessibleOnly);
    InstallLanguageFilter(object);
    SetText(object);
}

void QtBindLocalToolTip(QObject *object, const QString &english, const QString &russian) {
    object->setProperty("languageToolTipEnglish", english);
    object->setProperty("languageToolTipRussian", russian);
    InstallLanguageFilter(object);
    SetText(object);
}

void QtReloadLangTexts(QWidget &root) {
    SetText(&root);
    for (QObject *object : root.findChildren<QObject *>())
        SetText(object);
}

void QtNotifyLanguageChanged() {
    // Qt has no Win32 property-sheet/menu reload message. Notify current
    // windows after the source language is applied; pending input is untouched.
    for (QWidget *window : QApplication::topLevelWidgets()) {
        QEvent event(QEvent::LanguageChange);
        QApplication::sendEvent(window, &event);
    }
}
