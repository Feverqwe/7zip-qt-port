#include "qtlanguageservices.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "Common/Lang.h"
#include "qtownedtempdir.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>

static void InitLanguageResources() { Q_INIT_RESOURCE(upstream_languages); }

namespace {
class BundledLanguageFiles {
    QtOwnedTempDir _directory;
    CLang _english;

  public:
    BundledLanguageFiles() {
        InitLanguageResources();
        if (!_directory.isValid())
            return;
        for (const QString &name : {QStringLiteral("en.ttt"), QStringLiteral("ru.txt")})
            QFile::copy(":/upstream/7zip/Lang/" + name, _directory.filePath(name));
        const std::wstring path = _directory.filePath("en.ttt").toStdWString();
        _english.Open(us2fs(UString(path.c_str())), "7-Zip");
    }
    QString path() const { return _directory.path(); }
    UString english(UInt32 id) const {
        const wchar_t *s = _english.Get(id);
        return s ? UString(s) : UString();
    }
};
const BundledLanguageFiles &BundledFiles() {
    static const BundledLanguageFiles files;
    return files;
}
} // namespace

QString QtLanguageDirectory() {
    const QString installed = QCoreApplication::applicationDirPath() + "/Lang";
    if (QDir(installed).exists())
        return installed;
    return BundledFiles().path();
}

UString QtEnglishLanguageString(UInt32 id) { return BundledFiles().english(id); }

QString QtLocalText(const QString &english, const QString &russian) {
    UString language;
    LangString_OnlyFromLangFile(1, language);
    return language == L"Russian" ? russian : english;
}
