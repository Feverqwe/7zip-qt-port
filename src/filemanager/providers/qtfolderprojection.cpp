#include "filemanager/providers/qtfolderprojection.h"
#include <QElapsedTimer>
#include <QPointer>

QtFolderProjection::QtFolderProjection(QObject *parent) : QObject(parent) {
    timer_.setInterval(0);
    connect(&timer_, &QTimer::timeout, this, &QtFolderProjection::batch);
}

void QtFolderProjection::cancel() {
    ++generation_;
    timer_.stop();
    folder_.Release();
    append_ = {};
    completed_ = {};
}

void QtFolderProjection::start(IFolderFolder *folder, quint32 count,
                               std::function<HRESULT(quint32)> append,
                               std::function<void(HRESULT)> completed) {
    cancel();
    folder_ = folder;
    count_ = count;
    next_ = 0;
    append_ = std::move(append);
    completed_ = std::move(completed);
    timer_.start();
}

void QtFolderProjection::batch() {
    if (!folder_)
        return;
    const quint64 generation = generation_;
    const QPointer<QtFolderProjection> guard(this);
    CMyComPtr<IFolderFolder> retained = folder_;
    const auto append = append_;
    QElapsedTimer elapsed;
    elapsed.start();
    // Include property projection and native icon/row creation in the budget.
    for (unsigned rows = 0; rows < 32 && elapsed.elapsed() < 8 && next_ < count_; ++rows) {
        const HRESULT result = append(next_++);
        if (!guard || generation != generation_)
            return;
        if (result != S_OK) {
            auto completed = std::move(completed_);
            cancel();
            if (completed)
                completed(result);
            return;
        }
    }
    if (next_ < count_)
        return;
    auto completed = std::move(completed_);
    cancel();
    if (completed)
        completed(S_OK);
}
