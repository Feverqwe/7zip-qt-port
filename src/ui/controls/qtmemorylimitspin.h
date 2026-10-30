#pragma once
#include <QAbstractSpinBox>

// Win32 edit + up/down presentation adapter. The source SettingsPage validates
// the unaltered edit text on Apply; arrows alone obey the RAM-derived range.
class QtMemoryLimitSpin final : public QAbstractSpinBox {
    Q_OBJECT
    quint32 _minimum = 1;
    quint32 _maximum = 64;
    void stepBy(int steps) override;
    StepEnabled stepEnabled() const override;

  public:
    explicit QtMemoryLimitSpin(QWidget *parent = nullptr);
    void setRange(quint32 minimum, quint32 maximum);
    void setText(const QString &text);

  signals:
    void textChanged();
};
