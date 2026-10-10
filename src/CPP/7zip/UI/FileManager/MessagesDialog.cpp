// Adapted from 7-Zip CPP/7zip/UI/FileManager/MessagesDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "MessagesDialog.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "Common/IntToString.h"
#include "LangUtils.h"
#include "MessagesDialogRes.h"
#include "qtlangcontrols.h"
#include "qtlistview.h"
#include <QEvent>
#include <QFontMetricsF>
#include <QPushButton>
#include <QResizeEvent>
#include <cmath>

void CMessagesDialog::AddMessageDirect(const wchar_t *message) {
    const unsigned i = unsigned(_messageList.GetItemCount());
    wchar_t sz[16];
    ConvertUInt32ToString(i, sz);
    _messageList.InsertItem(i, sz);
    _messageList.SetSubItem(i, 1, message);
}

void CMessagesDialog::AddMessage(const wchar_t *message) {
    UString s = message;
    while (!s.IsEmpty()) {
        const int pos = s.Find(L'\n');
        if (pos < 0)
            break;
        AddMessageDirect(s.Left(unsigned(pos)));
        s.DeleteFrontal(unsigned(pos) + 1);
    }
    AddMessageDirect(s);
}

bool CMessagesDialog::OnInit() {
    _messageList.Attach(_list);
    _messageList.InsertColumn(0, L"", 30);
    _messageList.InsertColumn(1, LangString(IDS_MESSAGE), 600);

    if (Messages)
        FOR_VECTOR(i, *Messages)
    AddMessage((*Messages)[i]);

    _messageList.SetColumnWidthAuto(0);
    _messageList.SetColumnWidthAuto(1);
    OnSize(width(), height());
    return true;
}

bool CMessagesDialog::OnSize(int xSize, int ySize) {
    const int mx = int(std::round(8 * _unitX));
    const int my = int(std::round(8 * _unitY));
    const int bx = _close->width();
    const int by = _close->height();
    const int y = ySize - my - by;
    const int x = xSize - mx - bx;

    _close->setGeometry(x, y, bx, by);
    _list->setGeometry(mx, my, xSize - mx * 2, y - my * 2);
    return false;
}

CMessagesDialog::CMessagesDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("operationMessagesDialog");
    QtBindResourceText(this, IDD_MESSAGES, "7-Zip: Diagnostic messages");
    const QFontMetricsF metrics(font());
    _unitX = metrics.horizontalAdvance(
                 QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")) /
             52 / 4;
    _unitY = std::ceil(metrics.height()) / 8;
    _list = new CListView(this);
    _list->setObjectName("IDL_MESSAGE");
    _list->setColumnCount(2);
    _list->setHeaderHidden(false);
    _close = new QPushButton(this);
    QtBindResourceText(_close, IDS_CLOSE, "&Close");
    _close->setObjectName("IDOK");
    _close->setDefault(true);
    connect(_close, &QPushButton::clicked, this, &QDialog::accept);
    resize(int(std::round(456 * _unitX)), int(std::round(176 * _unitY)));
    _close->resize(qMax(int(std::round(64 * _unitX)), _close->sizeHint().width()),
                   qMax(int(std::round(16 * _unitY)), _close->sizeHint().height()));
}

INT_PTR CMessagesDialog::Create(QWidget *parent) {
    if (parent && parentWidget() != parent)
        setParent(parent, Qt::Dialog);
    if (!_initialized) {
        _initialized = OnInit();
        _list->setFocus();
    }
    return exec();
}

void CMessagesDialog::reject() { done(IDCANCEL); }

void CMessagesDialog::resizeEvent(QResizeEvent *event) {
    if (_initialized)
        OnSize(event->size().width(), event->size().height());
    QDialog::resizeEvent(event);
}

void CMessagesDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && _initialized) {
        _list->headerItem()->setText(1, QtLangText(IDS_MESSAGE));
        _close->resize(qMax(int(std::round(64 * _unitX)), _close->sizeHint().width()),
                       qMax(int(std::round(16 * _unitY)), _close->sizeHint().height()));
        OnSize(width(), height());
    }
    QDialog::changeEvent(event);
}
