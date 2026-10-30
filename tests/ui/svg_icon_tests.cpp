#include <QFile>
#include <QIcon>
#include <QTemporaryDir>
#include <QtTest>

class SvgIconTests final : public QObject {
    Q_OBJECT
  private slots:
    void rendersSvgIcon() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QFile source(temporary.filePath("icon.svg"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        const QByteArray svg =
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\">"
            "<rect width=\"16\" height=\"16\" fill=\"red\"/></svg>";
        QCOMPARE(source.write(svg), svg.size());
        source.close();

        const auto image = QIcon(source.fileName()).pixmap(16, 16).toImage();
        QVERIFY2(!image.isNull(), "The managed Qt runtime must retain its SVG icon renderer");
        QCOMPARE(image.pixelColor(8, 8), QColor(Qt::red));
    }
};

QTEST_MAIN(SvgIconTests)
#include "svg_icon_tests.moc"
