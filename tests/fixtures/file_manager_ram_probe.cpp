#include "CPP/7zip/UI/FileManager/Panel.h"
#include "filetable.h"
#include "mainwindow.h"
#include "qtfilemanagerstartup.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTimer>
#include <cstdio>

namespace {
unsigned ramQueries = 0;

QJsonObject OpenNested(FileTable &list, int row) {
    auto &panel = *list.sourcePanel;
    panel.Set_AutoRefresh_Mode(false);
    CMyComPtr<IFolderFolder> outer = panel.GetFolder();
    const UInt32 index = list.item(row, 0)->data(Qt::UserRole + 6).toUInt();
    const unsigned queriesBeforeOpen = ramQueries;
    const auto cached = NQtFileManager::GetRamInfo();
    panel.OpenItem(index, true, false, L"7z");
    const bool opened = panel.GetParentFolders().Size() == 2;
    bool virtualFile = false, copied = false;
    if (opened) {
        virtualFile = panel.GetParentFolders().Back().IsVirtual;
        UInt32 count = 0;
        const bool loaded = panel.GetFolder()->LoadItems() == S_OK &&
                            panel.GetFolder()->GetNumberOfItems(&count) == S_OK;
        if (loaded && count == 2) {
            CRecordVector<UInt32> indices;
            indices.Add(0);
            indices.Add(1);
            CCopyToOptions options;
            options.folder =
                ToUString(qEnvironmentVariable("SEVENZIP_RAM_OUTPUT") + QDir::separator());
            UStringVector errors;
            bool passwordDefined = false;
            UString password;
            copied = panel.CopyTo(options, indices, &errors, passwordDefined, password) == S_OK &&
                     errors.IsEmpty();
        }
        panel.OpenParentFolder();
    }
    return {{"opened", opened},
            {"virtual", virtualFile},
            {"copied", copied},
            {"restored", panel.GetFolder() == outer && panel.GetParentFolders().Size() == 1},
            {"queriesBeforeOpen", int(queriesBeforeOpen)},
            {"cachedSize", QString::number(cached.Size)},
            {"cachedDefined", cached.Defined}};
}
} // namespace

// Only this fixture's FM.cpp/PanelItemOpen.cpp platform dependency is renamed
// at compilation. Production keeps the original NSystem::GetRamSize service.
namespace NWindows {
namespace NSystem {
bool FixtureGetRamSize(size_t &size) {
    const bool later = ramQueries++ != 0;
    size = qEnvironmentVariable(later ? "SEVENZIP_RAM_LATER" : "SEVENZIP_RAM_START").toULongLong();
    return qEnvironmentVariableIntValue(later ? "SEVENZIP_RAM_DEFINED_LATER"
                                              : "SEVENZIP_RAM_DEFINED_START") != 0;
}
} // namespace NSystem
} // namespace NWindows

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    bool bound = false;
    bool timedOut = false;
    QJsonObject report;
    const unsigned pane = qEnvironmentVariableIntValue("SEVENZIP_RAM_PANE");
    const QString member = qEnvironmentVariable("SEVENZIP_RAM_MEMBER");
    QTimer messages;
    QObject::connect(&messages, &QTimer::timeout, &application, [] {
        if (auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
            message->accept();
    });
    messages.start(5);
    QTimer driver;
    QObject::connect(&driver, &QTimer::timeout, &application, [&] {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            auto *window = dynamic_cast<MainWindow *>(widget);
            if (!window)
                continue;
            auto *owner = window->findChild<QWidget *>(QString("filePanel%1").arg(pane));
            auto *list =
                owner ? dynamic_cast<FileTable *>(owner->findChild<QTableWidget *>("archiveTable"))
                      : nullptr;
            if (!list || !list->isEnabled() || !list->sourcePanel)
                continue;
            for (int row = 0; row < list->rowCount(); ++row) {
                if (list->item(row, 0)->text() != member)
                    continue;
                driver.stop();
                bound = true;
                report = OpenNested(*list, row);
                QTimer::singleShot(0, window, &QWidget::close);
                return;
            }
        }
    });
    driver.start(5);
    QTimer::singleShot(30000, &application, [&] {
        timedOut = true;
        application.quit();
    });
    const int result = RunSevenzipFileManager(application);
    report.insert("bound", bound);
    report.insert("timeout", timedOut);
    report.insert("queries", int(ramQueries));
    const QByteArray bytes = QJsonDocument(report).toJson(QJsonDocument::Compact);
    std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout);
    return timedOut ? 1 : result;
}
