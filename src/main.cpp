#include <QAction>
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Sevenzip Desktop");

    QMainWindow window;
    window.setWindowTitle("Sevenzip Desktop");
    window.resize(900, 600);

    auto *toolbar = window.addToolBar("Архив");
    toolbar->setMovable(false);
    for (const QString &title :
         {"Добавить", "Извлечь", "Тест", "Копировать", "Переместить", "Удалить", "Информация"}) {
        auto *action = toolbar->addAction(title);
        action->setEnabled(false);
    }

    auto *central = new QWidget(&window);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(new QLabel("Содержимое архива"));

    auto *table = new QTableWidget(0, 5, central);
    table->setHorizontalHeaderLabels({"Имя", "Размер", "Сжатый", "Изменён", "Атрибуты"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table);

    window.setCentralWidget(central);
    window.statusBar()->showMessage("Каркас интерфейса — операции с архивами ещё не подключены");
    window.show();
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
