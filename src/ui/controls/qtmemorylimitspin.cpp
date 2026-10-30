#include "ui/controls/qtmemorylimitspin.h"
#include <QLineEdit>
#include <algorithm>

QtMemoryLimitSpin::QtMemoryLimitSpin(QWidget *parent) : QAbstractSpinBox(parent) {
    lineEdit()->setObjectName("extractMemoryEdit");
    connect(lineEdit(), &QLineEdit::textChanged, this, &QtMemoryLimitSpin::textChanged);
}

void QtMemoryLimitSpin::setRange(quint32 minimum, quint32 maximum) {
    _minimum = minimum;
    _maximum = maximum;
}

void QtMemoryLimitSpin::setText(const QString &text) { lineEdit()->setText(text); }

void QtMemoryLimitSpin::stepBy(int steps) {
    bool valid = false;
    const qint64 current = lineEdit()->text().toLongLong(&valid);
    const qint64 value = valid ? std::clamp(current, qint64(_minimum), qint64(_maximum)) : _minimum;
    setText(QString::number(std::clamp(value + steps, qint64(_minimum), qint64(_maximum))));
}

QAbstractSpinBox::StepEnabled QtMemoryLimitSpin::stepEnabled() const {
    bool valid = false;
    const quint64 value = lineEdit()->text().toULongLong(&valid);
    if (!valid)
        return StepUpEnabled | StepDownEnabled;
    StepEnabled enabled = StepNone;
    if (value > _minimum)
        enabled |= StepDownEnabled;
    if (value < _maximum)
        enabled |= StepUpEnabled;
    return enabled;
}
