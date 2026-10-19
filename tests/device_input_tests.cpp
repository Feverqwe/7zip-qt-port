#include "7zip/Common/StreamUtils.h"
#include "7zip/IStream.h"
#include "Common/MyCom.h"
#include "deviceaccess.h"
#include <QtTest>
#include <cstring>

namespace {
class MemoryDevice final : public DeviceReadHandle {
  public:
    QByteArray Data;
    quint32 Sector;
    quint64 FarthestRead = 0;
    qint32 Failure = S_OK;
    quint32 MaximumTransfer = 0;
    MemoryDevice(quint32 sector, qsizetype size) : Data(size, Qt::Uninitialized), Sector(sector) {
        for (qsizetype i = 0; i < size; ++i)
            Data[i] = char(i % 251);
    }
    qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) override {
        read = 0;
        if (offset % Sector || size % Sector || quintptr(data) % Sector ||
            (MaximumTransfer && size > MaximumTransfer) || offset > quint64(Data.size()) ||
            size > quint64(Data.size()) - offset)
            return E_INVALIDARG;
        if (Failure != S_OK)
            return Failure;
        std::memcpy(data, Data.constData() + offset, size);
        read = size;
        FarthestRead = qMax(FarthestRead, offset + size);
        return S_OK;
    }
};

ExternalDevice Description(const MemoryDevice &memory) {
    ExternalDevice device;
    device.size = quint64(memory.Data.size());
    device.sectorSize = memory.Sector;
    return device;
}
} // namespace

class DeviceInputTests final : public QObject {
    Q_OBJECT

  private slots:
    void arbitraryReadsRemainAlignedAndBounded_data() {
        QTest::addColumn<quint32>("sector");
        QTest::newRow("512 byte sectors") << quint32(512);
        QTest::newRow("4096 byte sectors") << quint32(4096);
    }
    void arbitraryReadsRemainAlignedAndBounded() {
        QFETCH(quint32, sector);
        auto memory = std::make_shared<MemoryDevice>(sector, 2 * 1024 * 1024);
        CMyComPtr<IInStream> input;
        QCOMPARE(CreateDeviceInput(Description(*memory), memory, &input), qint32(S_OK));
        const UInt64 start = sector - 3;
        QCOMPARE(input->Seek(Int64(start), STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray output(sector + 17, Qt::Uninitialized);
        UInt32 count = 0;
        QCOMPARE(input->Read(output.data(), UInt32(output.size()), &count), S_OK);
        QCOMPARE(count, UInt32(output.size()));
        QCOMPARE(output, memory->Data.mid(qsizetype(start), output.size()));
        QCOMPARE(input->Seek(-7, STREAM_SEEK_END, nullptr), S_OK);
        QCOMPARE(input->Read(output.data(), UInt32(output.size()), &count), S_OK);
        QCOMPARE(count, UInt32(7));
        QCOMPARE(output.left(count), memory->Data.right(7));
        QCOMPARE(input->Read(output.data(), UInt32(output.size()), &count), S_OK);
        QCOMPARE(count, UInt32(0));
        QVERIFY(memory->FarthestRead <= quint64(memory->Data.size()));
        QVERIFY(input->Seek(-1, STREAM_SEEK_SET, nullptr) != S_OK);
    }
    void partitionViewsKeepIndependentCursorsAndRetainDevice() {
        auto memory = std::make_shared<MemoryDevice>(4096, 16 * 4096);
        std::weak_ptr<MemoryDevice> lifetime = memory;
        const QByteArray original = memory->Data;
        CMyComPtr<IInStream> input, first, second;
        QCOMPARE(CreateDeviceInput(Description(*memory), memory, &input), qint32(S_OK));
        QCOMPARE(CreateDevicePartitionInput(input, 4096, 8192, &first), qint32(S_OK));
        QCOMPARE(CreateDevicePartitionInput(input, 4 * 4096, 8192, &second), qint32(S_OK));
        input.Release();
        memory.reset();
        QVERIFY(!lifetime.expired());
        QByteArray buffer(19, Qt::Uninitialized);
        UInt32 count = 0;
        for (int round = 0; round < 3; ++round) {
            QCOMPARE(first->Read(buffer.data(), UInt32(buffer.size()), &count), S_OK);
            QCOMPARE(count, UInt32(buffer.size()));
            QCOMPARE(buffer, original.mid(4096 + round * buffer.size(), buffer.size()));
            QCOMPARE(second->Read(buffer.data(), UInt32(buffer.size()), &count), S_OK);
            QCOMPARE(buffer, original.mid(4 * 4096 + round * buffer.size(), buffer.size()));
        }
        QCOMPARE(first->Seek(-2, STREAM_SEEK_END, nullptr), S_OK);
        QCOMPARE(first->Read(buffer.data(), UInt32(buffer.size()), &count), S_OK);
        QCOMPARE(count, UInt32(2));
        QCOMPARE(buffer.left(2), original.mid(4096 + 8192 - 2, 2));
        first.Release();
        second.Release();
        QVERIFY(lifetime.expired());
    }
    void largeUnalignedReadsRespectNativeTransferLimit() {
        auto memory = std::make_shared<MemoryDevice>(4096, 3 * 1024 * 1024);
        memory->MaximumTransfer = 1024 * 1024; // The bounded elevated-reader protocol.
        CMyComPtr<IInStream> input;
        QCOMPARE(CreateDeviceInput(Description(*memory), memory, &input), qint32(S_OK));
        const quint64 offset = 4093;
        QCOMPARE(input->Seek(Int64(offset), STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray output(1024 * 1024 + 17, Qt::Uninitialized);
        size_t count = size_t(output.size());
        QCOMPARE(ReadStream(input, output.data(), &count), S_OK);
        QCOMPARE(count, size_t(output.size()));
        QCOMPARE(output, memory->Data.mid(qsizetype(offset), output.size()));
    }
    void disconnectionIsFailureAndNeverSuccessfulEmptyRead() {
        auto memory = std::make_shared<MemoryDevice>(512, 4096);
        CMyComPtr<IInStream> input;
        QCOMPARE(CreateDeviceInput(Description(*memory), memory, &input), qint32(S_OK));
        memory->Failure = E_FAIL;
        char output[20];
        UInt32 count = 9;
        QCOMPARE(input->Read(output, sizeof(output), &count), E_FAIL);
        QCOMPARE(count, UInt32(0));
        memory->Failure = E_ABORT;
        QCOMPARE(input->Read(output, sizeof(output), &count), E_ABORT);
        memory->Failure = S_OK;
        QCOMPARE(input->Read(output, sizeof(output), &count), S_OK);
        QCOMPARE(QByteArray(output, count), memory->Data.left(sizeof(output)));
    }
    void rejectsInvalidPartitionWithoutTouchingDevice() {
        auto memory = std::make_shared<MemoryDevice>(512, 4096);
        CMyComPtr<IInStream> input, partition;
        QCOMPARE(CreateDeviceInput(Description(*memory), memory, &input), qint32(S_OK));
        QCOMPARE(CreateDevicePartitionInput(input, 4095, 2, &partition), qint32(E_INVALIDARG));
        QCOMPARE(CreateDevicePartitionInput(input, quint64(-1), 1, &partition),
                 qint32(E_INVALIDARG));
        QVERIFY(!partition);
        QCOMPARE(memory->FarthestRead, quint64(0));
    }
    void rejectsViewsWhoseAlignedReadsWouldReachAnotherPartition() {
        auto memory = std::make_shared<MemoryDevice>(4096, 4 * 4096);
        CMyComPtr<IInStream> input, partition;
        QCOMPARE(CreateDeviceInput(Description(*memory), memory, &input), qint32(S_OK));
        // A logical bounded view cannot prevent native sector rounding from
        // reading its neighbour unless both selected boundaries are aligned.
        QCOMPARE(CreateDevicePartitionInput(input, 512, 4096, &partition), qint32(E_INVALIDARG));
        QVERIFY(!partition);
        QCOMPARE(CreateDevicePartitionInput(input, 4096, 4097, &partition), qint32(E_INVALIDARG));
        QVERIFY(!partition);
        QCOMPARE(memory->FarthestRead, quint64(0));
    }
    void journalReadLeaseKeepsTheSelectedPartitionBounds() {
        auto memory = std::make_shared<MemoryDevice>(4096, 4 * 4096);
        CMyComPtr<IInStream> input, selected, withoutReplay;
        QCOMPARE(CreateDeviceInput(Description(*memory), memory, &input), qint32(S_OK));
        QCOMPARE(CreateDevicePartitionInput(input, 4096, 8192, &selected), qint32(S_OK));
        QCOMPARE(CreateDeviceInputIgnoringJournal(selected, &withoutReplay), qint32(S_OK));
        input.Release();
        QByteArray output(3 * 4096, Qt::Uninitialized);
        size_t read = size_t(output.size());
        QCOMPARE(ReadStream(withoutReplay, output.data(), &read), S_OK);
        QCOMPARE(read, size_t(8192));
        QCOMPARE(output.left(qsizetype(read)), memory->Data.mid(4096, 8192));
        QVERIFY(memory->FarthestRead <= quint64(3 * 4096));
        // Opting in must leave the original view's policy and cursor untouched.
        QVERIFY(!dynamic_cast<DirectDeviceInput *>(selected.Interface())->ignoresDeviceJournal());
        char first = 0;
        UInt32 count = 0;
        QCOMPARE(selected->Read(&first, 1, &count), S_OK);
        QCOMPARE(count, UInt32(1));
        QCOMPARE(first, memory->Data[4096]);
    }
};

QTEST_GUILESS_MAIN(DeviceInputTests)
#include "device_input_tests.moc"
