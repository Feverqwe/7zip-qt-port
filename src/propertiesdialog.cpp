#include "propertiesdialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"

PropertiesDialog::PropertiesDialog(const QVector<PropertyItem> &items, bool archive,
                                   const QString &location, QWidget *parent)
    : CListViewDialog(parent) {
    CObjectVector<CPanelItem> snapshot;
    CRecordVector<UInt32> indices;
    for (const PropertyItem &item : items) {
        CPanelItem entry;
        entry.RelPath = item.path;
        entry.IsFolder = item.directory;
        entry.SizeDefined = !item.directory || archive;
        entry.Size = (UInt64)item.size;
        entry.PackSize = (UInt64)item.packedSize;
        entry.NumSubDirs = (UInt64)item.nestedFolders;
        entry.NumSubFiles = (UInt64)item.nestedFiles;
        auto add = [&](const QString &name, const QString &value) {
            entry.Properties.Add({ToUString(name), ToUString(value)});
        };
        add(QStringLiteral("Имя"), item.name);
        add(QStringLiteral("Путь"), item.path);
        add(QStringLiteral("Тип"),
            item.directory ? QStringLiteral("Папка") : QStringLiteral("Файл"));
        if (entry.SizeDefined)
            add(QStringLiteral("Размер"), ConvertSizeToString(entry.Size));
        if (archive) {
            add(QStringLiteral("Сжатый размер"), ConvertSizeToString(entry.PackSize));
            if (item.directory) {
                add(QStringLiteral("Папок"), ConvertSizeToString(entry.NumSubDirs));
                add(QStringLiteral("Файлов"), ConvertSizeToString(entry.NumSubFiles));
            }
        }
        add(QStringLiteral("Изменён"), item.modified);
        indices.Add(snapshot.Size());
        snapshot.Add(entry);
    }
    CPanel panel(location, snapshot);
    panel.Properties(indices, archive, *this);
    OnInit();
    setObjectName("selectedPropertiesDialog");
}
