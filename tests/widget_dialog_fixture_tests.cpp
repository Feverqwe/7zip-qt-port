#include "fixtures/widget_dialogs.h"
#include <QApplication>
#include <QEvent>
#include <QtTest>

class WidgetDialogFixtureTests final : public QObject {
    Q_OBJECT
  private slots:
    void nestedAnswerDoesNotReenterParent() {
        WidgetDialogFixture fixture;
        QDialog parent;
        QDialog child(&parent);
        parent.resize(240, 120);
        child.resize(180, 100);
        QTimer responder;
        int parentAnswers = 0, childAnswers = 0;
        bool timedOut = false;
        int childResult = QDialog::Rejected;
        fixture.connectResponder(responder, &parent, [&] {
            auto *dialog = fixture.activeDialog();
            if (dialog == &child) {
                ++childAnswers;
                child.accept();
                return;
            }
            if (dialog != &parent)
                return;
            if (++parentAnswers != 1)
                return;
            // Geometry/event processing can deliver another timer callback
            // while this answer is still handling the same modal dialog.
            QMetaObject::invokeMethod(&responder, "timeout", Qt::DirectConnection);
            QCoreApplication::sendPostedEvents(&parent, QEvent::MetaCall);
            // A distinct nested question must still receive its own answer.
            childResult = child.exec();
            responder.stop();
            parent.accept();
        });
        QTimer::singleShot(2000, &parent, [&] {
            timedOut = true;
            child.reject();
            parent.reject();
        });
        responder.start(5);
        QCOMPARE(parent.exec(), int(QDialog::Accepted));
        QVERIFY(!timedOut);
        QCOMPARE(childResult, int(QDialog::Accepted));
        QCOMPARE(parentAnswers, 1);
        QCOMPARE(childAnswers, 1);
    }
};

QTEST_MAIN(WidgetDialogFixtureTests)
#include "widget_dialog_fixture_tests.moc"
