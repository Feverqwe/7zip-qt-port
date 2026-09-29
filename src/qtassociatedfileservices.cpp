#include "qtassociatedfileservices.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include <QDesktopServices>
#include <QMessageBox>

namespace {
bool OpenAssociation(const QUrl &url) { return QDesktopServices::openUrl(url); }
} // namespace

QtAssociatedFileServices::QtAssociatedFileServices(std::function<bool(const QUrl &)> opener)
    : _opener(opener ? std::move(opener) : OpenAssociation) {}

HRESULT QtAssociatedFileServices::StartApplication(const UString &, const UString &path,
                                                   QWidget *window) {
    // The platform association API has no waitable process handle. The native
    // caller retains its temp through the no-handle exit-event branch.
    bool opened = false;
    try {
        opened = _opener(QUrl::fromLocalFile(ToQString(path)));
    } catch (...) {
        opened = false;
    }
    if (opened)
        return S_OK;
    QMessageBox box(QMessageBox::Warning, QStringLiteral("Открыть файл"),
                    QStringLiteral("Не удалось открыть файл: %1").arg(ToQString(path)),
                    QMessageBox::Ok, window);
    box.setObjectName("externalFileOpenError");
    box.setTextFormat(Qt::PlainText);
    box.exec();
    return E_FAIL;
}
