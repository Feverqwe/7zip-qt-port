#include "7zip/UI/Common/ArchiveCommandLine.h"
#include "Common/CommandLineParser.h"
#include <QProcess>
#include <QUuid>
#include <QtTest>
#include <cstring>

namespace {
// Disposable named objects use the original marker/string/event wire format.
// The public parser, rather than a second decoder, consumes these fixtures.
class MappingFixture {
  public:
    QString mapName = QStringLiteral("Sevenzip-test-map-") + QUuid::createUuid().toString();
    QString eventName = QStringLiteral("Sevenzip-test-event-") + QUuid::createUuid().toString();
    HANDLE mapping = nullptr;
    HANDLE event = nullptr;
    QByteArray bytes;
    explicit MappingFixture(const QByteArray &data) : bytes(data) {
        event = CreateEventW(nullptr, TRUE, FALSE, reinterpret_cast<LPCWSTR>(eventName.utf16()));
        mapping =
            CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                               DWORD(bytes.size()), reinterpret_cast<LPCWSTR>(mapName.utf16()));
        if (mapping) {
            void *view = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, SIZE_T(bytes.size()));
            if (view) {
                std::memcpy(view, bytes.constData(), size_t(bytes.size()));
                UnmapViewOfFile(view);
            } else {
                CloseHandle(mapping);
                mapping = nullptr;
            }
        }
    }
    ~MappingFixture() {
        if (mapping)
            CloseHandle(mapping);
        if (event)
            CloseHandle(event);
    }
    QString descriptor() const {
        return mapName + QLatin1Char(':') + QString::number(bytes.size()) + QLatin1Char(':') +
               eventName;
    }
};

QByteArray utf16Names(const QStringList &names) {
    QByteArray bytes(2, '\0');
    for (const auto &name : names) {
        bytes.append(reinterpret_cast<const char *>(name.utf16()), name.size() * 2);
        bytes.append(2, '\0');
    }
    return bytes;
}

void parse(const QStringList &args, CArcCmdLineOptions &options) {
    UStringVector strings;
    for (const auto &arg : args)
        strings.Add(arg.toStdWString().c_str());
    CArcCmdLineParser parser;
    parser.Parse1(strings, options);
    parser.Parse2(options);
}

bool includesPath(const NWildcard::CCensor &censor, const QString &path) {
    FOR_VECTOR(i, censor.Pairs) {
        const auto &pair = censor.Pairs[i];
        const QString prefix = QString::fromWCharArray(pair.Prefix);
        if (!path.startsWith(prefix, Qt::CaseInsensitive))
            continue;
        UStringVector parts;
        SplitPathToParts(path.mid(prefix.size()).toStdWString().c_str(), parts);
        bool included = false;
        if (pair.Head.CheckPathVect(parts, true, included) && included)
            return true;
    }
    return false;
}
} // namespace

class NativeArchiveCommandLineTests final : public QObject {
    Q_OBJECT
  private slots:
    void mappedSelections_data() {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("include");
        QTest::newRow("extract") << QStringLiteral("x") << QStringLiteral("-ai#");
        QTest::newRow("test") << QStringLiteral("t") << QStringLiteral("-ai#");
        QTest::newRow("add") << QStringLiteral("a") << QStringLiteral("-i#");
        QTest::newRow("hash") << QStringLiteral("h") << QStringLiteral("-i#");
    }
    void mappedSelections() {
        QFETCH(QString, command);
        QFETCH(QString, include);
        const QStringList paths{QStringLiteral("C:\\исходные файлы\\архив ①.zip"),
                                QStringLiteral("C:\\исходные файлы\\日本語 ②.7z")};
        MappingFixture fixture(utf16Names(paths));
        QVERIFY(fixture.mapping);
        QVERIFY(fixture.event);
        QStringList args{command, include + fixture.descriptor()};
        if (include == QStringLiteral("-ai#"))
            args << QStringLiteral("-an");
        else if (command == QStringLiteral("a"))
            args << QStringLiteral("--") << QStringLiteral("C:\\архивы\\результат.7z");
        CArcCmdLineOptions options;
        parse(args, options);
        QCOMPARE(WaitForSingleObject(fixture.event, 0), DWORD(WAIT_OBJECT_0));
        const auto &censor = include == QStringLiteral("-ai#") ? options.arcCensor : options.Censor;
        // Update keeps pre-items for its Add dialog; other commands publish the
        // source censor tree and consume that temporary list during Parse2.
        if (command == QStringLiteral("a")) {
            QCOMPARE(censor.CensorPaths.Size(), unsigned(paths.size()));
            for (unsigned i = 0; i < censor.CensorPaths.Size(); ++i) {
                QCOMPARE(QString::fromWCharArray(censor.CensorPaths[i].Path), paths[int(i)]);
                QVERIFY(censor.CensorPaths[i].Include);
            }
        } else {
            for (const auto &path : paths)
                QVERIFY2(includesPath(censor, path), qPrintable(path));
            QVERIFY(!includesPath(censor, QStringLiteral("C:\\исходные файлы\\другой.zip")));
        }
        if (command == QStringLiteral("a"))
            QCOMPARE(QString::fromWCharArray(options.ArchiveName),
                     QStringLiteral("C:\\архивы\\результат.7z"));
    }
    void malformedMapping_data() {
        QTest::addColumn<QByteArray>("payload");
        QTest::addColumn<QString>("sizeOverride");
        QTest::newRow("marker") << QByteArray("x\0\0\0", 4) << QString();
        QTest::newRow("unterminated") << QByteArray("\0\0x\0", 4) << QString();
        QTest::newRow("odd-size") << QByteArray(4, '\0') << QStringLiteral("3");
        QTest::newRow("zero-size") << QByteArray(4, '\0') << QStringLiteral("0");
        QTest::newRow("oversize") << QByteArray(4, '\0') << QStringLiteral("2147483650");
        QTest::newRow("bad-size") << QByteArray(4, '\0') << QStringLiteral("invalid");
    }
    void malformedMapping() {
        QFETCH(QByteArray, payload);
        QFETCH(QString, sizeOverride);
        MappingFixture fixture(payload);
        QVERIFY(fixture.mapping);
        QVERIFY(fixture.event);
        const QString descriptor =
            fixture.mapName + QLatin1Char(':') +
            (sizeOverride.isEmpty() ? QString::number(payload.size()) : sizeOverride) +
            QLatin1Char(':') + fixture.eventName;
        CArcCmdLineOptions options;
        QVERIFY_EXCEPTION_THROWN(
            parse({QStringLiteral("t"), QStringLiteral("-an"), QStringLiteral("-ai#") + descriptor},
                  options),
            CArcCmdLineException);
        // The original scope guard releases the sender even when parsing fails.
        QCOMPARE(WaitForSingleObject(fixture.event, 0), DWORD(WAIT_OBJECT_0));
    }
    void unavailableMapping() {
        MappingFixture fixture(QByteArray(4, '\0'));
        QVERIFY(fixture.mapping);
        QVERIFY(fixture.event);
        CloseHandle(fixture.mapping);
        fixture.mapping = nullptr;
        CArcCmdLineOptions options;
        QVERIFY_EXCEPTION_THROWN(parse({QStringLiteral("t"), QStringLiteral("-an"),
                                        QStringLiteral("-ai#") + fixture.descriptor()},
                                       options),
                                 CArcCmdLineException);
        QCOMPARE(WaitForSingleObject(fixture.event, 0), DWORD(WAIT_OBJECT_0));
    }
    void quotedNativeCommandLine() {
        UStringVector strings;
        NCommandLineParser::SplitCommandLine(
            L"\"C:\\Sevenzip Desktop\\7zG.exe\" a -ad -- \"C:\\архивы\\мой архив.7z\"", strings);
        QCOMPARE(strings.Size(), 5u);
        QCOMPARE(QString::fromWCharArray(strings[0]),
                 QStringLiteral("C:\\Sevenzip Desktop\\7zG.exe"));
        strings.Delete(0);
        CArcCmdLineOptions options;
        CArcCmdLineParser parser;
        parser.Parse1(strings, options);
        parser.Parse2(options);
        QVERIFY(options.ShowDialog);
        QCOMPARE(options.Command.CommandType, NCommandType::kAdd);
        QCOMPARE(QString::fromWCharArray(options.ArchiveName),
                 QStringLiteral("C:\\архивы\\мой архив.7z"));
    }
    void nativeHelperRejectsMalformedMapping() {
        MappingFixture fixture(QByteArray("x\0\0\0", 4));
        QVERIFY(fixture.mapping);
        QVERIFY(fixture.event);
        QProcess helper;
        helper.setProgram(QString::fromUtf8(NATIVE_GUI_EXECUTABLE));
        helper.setArguments({QStringLiteral("t"), QStringLiteral("-y"), QStringLiteral("-an"),
                             QStringLiteral("-ai#") + fixture.descriptor()});
        helper.start();
        QVERIFY(helper.waitForStarted(5000));
        QVERIFY(helper.waitForFinished(10000));
        QCOMPARE(helper.exitStatus(), QProcess::NormalExit);
        QCOMPARE(helper.exitCode(), 7); // original CMessagePathException policy
        QCOMPARE(WaitForSingleObject(fixture.event, 0), DWORD(WAIT_OBJECT_0));
    }
};

QTEST_GUILESS_MAIN(NativeArchiveCommandLineTests)
#include "native_archive_command_line_tests.moc"
