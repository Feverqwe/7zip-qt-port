#include "propertiesdialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

PropertiesDialog::PropertiesDialog(const QVector<PropertyItem> &items, bool archive,
                                   const QString &location, QWidget *parent)
    : QDialog(parent) {
    setObjectName("selectedPropertiesDialog");
    setWindowTitle(items.size() == 1 ? "Свойства: " + items.first().name : "Свойства выделения");
    auto *layout = new QVBoxLayout(this);
    auto *fields = new QFormLayout;
    if (items.size() == 1) {
        const PropertyItem &item = items.first();
        fields->addRow("Имя:", new QLabel(item.name, this));
        fields->addRow("Путь:", new QLabel(item.path, this));
        fields->addRow("Тип:", new QLabel(item.directory ? "Папка" : "Файл", this));
        if (!item.directory) {
            fields->addRow("Размер:", new QLabel(QString::number(item.size), this));
            if (archive)
                fields->addRow("Сжатый:", new QLabel(QString::number(item.packedSize), this));
        } else if (archive) {
            fields->addRow("Файлов внутри:", new QLabel(QString::number(item.nestedFiles), this));
            fields->addRow("Размер содержимого:", new QLabel(QString::number(item.size), this));
            fields->addRow("Сжатый размер содержимого:",
                           new QLabel(QString::number(item.packedSize), this));
        }
        if (!item.modified.isEmpty())
            fields->addRow("Изменён:", new QLabel(item.modified, this));
    } else {
        int files = 0;
        int folders = 0;
        qint64 size = 0;
        qint64 packed = 0;
        bool foldersSelected = false;
        for (const PropertyItem &item : items) {
            if (item.directory)
                ++folders;
            else
                ++files;
            foldersSelected |= item.directory;
            bool covered = false;
            if (archive) {
                for (const PropertyItem &parent : items) {
                    if (parent.directory && item.path != parent.path &&
                        item.path.startsWith(parent.path + '/')) {
                        covered = true;
                        break;
                    }
                }
            }
            if (!covered) {
                size += item.size;
                packed += item.packedSize;
            }
        }
        fields->addRow("Выбрано:", new QLabel(QString::number(items.size()), this));
        fields->addRow("Файлов:", new QLabel(QString::number(files), this));
        fields->addRow("Папок:", new QLabel(QString::number(folders), this));
        fields->addRow(foldersSelected && archive ? "Размер файлов и содержимого папок:"
                                                  : "Размер выбранных файлов:",
                       new QLabel(QString::number(size), this));
        if (archive)
            fields->addRow(foldersSelected ? "Сжатый размер файлов и содержимого папок:"
                                           : "Сжатый размер выбранных файлов:",
                           new QLabel(QString::number(packed), this));
        fields->addRow("Папка:", new QLabel(location, this));
    }
    layout->addLayout(fields);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
