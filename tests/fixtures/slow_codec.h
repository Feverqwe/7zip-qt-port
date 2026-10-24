#pragma once
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

class SlowCodecModuleFixture {
    const QByteArray entered_ = qgetenv("SEVENZIP_SLOW_CODEC_ENTERED");
    const QByteArray finished_ = qgetenv("SEVENZIP_SLOW_CODEC_FINISHED");
    const QByteArray delay_ = qgetenv("SEVENZIP_SLOW_CODEC_DELAY");
    static void Restore(const char *key, const QByteArray &value) {
        if (value.isNull())
            qunsetenv(key);
        else
            qputenv(key, value);
    }

  public:
    const QString Entered, Finished, Path;
    const bool Valid;
    SlowCodecModuleFixture(const QTemporaryDir &temporary, int delay)
        : Entered(temporary.filePath(QStringLiteral("начало 空"))),
          Finished(temporary.filePath(QStringLiteral("конец 空"))),
          Path(temporary.filePath(QFileInfo(QString::fromUtf8(SLOW_CODEC_FIXTURE)).fileName())),
          Valid(QFile::copy(QString::fromUtf8(SLOW_CODEC_FIXTURE), Path)) {
        qputenv("SEVENZIP_SLOW_CODEC_ENTERED", Entered.toUtf8().toBase64());
        qputenv("SEVENZIP_SLOW_CODEC_FINISHED", Finished.toUtf8().toBase64());
        qputenv("SEVENZIP_SLOW_CODEC_DELAY", QByteArray::number(delay));
    }
    ~SlowCodecModuleFixture() {
        Restore("SEVENZIP_SLOW_CODEC_ENTERED", entered_);
        Restore("SEVENZIP_SLOW_CODEC_FINISHED", finished_);
        Restore("SEVENZIP_SLOW_CODEC_DELAY", delay_);
    }
};
