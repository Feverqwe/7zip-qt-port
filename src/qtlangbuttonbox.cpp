#include "qtlangbuttonbox.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include <QCoreApplication>
#include <QEvent>
#include <QPushButton>
#include <utility>

QtLangButtonBox::QtLangButtonBox(StandardButtons buttons, QWidget *parent, UInt32 acceptID)
    : QDialogButtonBox(buttons, parent), acceptID_(acceptID) {
    ReloadText();
}

void QtLangButtonBox::SetAcceptText(const char *context, const char *source) {
    acceptContext_ = context;
    acceptSource_ = source;
    ReloadText();
}

void QtLangButtonBox::UseCompactHelp() {
    compactHelp_ = true;
    ReloadText();
}

QPushButton *QtLangButtonBox::Control(UInt32 controlID) const {
    if (controlID == acceptID_)
        return button(Ok);
    switch (controlID) {
    case IDCANCEL:
        return button(Cancel);
    case IDYES:
        return button(Yes);
    case IDNO:
        return button(No);
    case IDCLOSE:
        return button(Close);
    case IDHELP:
        return button(Help);
    default:
        return nullptr;
    }
}

bool QtLangButtonBox::HasItem(UInt32 controlID) const { return Control(controlID) != nullptr; }

void QtLangButtonBox::SetItemText(UInt32 controlID, const wchar_t *text) {
    if (auto *control = Control(controlID))
        control->setText(QString::fromWCharArray(text));
}

void QtLangButtonBox::ReloadText() {
    // GuiCommon.rc captions survive when the selected language has no entry.
    if (auto *control = button(Ok))
        control->setText(acceptID_ == IDCONTINUE ? QStringLiteral("Continue")
                                                 : QStringLiteral("OK"));
    for (const auto &entry :
         {std::pair{Cancel, "Cancel"}, std::pair{Yes, "&Yes"}, std::pair{No, "&No"},
          std::pair{Close, "&Close"}, std::pair{Help, "Help"}})
        if (auto *control = button(entry.first))
            control->setText(QString::fromLatin1(entry.second));
    LangSetDlgItems(*this, nullptr, 0);
    if (auto *control = button(Ok); control && !acceptSource_.isEmpty())
        control->setText(
            QCoreApplication::translate(acceptContext_.constData(), acceptSource_.constData()));
    if (auto *help = button(Help); help && compactHelp_) {
        const QString caption = help->text().remove('&');
        help->setAccessibleName(caption);
        help->setToolTip(caption);
        help->setText(QStringLiteral("?"));
        help->setIcon({});
        help->setAutoDefault(false);
#ifdef Q_OS_MACOS
        help->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
        help->setFixedWidth(help->sizeHint().height());
    }
}

void QtLangButtonBox::changeEvent(QEvent *event) {
    QDialogButtonBox::changeEvent(event);
    // Qt resets standard captions in its base handler, after the window filter.
    if (event->type() == QEvent::LanguageChange ||
        ((event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) &&
         compactHelp_))
        ReloadText();
}
