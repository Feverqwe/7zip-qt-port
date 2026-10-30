#include "Common/StringConvert.h"
#include "localization/qtlangcontrols.h"
#include "platform/associations/qtassociationplatform.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QSettings>
#include <ShlObj.h>

namespace {
QByteArray registryRevision(unsigned group, const UString &ext,
                            const NRegistryAssoc::CShellExtInfo &info) {
    const QString root = group == 0 ? "HKEY_CURRENT_USER" : "HKEY_LOCAL_MACHINE";
    // QSettings opens its registry root for writing even when only reading.
    // Anchor it at the existing hive and traverse missing keys read-only.
    QSettings classes(root, QSettings::NativeFormat);
    classes.beginGroup("Software/Classes");
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    const QString extension = QString::fromWCharArray(ext.Ptr());
    for (const QString &path :
         {"." + extension, QString::fromWCharArray(info.ProgramKey.Ptr()), "7-Zip." + extension}) {
        if (path.isEmpty())
            continue;
        classes.beginGroup(path);
        for (const QString &key : classes.allKeys())
            stream << path << key << classes.value(key);
        classes.endGroup();
    }
    if (group == 0) {
        QSettings choice(root, QSettings::NativeFormat);
        choice.beginGroup("Software/Microsoft/Windows/CurrentVersion/Explorer/FileExts/." +
                          extension + "/UserChoice");
        for (const QString &key : choice.allKeys())
            stream << key << choice.value(key);
    }
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}
class WindowsAssociationPlatform final : public IQtAssociationPlatform {
  public:
    bool CanApply(unsigned group) const override { return group < 2; }
    QString CurrentUserName() const override {
        // SystemPage::OnInit's original Unicode username branch.
        const unsigned kSize = 256;
        DWORD size = kSize;
        UString s;
        const BOOL res = GetUserNameW(s.GetBuf(size), &size);
        s.ReleaseBuf_CalcLen(size < kSize ? size : kSize);
        return res ? QString::fromWCharArray(s.Ptr()) : QString();
    }
    QString UnavailableReason(unsigned) const override { return {}; }
    QString ApplyExplanation(unsigned) const override {
        return QCoreApplication::translate(
            "qtassociationplatform_win",
            "Registers the 7-Zip handler. Windows may keep your default app choice; choose the "
            "default app in Windows Settings if needed.");
    }
    UString OwnIconPath() const override {
        return UString(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())
                           .toStdWString()
                           .c_str());
    }
    LONG Read(unsigned group, const UString &ext, QtAssociationRecord &record) override {
        if (group > 1)
            return ERROR_NOT_SUPPORTED;
        record.info.ReadFromRegistry(group == 0 ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
                                     GetSystemString(ext));
        record.revision = registryRevision(group, ext, record.info);
        return 0;
    }
    LONG Write(unsigned group, const UString &ext, bool associate, const QtAssociationRecord &,
               const UString &title, const UString &command, const UString &iconPath,
               int iconIndex) override {
        if (group > 1)
            return ERROR_NOT_SUPPORTED;
        const HKEY key = group == 0 ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
        // Preserve source registry and per-format DLL icon registration.
        // UserChoice belongs to Windows Settings and is never overwritten.
        return associate ? NRegistryAssoc::AddShellExtensionInfo(
                               key, GetSystemString(ext), title, command,
                               iconPath.IsEmpty() ? OwnIconPath() : iconPath,
                               iconIndex < 0 ? 0 : iconIndex)
                         : NRegistryAssoc::DeleteShellExtensionInfo(key, GetSystemString(ext));
    }
    void NotifyChanged() override {
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    }
};
} // namespace
std::shared_ptr<IQtAssociationPlatform> CreateNativeAssociationPlatform() {
    return std::make_shared<WindowsAssociationPlatform>();
}
