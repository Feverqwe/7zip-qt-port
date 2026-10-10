#include "qtpaneldragnative.h"
#include <QGuiApplication>
#include <QMimeData>
#include <QUuid>
#include <QVariant>

namespace {
const QString kNativeMime = QStringLiteral("application/x-sevenzip-native-drag");
const QString kRegistryName = QStringLiteral("sevenzipNativeDragConverter");
void DestroyApplicationConverter() {
    if (auto *application = QGuiApplication::instance())
        delete dynamic_cast<QtNativePanelDragConverter *>(
            application->findChild<QObject *>(kRegistryName, Qt::FindDirectChildrenOnly));
}
bool IsNamesFormat(CLIPFORMAT format) {
    return format == CF_HDROP || format == RegisterClipboardFormatW(L"Shell IDList Array");
}
} // namespace

QtNativePanelDragConverter::QtNativePanelDragConverter(QObject *parent) : QObject(parent) {
    setObjectName(kRegistryName);
}
QtNativePanelDragConverter *QtNativePanelDragConverter::ForApplication() {
    auto *application = QGuiApplication::instance();
    if (!application || QGuiApplication::platformName() != QStringLiteral("windows"))
        return nullptr;
    if (auto *existing = dynamic_cast<QtNativePanelDragConverter *>(
            application->findChild<QObject *>(kRegistryName, Qt::FindDirectChildrenOnly)))
        return existing;
    auto *converter = new QtNativePanelDragConverter(application);
    // QGuiApplication runs post routines before deleting platform integration.
    // QObject's ordinary child cleanup is too late for unregisterMime().
    qAddPostRoutine(DestroyApplicationConverter);
    return converter;
}

std::shared_ptr<IQtNativeDragData> QtNativePanelDragConverter::Read(const QMimeData &mime) {
    const QByteArray token = mime.data(kNativeMime);
    if (token.isEmpty() || token != token_)
        return nullptr;
    if (mime_ != &mime) {
        mime_ = &mime;
        connect(&mime, &QObject::destroyed, this, [this, source = &mime] {
            if (!mime_ || mime_ == source)
                Clear();
        });
    }
    return data_;
}
void QtNativePanelDragConverter::Clear() {
    token_.clear();
    data_.reset();
    mime_.clear();
}
bool QtNativePanelDragConverter::canConvertFromMime(const FORMATETC &, const QMimeData *) const {
    return false;
}
bool QtNativePanelDragConverter::convertFromMime(const FORMATETC &, const QMimeData *,
                                                 STGMEDIUM *) const {
    return false;
}
QList<FORMATETC> QtNativePanelDragConverter::formatsForMime(const QString &,
                                                            const QMimeData *) const {
    return {};
}
bool QtNativePanelDragConverter::canConvertToMime(const QString &mime, IDataObject *data) const {
    if (mime != kNativeMime || !data)
        return false;
    FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (data->QueryGetData(&format) == S_OK)
        return true;
    format.cfFormat = CLIPFORMAT(RegisterClipboardFormatW(L"Shell IDList Array"));
    return data->QueryGetData(&format) == S_OK;
}
QVariant QtNativePanelDragConverter::convertToMime(const QString &mime, IDataObject *data,
                                                   QMetaType) const {
    if (!canConvertToMime(mime, data))
        return {};
    data_ = ReadPanelDragDataObject(data);
    token_ = QUuid::createUuid().toByteArray();
    return token_;
}
QString QtNativePanelDragConverter::mimeForFormat(const FORMATETC &format) const {
    return IsNamesFormat(format.cfFormat) ? kNativeMime : QString();
}
