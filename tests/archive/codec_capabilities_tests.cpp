#include "7zip/UI/Common/DefaultName.h"
#include "Common/StringConvert.h"
#include "archive/codecs/LoadCodecs.h"
#include "archive/opening/OpenArchive.h"
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

class CodecCapabilitiesTests : public QObject {
    Q_OBJECT

    static FString NativePath(const QString &path) {
        return us2fs(UString(QDir::toNativeSeparators(path).toStdWString().c_str()));
    }

  private slots:
    void sourceOpenTypesPreserveParserRecursionAndOffsets() {
        CCodecs codecs;
        CArcInfoEx format;
        format.Name = L"zip";
        codecs.Formats.Add(format);
        CObjectVector<COpenType> types;
        QVERIFY(ParseOpenTypes(codecs, L"*", types));
        QCOMPARE(types.Size(), 1u);
        QVERIFY(types[0].CanReturnArc);
        QVERIFY(!types[0].CanReturnParser);
        QVERIFY(!types[0].Recursive);
        QVERIFY(ParseOpenTypes(codecs, L"#:e:s4k", types));
        QVERIFY(!types[0].CanReturnArc);
        QVERIFY(types[0].CanReturnParser);
        QVERIFY(types[0].EachPos);
        QVERIFY(types[0].MaxStartOffset_Defined);
        QCOMPARE(types[0].MaxStartOffset, UInt64(4096));
        QVERIFY(ParseOpenTypes(codecs, L"ZIP:r.*", types));
        QCOMPARE(types.Size(), 2u);
        QCOMPARE(types[0].FormatIndex, 0);
        QVERIFY(types[0].Recursive);
        for (const wchar_t *invalid : {L"missing", L"#:unknown", L"#:s16tX",
                                       L"#:s18446744073709551615t", L"zip..*", L"hash.zip"})
            QVERIFY(!ParseOpenTypes(codecs, invalid, types));
    }

    void sourceDefaultNamesRetainAdditionalExtensionAndFallback() {
        const auto name = [](const wchar_t *file, const wchar_t *ext, const wchar_t *add) {
            return QString::fromWCharArray(GetDefaultName2(file, ext, add).Ptr());
        };
        QCOMPARE(name(L"каталог с пробелом.TGZ", L"tgz", L".tar"),
                 QStringLiteral("каталог с пробелом.tar"));
        QCOMPARE(name(L"file.unknown", L"gz", L""), QStringLiteral("file"));
        QCOMPARE(name(L"without suffix", L"gz", L""), QStringLiteral("without suffix~"));
        QCOMPARE(name(L"without suffix", L"tgz", L".tar"), QStringLiteral("without suffix.tar"));
        QCOMPARE(name(L"file .gz", L"gz", L""), QStringLiteral("file"));
    }

    void extensionsRetainSourceAliasAndAdditionalNameRules() {
        CArcInfoEx format;
        format.AddExts(L"gz gzip tgz", L"* * .tar");
        QCOMPARE(format.Exts.Size(), 3u);
        QCOMPARE(format.FindExtension(L"TGZ"), 2);
        QCOMPARE(format.FindExtension(L"zip"), -1);
        QVERIFY(format.Exts[0].AddExt.IsEmpty());
        QVERIFY(format.Exts[1].AddExt.IsEmpty());
        QVERIFY(format.Exts[2].AddExt.IsEqualTo(".tar"));
        CCodecs codecs;
        format.Name = L"gzip";
        codecs.Formats.Add(format);
        QCOMPARE(codecs.FindFormatForArchiveName(L"/folder.zip/file.TGZ"), 0);
        QCOMPARE(codecs.FindFormatForArchiveName(L"/folder.gz/file"), -1);
        QCOMPARE(codecs.FindFormatForArchiveName(L"/folder/file.exe"), -1);
        QCOMPARE(codecs.FindFormatForArchiveType(L"GZIP"), 0);
        CIntVector types;
        QVERIFY(codecs.FindFormatForArchiveType(L"gzip.*", types));
        QCOMPARE(types.Size(), 2u);
        QCOMPARE(types[1], -1);
        QVERIFY(!codecs.FindFormatForArchiveType(L"gzip.missing", types));
        QVERIFY(types.IsEmpty());
    }

    void missingLibraryRecordsFailureWithoutInventingFormats() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        CCodecs codecs;
        bool loaded = true;
        QCOMPARE(codecs.LoadDll(NativePath(temporary.filePath("missing й module")), false, &loaded),
                 S_OK);
        QVERIFY(!loaded);
        QVERIFY(codecs.Formats.IsEmpty());
        QVERIFY(codecs.Libs.IsEmpty());
        QCOMPARE(codecs.Errors.Size(), 1u);
        QVERIFY(codecs.Errors[0].ErrorCode != S_OK);
    }

    void ordinaryLibraryDoesNotBecomeArchiveCodec() {
        CCodecs codecs;
        bool loaded = false;
        QCOMPARE(
            codecs.LoadDll(NativePath(QString::fromUtf8(NATIVE_LIBRARY_FIXTURE)), false, &loaded),
            S_OK);
        QVERIFY(codecs.Formats.IsEmpty());
        QVERIFY(codecs.Libs.IsEmpty());
#ifndef _WIN32
        QVERIFY(!loaded); // Legacy POSIX virtual-IUnknown ABI is rejected upstream.
        QCOMPARE(codecs.Errors.Size(), 1u);
#endif
    }

    void officialModuleSuppliesActualHandlersAndWriters() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official codec module unavailable; native capabilities remain unverified");
#else
        CMyComPtr<ICompressCodecsInfo> owner;
        auto *codecs = new CCodecs;
        owner = codecs;
        CCodecs::CReleaser releaser;
        releaser.Set(codecs);
        bool loaded = false;
        QCOMPARE(
            codecs->LoadDll(NativePath(QString::fromUtf8(SEVENZIP_CODEC_MODULE)), false, &loaded),
            S_OK);
        QVERIFY(loaded);
        QVERIFY(codecs->Errors.IsEmpty());
        QCOMPARE(codecs->Libs.Size(), 1u);
        QVERIFY(codecs->Libs[0].Version != 0);
        const int zip = codecs->FindFormatForArchiveType(L"zip");
        const int rar = codecs->FindFormatForArchiveType(L"Rar5");
        const int gzip = codecs->FindFormatForArchiveType(L"gzip");
        QVERIFY(zip >= 0 && rar >= 0 && gzip >= 0);
        QVERIFY(codecs->Formats[zip].UpdateEnabled);
        QVERIFY(!codecs->Formats[rar].UpdateEnabled);
        QVERIFY(codecs->Formats[gzip].Flags_KeepName());
        QCOMPARE(codecs->FindFormatForExtension(L"DOCX"), zip);
        const int tgz = codecs->Formats[gzip].FindExtension(L"tgz");
        QVERIFY(tgz >= 0);
        QVERIFY(codecs->Formats[gzip].Exts[tgz].AddExt.IsEqualTo(".tar"));
        CMyComPtr<IInArchive> reader;
        QCOMPARE(codecs->CreateInArchive(unsigned(rar), reader), S_OK);
        QVERIFY(reader);
        CMyComPtr<IOutArchive> writer;
        QCOMPARE(codecs->CreateOutArchive(unsigned(zip), writer), S_OK);
        QVERIFY(writer);
        reader.Release();
        writer.Release();
        codecs->Formats.Clear();
        codecs->CloseLibs();
        QVERIFY(codecs->Libs.IsEmpty());
#endif
    }
};

QTEST_GUILESS_MAIN(CodecCapabilitiesTests)
#include "codec_capabilities_tests.moc"
