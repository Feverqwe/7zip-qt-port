#include "CPP/7zip/UI/Common/HashCalc.h"
#include <QCryptographicHash>
#include <QtTest>

class NativeHashBundleTests final : public QObject {
    Q_OBJECT
  private slots:
    void crc64Provider() {
        CHashBundle bundle;
        UStringVector names;
        names.Add(L"CRC64");
        QCOMPARE(bundle.SetMethods(nullptr, names), S_OK);
        bundle.InitForNewFile();
        bundle.Update("1234", 4);
        bundle.Update("", 0);
        bundle.Update("56789", 5);
        bundle.Final(false, false, L"CRC64 空.bin");
        char text[160];
        bundle.Hashers[0].WriteToString(k_HashCalc_Index_Current, text);
        QCOMPARE(QByteArray(text), QByteArray("995DC9BBDF1939FA"));
        bundle.InitForNewFile();
        bundle.Final(false, false, L"empty");
        bundle.Hashers[0].WriteToString(k_HashCalc_Index_Current, text);
        QCOMPARE(QByteArray(text), QByteArray("0000000000000000"));
    }

    void defaultCRCAndSourceStatistics() {
        CHashBundle bundle;
        QCOMPARE(bundle.SetMethods(nullptr, UStringVector()), S_OK);
        QCOMPARE(bundle.Hashers.Size(), unsigned(1));
        QCOMPARE(QString::fromLatin1(bundle.Hashers[0].Name.Ptr()), QString("CRC32"));
        bundle.InitForNewFile();
        bundle.Update("1234", 4);
        bundle.Update("56789", 5);
        bundle.Final(false, false, L"каталог/密碼 file.bin");
        char text[160];
        bundle.Hashers[0].WriteToString(k_HashCalc_Index_Current, text);
        QCOMPARE(QByteArray(text), QByteArray("CBF43926"));
        QCOMPARE(bundle.NumFiles, UInt64(1));
        QCOMPARE(bundle.FilesSize, UInt64(9));
        bundle.InitForNewFile();
        bundle.Final(true, false, L"folder");
        QCOMPARE(bundle.NumDirs, UInt64(1));
        QCOMPARE(bundle.NumFiles, UInt64(1));
    }
    void builtinProvidersAndIncrementalDigest() {
        const QByteArray bytes = QByteArray("binary\0", 7) + QByteArray(32768, char(0xff));
        for (const auto &method : {qMakePair("MD5", QCryptographicHash::Md5),
                                   qMakePair("SHA1", QCryptographicHash::Sha1),
                                   qMakePair("SHA256", QCryptographicHash::Sha256),
                                   qMakePair("SHA384", QCryptographicHash::Sha384),
                                   qMakePair("SHA512", QCryptographicHash::Sha512),
                                   qMakePair("SHA3-256", QCryptographicHash::Sha3_256)}) {
            CHashBundle bundle;
            UStringVector names;
            names.Add(UString(method.first));
            QCOMPARE(bundle.SetMethods(nullptr, names), S_OK);
            bundle.InitForNewFile();
            bundle.Update(bytes.constData(), 7);
            bundle.Update(bytes.constData() + 7, bytes.size() - 7);
            bundle.Final(false, false, L"Unicode 密碼.bin");
            char text[160];
            bundle.Hashers[0].WriteToString(k_HashCalc_Index_Current, text);
            QCOMPARE(QByteArray(text), QCryptographicHash::hash(bytes, method.second).toHex());
        }
        CHashBundle all;
        UStringVector wildcard;
        wildcard.Add(UString("*"));
        QCOMPARE(all.SetMethods(nullptr, wildcard), S_OK);
        QStringList names;
        FOR_VECTOR(i, all.Hashers)
        names.append(QString::fromLatin1(all.Hashers[i].Name.Ptr()));
        for (const auto *name : {"CRC32", "CRC64", "CKSUM", "XXH64", "BLAKE2sp", "SHA256"})
            QVERIFY2(names.contains(name), name);
        UStringVector missing;
        missing.Add(UString("unsupported-test-method"));
        CHashBundle unsupported;
        QVERIFY(unsupported.SetMethods(nullptr, missing) != S_OK);
    }
};
QTEST_GUILESS_MAIN(NativeHashBundleTests)
#include "native_hash_bundle_tests.moc"
