#pragma once
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/IPassword.h"
#include "7zip/PropID.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archiveenginelocation.h"
#include <QFile>

namespace NativeArchiveFixture {
struct Item {
    QString Path;
    QByteArray Bytes = "x";
    bool AltStream = false;
    QString SymLink;
    QString HardLink;
};
inline std::shared_ptr<const ArchiveCodecCatalog> Catalog() {
    static const auto catalog = ArchiveCodecProbe::LoadCatalog(FindArchiveEngine());
    return catalog;
}
class CWriter final : public IArchiveUpdateCallback,
                      public ICryptoGetTextPassword2,
                      public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveUpdateCallback, ICryptoGetTextPassword2)
    const QList<Item> items_;
    const QString password_;

  public:
    CWriter(QList<Item> items, QString password)
        : items_(std::move(items)), password_(std::move(password)) {}
    Z7_COM7F_IMF(CryptoGetTextPassword2(Int32 *defined, BSTR *value)) override {
        *defined = !password_.isNull();
        return StringToBstr(password_.toStdWString().c_str(), value);
    }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetUpdateItemInfo(UInt32, Int32 *data, Int32 *props, UInt32 *index)) override {
        *data = *props = 1;
        *index = UInt32(-1);
        return S_OK;
    }
    Z7_COM7F_IMF(GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) override {
        const auto &item = items_[int(index)];
        NWindows::NCOM::CPropVariant property;
        if (id == kpidPath)
            property = item.Path.toStdWString().c_str();
        else if (id == kpidSize)
            property = UInt64(item.Bytes.size());
        else if (id == kpidIsDir)
            property = false;
        else if (id == kpidIsAltStream)
            property = item.AltStream;
        else if (id == kpidSymLink && !item.SymLink.isNull())
            property = item.SymLink.toStdWString().c_str();
        else if (id == kpidHardLink && !item.HardLink.isNull())
            property = item.HardLink.toStdWString().c_str();
        return property.Detach(value);
    }
    Z7_COM7F_IMF(GetStream(UInt32 index, ISequentialInStream **stream)) override {
        const auto &bytes = items_[int(index)].Bytes;
        Create_BufInStream_WithNewBuffer(bytes.constData(), size_t(bytes.size()), stream);
        return S_OK;
    }
    Z7_COM7F_IMF(SetOperationResult(Int32)) override { return S_OK; }
};
inline bool Write(const QString &path, const QList<Item> &items, const QString &format = "7z",
                  const QString &password = {}) {
    const auto catalog = Catalog();
    const int index = catalog->formatIndex(format);
    if (index < 0)
        return false;
    CMyComPtr<IOutArchive> writer;
    if (catalog->nativeCodecs().CreateOutArchive(unsigned(index), writer) != S_OK || !writer)
        return false;
    CMyComPtr<ISetCompressCodecsInfo> providers;
    writer.QueryInterface(IID_ISetCompressCodecsInfo, &providers);
    if (providers &&
        providers->SetCompressCodecsInfo(const_cast<CCodecs *>(&catalog->nativeCodecs())) != S_OK)
        return false;
    if (!password.isNull() && format == "7z") {
        CMyComPtr<ISetProperties> properties;
        if (writer.QueryInterface(IID_ISetProperties, &properties) != S_OK)
            return false;
        const wchar_t *name = L"he";
        NWindows::NCOM::CPropVariant enabled(true);
        if (properties->SetProperties(&name, &enabled, 1) != S_OK)
            return false;
    }
    CMyComPtr<IArchiveUpdateCallback> callback = new CWriter(items, password);
    CMyComPtr2_Create<IOutStream, COutFileStream> output;
    if (!output->Create_NEW(us2fs(path.toStdWString().c_str())))
        return false;
    const HRESULT result = writer->UpdateItems(output, UInt32(items.size()), callback);
    const HRESULT closed = output->Close();
    return result == S_OK && closed == S_OK;
}
inline HRESULT Attach(ArchiveBackend &backend) {
    const auto catalog = Catalog();
    if (catalog->result() != S_OK || catalog->nativeCodecs().Libs.IsEmpty())
        return E_FAIL;
    return backend.loadCodecCatalog(
        QString::fromWCharArray(fs2us(catalog->nativeCodecs().Libs[0].Path)), catalog->version());
}
} // namespace NativeArchiveFixture
