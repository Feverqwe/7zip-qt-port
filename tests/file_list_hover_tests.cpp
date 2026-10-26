#include "qtfilelisthover.h"

#include <QMouseEvent>
#include <QStandardItemModel>
#include <QTreeView>
#include <QtTest>
#include <memory>

class FileListHoverTests final : public QObject {
    Q_OBJECT

  private slots:
    void destroyingViewWithPendingHover() {
        QStandardItemModel model(1, 1);
        auto view = std::make_unique<QTreeView>();
        view->setModel(&model);
        auto *hover = new QtFileListHover(*view);
        hover->SetEnabled(true);
        view->show();
        QCoreApplication::processEvents();

        const auto position = view->visualRect(model.index(0, 0)).center();
        QMouseEvent move(QEvent::MouseMove, position, position, Qt::NoButton, Qt::NoButton,
                         Qt::NoModifier);
        QCoreApplication::sendEvent(view->viewport(), &move);
        // The viewport sends events while its parent has already unwound from
        // QAbstractItemView to QWidget. The adapter must tolerate that phase.
        view.reset();
        QCoreApplication::processEvents();
    }
};

QTEST_MAIN(FileListHoverTests)
#include "file_list_hover_tests.moc"
