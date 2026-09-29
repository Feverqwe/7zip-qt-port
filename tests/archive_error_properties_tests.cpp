#include "7zip/PropID.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "CPP/7zip/UI/FileManager/ExtractCallback.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"

#include <QTest>

using NWindows::NCOM::CPropVariant;

static QString Message(UInt32 flags) {
    return QString::fromWCharArray(GetOpenArcErrorMessage(flags));
}

class ArchiveErrorPropertiesTests : public QObject {
    Q_OBJECT

  private slots:
    void absentVersusDefinedZeroAndWideValues() {
        CPropVariant prop;
        bool defined = true;
        QCOMPARE(GetOpenArcErrorFlags(prop, &defined), UInt32(0));
        QVERIFY(!defined);
        QCOMPARE(GetOpenArcErrorFlags(prop), UInt32(0));
        prop = UInt32(0);
        QCOMPARE(GetOpenArcErrorFlags(prop, &defined), UInt32(0));
        QVERIFY(defined);
        prop = UInt32(kpv_ErrorFlags_CrcError);
        QCOMPARE(GetOpenArcErrorFlags(prop), UInt32(kpv_ErrorFlags_CrcError));
        prop = UInt64(0x100000000ULL | kpv_ErrorFlags_DataError);
        QCOMPARE(GetOpenArcErrorFlags(prop, &defined), UInt32(kpv_ErrorFlags_DataError));
        QVERIFY(defined);
        prop = UInt64(0xFFFFFFFFFFFFFFFFULL);
        QCOMPARE(GetOpenArcErrorFlags(prop), UInt32(0xFFFFFFFF));
    }

    void incompatiblePropertyThrowsAndClearsDefined() {
        CPropVariant signedProp;
        signedProp.Set_Int64(-1);
        const CPropVariant props[] = {CPropVariant(true), CPropVariant(L"й error"), signedProp};
        for (const auto &prop : props) {
            bool defined = true;
            bool threw = false;
            try {
                GetOpenArcErrorFlags(prop, &defined);
            } catch (int error) {
                threw = true;
                QCOMPARE(error, 151199);
            }
            QVERIFY(threw);
            QVERIFY(!defined);
        }
    }

    void messagesPreserveSourceOrderAndEncryptedHeaderHint() {
        QVERIFY(GetOpenArcErrorMessage(0).IsEmpty());
        QCOMPARE(Message(kpv_ErrorFlags_EncryptedHeadersError),
                 QStringLiteral("Ошибка заголовков : Неверный пароль?"));
        QCOMPARE(Message(kpv_ErrorFlags_CrcError | kpv_ErrorFlags_HeadersError),
                 QStringLiteral("Ошибка заголовков\nОшибка CRC"));
        QCOMPARE(Message(0x7FF),
                 QStringLiteral("Не является архивом\nОшибка заголовков\n"
                                "Ошибка заголовков : Неверный пароль?\n"
                                "Недоступно начало архива\nНачало архива не подтверждено\n"
                                "Неожиданный конец данных\nЕсть данные после конца содержимого\n"
                                "Неподдерживаемый метод сжатия\nНеподдерживаемая функция\n"
                                "Ошибка данных\nОшибка CRC"));
    }

    void unknownBitsSurviveAfterLocalizedMessages() {
        QCOMPARE(Message(0x80000000), QString("0x80000000"));
        QCOMPARE(Message(0xA0000000 | kpv_ErrorFlags_DataError),
                 QStringLiteral("Ошибка данных\n0xA0000000"));
        QVERIFY(Message(0xFFFFFFFF).endsWith("Ошибка CRC\n0xFFFFF800"));
        // Even though resource 3710 exists, external property ID 2710 must
        // bypass the builtin-resource path (the source cutoff is 1000).
        QCOMPARE(QString::fromWCharArray(GetNameOfProperty(2710, nullptr)), QString("2710"));
    }
};

QTEST_GUILESS_MAIN(ArchiveErrorPropertiesTests)
#include "archive_error_properties_tests.moc"
