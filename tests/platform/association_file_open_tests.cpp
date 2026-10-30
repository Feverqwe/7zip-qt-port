#include "platform/associations/qtassociationfileopen.h"
#include <QApplication>
#include <QFileOpenEvent>
#include <QUrl>
#include <QtTest>

class AssociationFileOpenTests final : public QObject {
    Q_OBJECT
  private slots:
    void earlyAndRepeatedLocalRequestsAreRetainedUntilTheReceiverIsReady() {
        QObject source;
        QtAssociationFileOpen receiver(source);
        QSignalSpy requests(&receiver, &QtAssociationFileOpen::OpenRequested);
        const QString path = QStringLiteral("/tmp/архив 空 с пробелами.zip");
        QFileOpenEvent early(path);
        QVERIFY(QApplication::sendEvent(&source, &early));
        QCOMPARE(requests.size(), 0);
        receiver.Activate();
        receiver.Activate();
        QTRY_COMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toString(), path);
        QFileOpenEvent second(QUrl::fromLocalFile(path));
        QVERIFY(QApplication::sendEvent(&source, &second));
        QTRY_COMPARE(requests.size(), 2);
        QCOMPARE(requests[1][0].toString(), path);
    }
    void remoteUrlsAndEmptyRequestsNeverBecomeLocalArchivePaths() {
        QObject source;
        QtAssociationFileOpen receiver(source);
        receiver.Activate();
        QSignalSpy requests(&receiver, &QtAssociationFileOpen::OpenRequested);
        QFileOpenEvent remote(QUrl("https://example.invalid/archive.zip"));
        QApplication::sendEvent(&source, &remote);
        QFileOpenEvent empty(QString{});
        QApplication::sendEvent(&source, &empty);
        QApplication::processEvents();
        QCOMPARE(requests.size(), 0);
    }
};
QTEST_MAIN(AssociationFileOpenTests)
#include "association_file_open_tests.moc"
