#include "native_property_failure_runtime.h"
#include "source_extract_fixture.h"
#include <QCoreApplication>
#include <QProcess>

namespace {
#ifdef Q_OS_WIN
constexpr const char *kEngine = "7z.exe";
constexpr const char *kModule = "7z.dll";
constexpr const char *kFacade = "Formats/native-property.dll";
#else
constexpr const char *kEngine = "7z";
constexpr const char *kModule = "7z.so";
constexpr const char *kFacade = "Formats/native-property.so";
#endif
} // namespace

bool NativePropertyFailureRuntime::prepare(const QString &prepared, const QString &facade) {
    if (!owned_.isValid())
        return false;
    runtime_ = owned_.filePath(QStringLiteral("runtime 空 with spaces"));
    control_ = owned_.filePath(QStringLiteral("native property δ.txt"));
    diskControl_ = owned_.filePath(QStringLiteral("native disk δ.txt"));
    const QDir source(prepared);
    if (!QDir().mkpath(QDir(runtime_).filePath("Formats/resources/fixture")) ||
        !QFile::copy(source.filePath(kEngine), engine()) ||
        !QFile::copy(source.filePath(kModule), module()) ||
        !QFile::copy(facade, QDir(runtime_).filePath(kFacade)))
        return false;
    return SourceExtractFixture::writeFile(
               QDir(runtime_).filePath("Formats/resources/fixture/marker.txt"),
               "fixture resource") &&
           SourceExtractFixture::writeFile(
               QDir(runtime_).filePath("Formats/resources/fixture/property-control-path.txt"),
               control_.toUtf8()) &&
           SourceExtractFixture::writeFile(
               QDir(runtime_).filePath("Formats/resources/fixture/disk-control-path.txt"),
               diskControl_.toUtf8());
}

QString NativePropertyFailureRuntime::engine() const { return QDir(runtime_).filePath(kEngine); }
QString NativePropertyFailureRuntime::module() const { return QDir(runtime_).filePath(kModule); }

bool NativePropertyFailureRuntime::createArchive(const QString &path, int count) const {
    QTemporaryDir entries;
    if (!entries.isValid())
        return false;
    QStringList names;
    for (int i = 0; i < count; ++i) {
        const QString name = QString("item %1 空.bin").arg(i, 3, 10, QLatin1Char('0'));
        if (!SourceExtractFixture::writeFile(entries.filePath(name), name.toUtf8()))
            return false;
        names << name;
    }
    QProcess create;
    create.setWorkingDirectory(entries.path());
    create.start(engine(), QStringList{"a", "-t7z", "-m0=Copy", "--", path} + names);
    return create.waitForFinished(10000) && create.exitStatus() == QProcess::NormalExit &&
           create.exitCode() == 0;
}

bool NativePropertyFailureRuntime::arm(UInt32 index, HRESULT result, UInt32 itemCount) {
    QFile::remove(control_ + ".hit");
    QFile::remove(control_ + ".open");
    return SourceExtractFixture::writeFile(
        control_, QByteArray::number(QCoreApplication::applicationPid()) + ' ' +
                      QByteArray::number(index) + ' ' + QByteArray::number(quint32(result)) + ' ' +
                      QByteArray::number(itemCount));
}

bool NativePropertyFailureRuntime::clear() {
    for (const QString &path :
         {control_, diskControl_, diskControl_ + ".hit", control_ + ".extract", control_ + ".open"})
        if (QFile::exists(path) && !QFile::remove(path))
            return false;
    return true;
}

bool NativePropertyFailureRuntime::forceDisk(bool enabled) {
    return enabled ? SourceExtractFixture::writeFile(
                         diskControl_, QByteArray::number(QCoreApplication::applicationPid()))
                   : !QFile::exists(diskControl_) || QFile::remove(diskControl_);
}

QByteArray NativePropertyFailureRuntime::diskReceipts() const {
    return SourceExtractFixture::readFile(diskControl_ + ".hit");
}

QByteArray NativePropertyFailureRuntime::extractionReceipts() const {
    return SourceExtractFixture::readFile(control_ + ".extract");
}

QByteArray NativePropertyFailureRuntime::receipts() const {
    return SourceExtractFixture::readFile(control_ + ".hit");
}

QByteArray NativePropertyFailureRuntime::openReceipts() const {
    return SourceExtractFixture::readFile(control_ + ".open");
}
