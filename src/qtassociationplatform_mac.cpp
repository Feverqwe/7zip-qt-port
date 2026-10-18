#include "Common/StringConvert.h"
#include "qtassociationplatform.h"
#include "qtlangcontrols.h"
#include "qtmacassociationnative.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QSettings>
#include <QStandardPaths>

namespace {
class MacAssociationPlatform final : public IQtAssociationPlatform {
    QHash<QString, QString> previous_;

  public:
    bool CanApply(unsigned group) const override { return group == 0 && QtMacCanSetAssociation(); }
    QString CurrentUserName() const override { return QtMacCurrentUserName(); }
    QString UnavailableReason(unsigned group) const override {
        return group != 0 ? QCoreApplication::translate(
                                "qtassociationplatform_mac",
                                "macOS supports associations for the current user only.")
                          : QCoreApplication::translate(
                                "qtassociationplatform_mac",
                                "An application bundle and macOS 12 or later are required.");
    }
    UString OwnIconPath() const override {
        return UString(QtMacOwnBundlePath().toStdWString().c_str());
    }
    QIcon OwnIcon() const override {
        if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
            return {};
        const QFileInfo bundle(QtMacOwnBundlePath());
        return bundle.isDir() && bundle.suffix() == "app" ? QFileIconProvider().icon(bundle)
                                                          : QIcon();
    }
    LONG Read(unsigned group, const UString &ext, QtAssociationRecord &record) override {
        if (group != 0)
            return 50;
        QtMacAssociationState state;
        const auto result = QtMacReadAssociation(QString::fromWCharArray(ext.Ptr()), state);
        if (result != 0)
            return result;
        const bool ours = !state.bundle.isEmpty() && state.bundle == QtMacOwnBundleIdentifier();
        record.info.ProgramKey = GetSystemString(
            UString((ours ? QString("7-Zip.bundle") : state.bundle).toStdWString().c_str()));
        record.info.IconPath = UString(state.path.toStdWString().c_str());
        record.revision = QCryptographicHash::hash(
            (state.type + '\n' + state.bundle + '\n' + state.path).toUtf8(),
            QCryptographicHash::Sha256);
        record.scope = "mac:0:" + state.type.toUtf8();
        return 0;
    }
    LONG Write(unsigned group, const UString &ext, bool associate, const QtAssociationRecord &,
               const UString &, const UString &, const UString &, int) override {
        if (!CanApply(group))
            return 5;
        const QString extension = QString::fromWCharArray(ext.Ptr());
        QtMacAssociationState state;
        const auto detected = QtMacReadAssociation(extension, state);
        if (detected != 0)
            return detected;
        QSettings history(QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
                              .filePath("association-history.ini"),
                          QSettings::IniFormat);
        const QString target =
            associate ? QtMacOwnBundlePath()
                      : previous_.value(state.type, history.value(state.type).toString());
        if (target.isEmpty())
            return 50; // macOS has no API for an empty default handler.
        const auto result = QtMacSetAssociation(extension, target);
        if (result == 0 && associate && !state.path.isEmpty() &&
            state.path != QtMacOwnBundlePath()) {
            previous_[state.type] = state.path;
            history.setValue(state.type, state.path);
            history.sync();
            if (history.status() != QSettings::NoError)
                return 5;
        }
        return result;
    }
    void NotifyChanged() override {}
};
} // namespace
std::shared_ptr<IQtAssociationPlatform> CreateNativeAssociationPlatform() {
    return std::make_shared<MacAssociationPlatform>();
}
