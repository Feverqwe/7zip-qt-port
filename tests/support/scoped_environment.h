#pragma once
#include <QString>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class ScopedTestEnvironment final {
    const QByteArray key_;
#ifdef Q_OS_WIN
    const bool present_ = qEnvironmentVariableIsSet(key_.constData());
    const QString old_ = qEnvironmentVariable(key_.constData());
    void set(const QString *value) const {
        const auto key = QString::fromLatin1(key_).toStdWString();
        const auto text = value ? value->toStdWString() : std::wstring();
        // CreateProcessW inherits the Unicode OS environment. qputenv's byte
        // input goes through the Windows code page and corrupts UTF-8 paths.
        SetEnvironmentVariableW(key.c_str(), value ? text.c_str() : nullptr);
    }
#else
    const QByteArray old_ = qgetenv(key_.constData());
#endif

  public:
    ScopedTestEnvironment(const char *key, const QString &value) : key_(key) {
#ifdef Q_OS_WIN
        set(&value);
#else
        qputenv(key, value.toUtf8());
#endif
    }
    ~ScopedTestEnvironment() {
#ifdef Q_OS_WIN
        set(present_ ? &old_ : nullptr);
#else
        if (old_.isNull())
            qunsetenv(key_.constData());
        else
            qputenv(key_.constData(), old_);
#endif
    }
};
