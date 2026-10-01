#include "qtlangcontrols.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QVariant>
#include <QWidget>

QString QtLangText(UInt32 id, const QString &englishFallback) {
    const UString s = LangString(id);
    return s.IsEmpty() ? englishFallback : QString::fromWCharArray(s);
}

namespace {
void SetText(QObject *object) {
    const QVariant id = object->property("languageID");
    if (!id.isValid())
        return;
    QString text = QtLangText(id.toUInt(), object->property("languageFallback").toString());
    if (object->property("languageColon").toBool())
        text += ':';
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
} // namespace

void QtBindLangText(QObject *object, UInt32 id, const QString &englishFallback,
                    bool accessibleOnly) {
    object->setProperty("languageID", id);
    object->setProperty("languageFallback", englishFallback);
    object->setProperty("languageAccessibleOnly", accessibleOnly);
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
