#pragma once

#include "archivebackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QtTest>

#include <cstdio>

static void writeOutput(const QByteArray &text) {
    std::fwrite(text.constData(), 1, size_t(text.size()), stdout);
    std::fflush(stdout);
}

class ArchiveBackendTestSupport {
  protected:
    static void writeFile(const QString &path, const QByteArray &data) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), data.size());
    }
    static void await(ArchiveBackend &backend, bool expected) {
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        if (spy.isEmpty())
            QVERIFY2(spy.wait(30000), "operation timed out");
        QCOMPARE(spy.takeFirst().at(0).toBool(), expected);
    }
};

inline int backendTestMain(int argc, char **argv, QObject *tests) {
    QCoreApplication app(argc, argv);
    const QByteArray mode = qgetenv("SEVENZIP_TEST_HELPER");
    if (!mode.isEmpty()) {
        if (mode == "fragment") {
            writeOutput("  4");
            QThread::msleep(20);
            writeOutput("2%\b\b    \b\b+ папка/имя с пробелом.txt\r");
            writeOutput("ERROR: incomplete tail");
            return 2;
        }
        if (mode == "large") {
            for (int i = 0; i < 3000; ++i) {
                writeOutput("Path = file" + QByteArray::number(i) + ".txt\nSize = 1\n\n");
                if (i == 999)
                    QThread::msleep(100);
            }
            return 0;
        }
        if (mode == "listing") {
            writeOutput(qgetenv("SEVENZIP_TEST_LISTING"));
            return 0;
        }
        if (mode == "createpartial" || mode == "createfail") {
            const QStringList arguments = QCoreApplication::arguments();
            const int separator = arguments.indexOf("--");
            if (separator < 0 || separator + 1 >= arguments.size())
                return 3;
            QFile partial(arguments.at(separator + 1));
            if (!partial.open(QIODevice::WriteOnly))
                return 4;
            partial.write("incomplete archive");
            partial.close();
            writeOutput("started\n");
            if (mode == "createfail")
                return 2;
            QThread::msleep(30000);
            return 0;
        }
        writeOutput("started\npartial");
        QThread::msleep(30000);
        return 0;
    }
    return QTest::qExec(tests, argc, argv);
}
