// Adapted from 7-Zip CPP/7zip/UI/FileManager/EditDialog.cpp/.rc (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt replaces Win32 edit attachment/OnSize with a layout. Resource styles
// ES_READONLY/ES_MULTILINE/WS_HSCROLL/WS_VSCROLL/ES_WANTRETURN are retained.
#include "EditDialog.h"
#include "ViewSettings.h"
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QVBoxLayout>

CEditDialog::CEditDialog(QWidget *parent) : QDialog(parent) {}

bool CEditDialog::OnInit() {
    setObjectName("editDialog");
    resize(480, 320);
    auto *layout = new QVBoxLayout(this);
    _edit = new QPlainTextEdit(this);
    _edit->setObjectName("editText");
    _edit->setReadOnly(true);
    _edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    _edit->setTabChangesFocus(true);
    _edit->installEventFilter(this);
    setWindowTitle(ToQString(Title));
    _edit->setPlainText(ToQString(Text));
    layout->addWidget(_edit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->setObjectName("editButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    _edit->setFocus();
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
