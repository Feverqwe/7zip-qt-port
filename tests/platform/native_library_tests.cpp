#include "Common/StringConvert.h"
#include "platform/loader/DLL.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <vector>

using NWindows::NDLL::CLibrary;

class NativeLibraryTests : public QObject {
    Q_OBJECT

    static FString nativePath(const QString &path) {
        const auto native = QDir::toNativeSeparators(path).toStdWString();
        return us2fs(native.c_str());
    }
    static QString copyFixture(const QTemporaryDir &temporary) {
        const QString original = QString::fromUtf8(NATIVE_LIBRARY_FIXTURE);
        const QString path = temporary.filePath(QStringLiteral("модуль й с пробелом.") +
                                                QFileInfo(original).suffix());
        return QFile::copy(original, path) ? path : QString();
    }
    static int fixtureValue(const CLibrary &library) {
        const auto function = reinterpret_cast<int (*)(void)>(
            ::GetProcAddress(library.Get_HMODULE(), "sevenzip_fixture_value"));
        return function ? function() : -1;
    }

  private slots:
    void emptyAndFailedLoadAreIdempotent() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        CLibrary library;
        QVERIFY(!library.IsLoaded());
        QVERIFY(library.Get_HMODULE() == NULL);
        QVERIFY(library.Free());
        QVERIFY(library.Detach() == NULL);
        CLibrary emptyCopy(library);
        QVERIFY(!emptyCopy.IsLoaded());
        QVERIFY(!library.Load(nativePath(temporary.filePath("missing module"))));
        QVERIFY(!library.IsLoaded());
        QVERIFY(library.Free());
        library.Attach(NULL);
        QVERIFY(!library.IsLoaded());
#ifndef _WIN32
        QVERIFY(::GetProcAddress(NULL, "sevenzip_fixture_value") == NULL);
#endif
    }

    void loadedLibraryTransfersAndRejectsCopy() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = copyFixture(temporary);
        QVERIFY(!path.isEmpty());
        CLibrary library;
        QVERIFY(library.Load(nativePath(path)));
        QVERIFY(library.IsLoaded());
        QCOMPARE(fixtureValue(library), 42);
        QVERIFY(::GetProcAddress(library.Get_HMODULE(), "missing_fixture_symbol") == NULL);
        bool threw = false;
        try {
            CLibrary forbidden(library);
        } catch (int) {
            threw = true;
        }
        QVERIFY(threw);
        QVERIFY(library.IsLoaded());
        QCOMPARE(fixtureValue(library), 42);
        const auto module = library.Detach();
        QVERIFY(module != NULL);
        QVERIFY(!library.IsLoaded());
        QVERIFY(library.Free());
        CLibrary destination;
        destination.Attach(module);
        QVERIFY(destination.IsLoaded());
        QCOMPARE(fixtureValue(destination), 42);
        QVERIFY(destination.Free());
        QVERIFY(!destination.IsLoaded());
        QVERIFY(destination.Free());
        QVERIFY(QFile::remove(path));
    }

    void replacingHandleReleasesPreviousOwnership() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = copyFixture(temporary);
        QVERIFY(!path.isEmpty());
        CLibrary source;
        CLibrary destination;
        QVERIFY(source.Load(nativePath(path)));
        QVERIFY(destination.Load(nativePath(path)));
        destination.Attach(source.Detach());
        QVERIFY(!source.IsLoaded());
        QCOMPARE(fixtureValue(destination), 42);
        destination.Attach(NULL);
        QVERIFY(!destination.IsLoaded());
        QVERIFY(QFile::remove(path));
    }

    void failedReloadReleasesTheOldModule() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = copyFixture(temporary);
        QVERIFY(!path.isEmpty());
        {
            CLibrary library;
            QVERIFY(library.Load(nativePath(path)));
            QCOMPARE(fixtureValue(library), 42);
            QVERIFY(!library.Load(nativePath(temporary.filePath("missing module"))));
            QVERIFY(!library.IsLoaded());
            QVERIFY(QFile::remove(path));
        }
        const QString recreated = copyFixture(temporary);
        QVERIFY(!recreated.isEmpty());
        {
            CLibrary library;
            QVERIFY(library.Load(nativePath(recreated)));
            QCOMPARE(fixtureValue(library), 42);
        }
        QVERIFY(QFile::remove(recreated));
    }

#ifdef Q_OS_WIN
    void modulePathBelongsToLoadedDllInsteadOfHostExecutable() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QStringLiteral("модуль й 密碼 с пробелом.dll"));
        QVERIFY(QFile::copy(QString::fromUtf8(NATIVE_MODULE_PATH_FIXTURE), path));
        CLibrary library;
        QVERIFY(library.Load(nativePath(path)));
        const auto modulePath = reinterpret_cast<unsigned (*)(wchar_t *, unsigned)>(
            ::GetProcAddress(library.Get_HMODULE(), "sevenzip_module_path"));
        QVERIFY(modulePath);
        const unsigned required = modulePath(nullptr, 0);
        QVERIFY(required > 1);
        std::vector<wchar_t> buffer(required, wchar_t(0));
        QCOMPARE(modulePath(buffer.data(), required), required);
        QCOMPARE(QDir::fromNativeSeparators(QString::fromWCharArray(buffer.data())), path);
        FString executable;
        QVERIFY(NWindows::NDLL::MyGetModuleFileName(executable));
        QCOMPARE(QDir::fromNativeSeparators(QString::fromWCharArray(executable.Ptr())),
                 QCoreApplication::applicationFilePath());
        QVERIFY(library.Free());
        QVERIFY(QFile::remove(path));
    }

    void windowsResourceLoadExRetainsSourceLifetime() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = copyFixture(temporary);
        QVERIFY(!path.isEmpty());
        CLibrary library;
        QVERIFY(library.LoadEx(nativePath(path)));
        QVERIFY(library.IsLoaded());
        const auto module = library.Detach();
        CLibrary destination;
        destination.Attach(module);
        QVERIFY(destination.IsLoaded());
        QVERIFY(destination.Free());
        QVERIFY(QFile::remove(path));
    }
#endif
};

QTEST_GUILESS_MAIN(NativeLibraryTests)
#include "native_library_tests.moc"
