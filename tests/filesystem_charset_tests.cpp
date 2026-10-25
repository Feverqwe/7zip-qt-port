// SPDX-License-Identifier: GPL-2.0-only
#include <QtTest>
#include <array>
extern "C" {
#include "fatreader.h"
#include "fs/nls/types.h"
int sevenzip_iso_charset(const char *);
struct nls_table *sevenzip_nls_table(int);
}

class FilesystemCharsetTests : public QObject {
    Q_OBJECT
  private slots:
    void fatShortNames() {
        QVERIFY(fat_linux_charset_available());
        // Accented OEM bytes and NT lowercase flags must survive the OS boundary.
        const unsigned char alias[11] = {0x80, 0x8e, 0x99, 0xe4, 0xe8, 'A',
                                         ' ',  ' ',  'T',  'X',  'T'};
        char output[73] = {};
        int size = fat_linux_short_name(alias, 0, output);
        QCOMPARE(QString::fromUtf8(output, size), QString::fromUtf8("ÇÄÖΣΦA.TXT"));
        size = fat_linux_short_name(alias, 0x18, output);
        QCOMPARE(QString::fromUtf8(output, size), QString::fromUtf8("çäöσφa.txt"));
    }
    void boundedAndExactConversion() {
        auto *table = sevenzip_nls_table(sevenzip_iso_charset("cp437"));
        QVERIFY(table);
        std::array<unsigned char, 8> output;
        output.fill(0xaa);
        QVERIFY(table->uni2char(0x00e9, output.data(), 0) < 0);
        QCOMPARE(output.front(), static_cast<unsigned char>(0xaa));
        QCOMPARE(table->uni2char(0x00e9, output.data(), 1), 1);
        QCOMPARE(output.front(), static_cast<unsigned char>(0x82));
        QCOMPARE(output[1], static_cast<unsigned char>(0xaa));
        // Avoid silent approximation/expansion by macOS iconv or Win32 best fit.
        QVERIFY(table->uni2char(0x00a9, output.data(), output.size()) < 0);
        QVERIFY(table->uni2char(0xd800, output.data(), output.size()) < 0);
        fat_wchar_t unit = 0;
        const unsigned char input[] = {0x82, 'a'};
        QCOMPARE(table->char2uni(input, 2, &unit), 1);
        QCOMPARE(unit, static_cast<fat_wchar_t>(0x00e9));
    }
    void malformedMultibyte() {
        auto *table = sevenzip_nls_table(sevenzip_iso_charset("cp932"));
        QVERIFY(table);
        fat_wchar_t unit = 0;
        const unsigned char truncated[] = {0x81};
        QVERIFY(table->char2uni(truncated, 1, &unit) < 0);
        const unsigned char pair[] = {0x93, 0xfa, 'a'};
        QCOMPARE(table->char2uni(pair, 3, &unit), 2);
        QCOMPARE(unit, static_cast<fat_wchar_t>(0x65e5));
        std::array<unsigned char, 3> output{0xaa, 0xaa, 0xaa};
        QVERIFY(table->uni2char(0x65e5, output.data(), 1) < 0);
        QCOMPARE(output.front(), static_cast<unsigned char>(0xaa));
    }
    void encodedUnitIsConsumedCompletely() {
        for (const char *charset : {"cp932", "euc-jp", "cp950"}) {
            auto *table = sevenzip_nls_table(sevenzip_iso_charset(charset));
            QVERIFY(table);
            for (fat_wchar_t sample : {0x00a5, 0x0100, 0x2160, 0x2170, 0x4f60, 0x65e5}) {
                unsigned char bytes[NLS_MAX_CHARSET_SIZE];
                const int size = table->uni2char(sample, bytes, sizeof(bytes));
                if (size <= 0)
                    continue;
                fat_wchar_t decoded = 0;
                QCOMPARE(table->char2uni(bytes, size, &decoded), size);
                QCOMPARE(decoded, sample);
            }
        }
    }
};
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc == 3 && QByteArray(argv[1]) == "--charset-available")
        return sevenzip_iso_charset(argv[2]) >= 0 ? 0 : 2;
    FilesystemCharsetTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "filesystem_charset_tests.moc"
