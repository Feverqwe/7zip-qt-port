#pragma once
#include <QDialog>
#include <QVector>
class QComboBox;
class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;
class QVBoxLayout;
class QGroupBox;
class QScrollArea;

class FscryptKeyDialog final : public QDialog {
    Q_OBJECT
    struct Row {
        QGroupBox *group;
        QComboBox *version;
        QLineEdit *key;
        QLineEdit *descriptor;
        QLabel *descriptorLabel;
        QPushButton *remove;
    };
    QVector<Row> rows_;
    QVBoxLayout *keysLayout_;
    QScrollArea *area_;
    QCheckBox *show_;
    QLabel *error_;
    QPushButton *open_;
    QString description_;
    void addKey();
    void updateState();
    QString serialize(QString *error) const;

  public:
    explicit FscryptKeyDialog(QWidget *parent = nullptr);
    QString keyDescription() const { return description_; }
    void accept() override;
};
