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
        }
        if (!item.modified.isEmpty())
            fields->addRow("Изменён:", new QLabel(item.modified, this));
    } else {
        int files = 0;
        int folders = 0;
        qint64 size = 0;
        qint64 packed = 0;
        for (const PropertyItem &item : items) {
            if (item.directory)
                ++folders;
            else {
                ++files;
                size += item.size;
                packed += item.packedSize;
            }
        }
        fields->addRow("Выбрано:", new QLabel(QString::number(items.size()), this));
        fields->addRow("Файлов:", new QLabel(QString::number(files), this));
        fields->addRow("Папок:", new QLabel(QString::number(folders), this));
        fields->addRow("Размер выбранных файлов:", new QLabel(QString::number(size), this));
        if (archive)
            fields->addRow("Сжатый размер выбранных файлов:",
                           new QLabel(QString::number(packed), this));
        fields->addRow("Папка:", new QLabel(location, this));
    }
    layout->addLayout(fields);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
