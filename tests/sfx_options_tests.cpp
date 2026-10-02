#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include <QtTest>

class SfxOptionsTests final : public QObject {
    Q_OBJECT
  private slots:
    void supportedMethodsAndFormat() {
        using namespace NCompressDialog;
        for (auto method : {kCopy, kLZMA, kLZMA2, kPPMd})
            QVERIFY(IsMethodSupportedBySfx(method));
        for (auto method : {kBZip2, kDeflate, kDeflate64})
            QVERIFY(!IsMethodSupportedBySfx(method));
        const auto methods = GetMethodNames(QStringLiteral("7z"), true);
        QVERIFY(methods.contains("LZMA2"));
        QVERIFY(methods.contains("LZMA"));
        QVERIFY(methods.contains("PPMd"));
        QVERIFY(!methods.contains("BZip2"));
        CCompressDialog dialog;
        dialog.Format = "7z";
        dialog.MethodID = kLZMA2;
        dialog.Info.SFXMode = true;
        QVERIFY(dialog.CheckSFXControlsEnable());
        QVERIFY(dialog.Info.SFXMode);
        dialog.MethodID = kBZip2;
        QVERIFY(!dialog.CheckSFXControlsEnable());
        QVERIFY(!dialog.Info.SFXMode);
        dialog.Format = "zip";
        dialog.MethodID = kLZMA;
        dialog.Info.SFXMode = true;
        QVERIFY(!dialog.CheckSFXControlsEnable());
        QVERIFY(!dialog.Info.SFXMode);
    }
    void namesFollowSourceTransitions() {
        CCompressDialog dialog;
        dialog.Format = "7z";
        dialog.m_ArchivePath = QStringLiteral("папка с пробелом/空 name.7z");
        dialog.Info.SFXMode = true;
        dialog.OnButtonSFX();
#ifdef _WIN32
        const QString sfx = QStringLiteral("папка с пробелом/空 name.exe");
#else
        const QString sfx = QStringLiteral("папка с пробелом/空 name");
#endif
        QCOMPARE(dialog.m_ArchivePath, sfx);
        dialog.OnButtonSFX();
        QCOMPARE(dialog.m_ArchivePath, sfx);
        dialog.Info.SFXMode = false;
        dialog.OnButtonSFX();
        QCOMPARE(dialog.m_ArchivePath, QStringLiteral("папка с пробелом/空 name.7z"));
        dialog.m_ArchivePath = QStringLiteral("папка.with.dot/.hidden.7z");
        dialog.Info.SFXMode = true;
        dialog.OnButtonSFX();
#ifdef _WIN32
        QCOMPARE(dialog.m_ArchivePath, QStringLiteral("папка.with.dot/.hidden.exe"));
#else
        QCOMPARE(dialog.m_ArchivePath, QStringLiteral("папка.with.dot/.hidden"));
#endif
    }
};
QTEST_GUILESS_MAIN(SfxOptionsTests)
#include "sfx_options_tests.moc"
