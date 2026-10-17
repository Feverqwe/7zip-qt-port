// Adapted from 7-Zip CPP/7zip/UI/FileManager/EditDialog.cpp/.rc (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt replaces Win32 edit attachment/OnSize with a layout. Resource styles
// ES_READONLY/ES_MULTILINE/WS_HSCROLL/WS_VSCROLL/ES_WANTRETURN are retained.
#include "EditDialog.h"
#include "ViewSettings.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

CEditDialog::CEditDialog(QWidget *parent) : QDialog(parent) {}

bool CEditDialog::OnInit() {
    setObjectName("editDialog");
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    const QtDialogUnits units(font());
    const QSize margin = units.pixels(QSize(8, 8));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(margin.width(), margin.height(), margin.width(), margin.height());
    layout->setSpacing(margin.height());
    _edit = new QPlainTextEdit(this);
    _edit->setObjectName("editText");
    _edit->setReadOnly(true);
    _edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    _edit->setTabChangesFocus(true);
    _edit->installEventFilter(this);
    setWindowTitle(ToQString(Title));
    _edit->setPlainText(ToQString(Text));
    layout->addWidget(_edit);
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Close, this);
    buttons->setObjectName("editButtons");
    auto *close = buttons->button(QDialogButtonBox::Close);
    close->setDefault(true);
    close->setFixedWidth(qMax(units.pixels(QSize(64, 0)).width(), close->sizeHint().width()));
    close->setMinimumHeight(units.pixels(QSize(0, 16)).height());
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    resize(units.pixels(QSize(336, 256)).expandedTo(minimumSizeHint()));
    QtNormalizeDialogSize(*this);
    close->setFocus();
    return true;
}

bool CEditDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == _edit && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        // ES_WANTRETURN keeps Enter in the read-only edit instead of
        // activating the dialog's default Close button.
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
            return true;
    }
    return QDialog::eventFilter(watched, event);
}
