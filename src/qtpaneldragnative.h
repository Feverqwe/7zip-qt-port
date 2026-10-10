#pragma once
#include "qtpaneldragmodel.h"
#ifdef _WIN32
#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QWindowsMimeConverter>
class QMimeData;
// The original Windows IDataObject protocol, with typed ownership at Qt's boundary.
HRESULT CreatePanelDragDataObject(const std::shared_ptr<CDataObject> &, IDataObject **);
std::shared_ptr<IQtNativeDragData> ReadPanelDragDataObject(IDataObject *);
HRESULT DoNativePanelDrag(CDataObject &, CDropSource &, DWORD, DWORD &);
class QtNativePanelDragConverter final : public QObject, public QWindowsMimeConverter {
  public:
    explicit QtNativePanelDragConverter(QObject *parent = nullptr);
    static QtNativePanelDragConverter *ForApplication();
    std::shared_ptr<IQtNativeDragData> Read(const QMimeData &);
    void Clear();
    bool canConvertFromMime(const FORMATETC &, const QMimeData *) const override;
    bool convertFromMime(const FORMATETC &, const QMimeData *, STGMEDIUM *) const override;
    QList<FORMATETC> formatsForMime(const QString &, const QMimeData *) const override;
    bool canConvertToMime(const QString &, IDataObject *) const override;
    QVariant convertToMime(const QString &, IDataObject *, QMetaType) const override;
    QString mimeForFormat(const FORMATETC &) const override;

  private:
    mutable QByteArray token_;
    mutable std::shared_ptr<IQtNativeDragData> data_;
    QPointer<const QMimeData> mime_;
};
#endif
