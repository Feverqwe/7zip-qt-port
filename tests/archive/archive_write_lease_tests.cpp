#include "archive/resources/qtarchivewritelease.h"
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <optional>

class ArchiveWriteLeaseTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void movedLeaseRetainsOwnershipUntilRelease() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString archive = directory.filePath(QStringLiteral("архив 空.zip"));
        std::optional<QtArchiveWriteLease> retained;
        {
            auto acquired = QtArchiveWriteLease::Acquire({archive});
            QVERIFY(acquired);
            retained.emplace(std::move(*acquired));
        }
        const auto blocked = QtArchiveWriteLease::Acquire({archive});
        QVERIFY(!blocked);
        QVERIFY(!blocked.error().isEmpty());
        retained.reset();
        QVERIFY(QtArchiveWriteLease::Acquire({archive}));
    }
    void failedGroupReleasesOnlyNewlyAcquiredLocks() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString first = directory.filePath("a.zip");
        const QString second = directory.filePath("z.zip");
        const auto owner = QtArchiveWriteLease::Acquire({second});
        QVERIFY(owner);
        const auto failed = QtArchiveWriteLease::Acquire({first, second});
        QVERIFY(!failed);
        QVERIFY(QtArchiveWriteLease::Acquire({first}));
        QVERIFY(!QtArchiveWriteLease::Acquire({second}));
    }
    void volumeAliasesShareOwnership() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString archive = directory.filePath("volumes.7z");
        {
            const auto owner = QtArchiveWriteLease::Acquire({archive, archive + ".001"});
            QVERIFY(owner);
            QVERIFY(!QtArchiveWriteLease::Acquire({archive + ".002"}));
        }
        QVERIFY(QtArchiveWriteLease::Acquire({archive + ".002"}));
    }
};

QTEST_GUILESS_MAIN(ArchiveWriteLeaseTests)
#include "archive_write_lease_tests.moc"
