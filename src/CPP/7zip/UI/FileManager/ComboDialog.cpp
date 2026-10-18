// Adapted from 7-Zip CPP/7zip/UI/FileManager/ComboDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt layouts replace Win32 OnSize/control attachment; original portable state
// converts only at widget boundaries. OnOK keeps the raw value.
#include "ComboDialog.h"
#include "ViewSettings.h"
#include "qtdialoglayout.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

CComboDialog::CComboDialog(const QString &title, const QString &prompt, const QString &initial,
                           QWidget *parent, const QStringList &strings)
    : QDialog(parent), Title(ToUString(title)), Static(ToUString(prompt)),
      Value(ToUString(initial)) {
    for (const QString &s : strings)
        Strings.Add(ToUString(s));
    OnInit();
}

bool CComboDialog::OnInit() {
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setObjectName("comboDialog");
    setWindowTitle(ToQString(Title));
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("comboButtons");
    auto *layout = QtCreateDialogLayout(*this, *buttons);
    auto *staticControl = new QtWrappingLabel(ToQString(Static), this);
    staticControl->setObjectName("comboPrompt");
    staticControl->setTextFormat(Qt::PlainText);
    staticControl->setWordWrap(true);
    layout->addWidget(staticControl);
    _comboBox = new QComboBox(this);
    _comboBox->setObjectName("comboValue");
    _comboBox->setAccessibleName(ToQString(Static));
    _comboBox->setEditable(true);
    _comboBox->setInsertPolicy(QComboBox::NoInsert);
    _comboBox->setMinimumContentsLength(12);
    _comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    staticControl->setBuddy(_comboBox);
    // Native AddString leaves the edit text unchanged. Qt's first insertion
    // selects item 0, so restore the initial Value after populating Strings.
    _comboBox->setEditText(ToQString(Value));
    FOR_VECTOR(i, Strings)
    _comboBox->addItem(ToQString(Strings[i]));
    _comboBox->setCurrentIndex(-1);
    _comboBox->setEditText(ToQString(Value));
    layout->addWidget(_comboBox);
    layout->addStretch(1);
    connect(buttons, &QDialogButtonBox::accepted, this, &CComboDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    _comboBox->setFocus();
    _comboBox->lineEdit()->selectAll();
    resize(QSize(QtDialogUnits(font()).pixels(QSize(256, 0)).width(), minimumSizeHint().height())
               .expandedTo(minimumSizeHint()));
    QtNormalizeDialogSize(*this);
    return true;
}

void CComboDialog::OnOK() {
    Value = ToUString(_comboBox->currentText());
    QDialog::accept();
}

void CComboDialog::accept() { OnOK(); }

void CComboDialog::BindLanguage(UInt32 titleID, UInt32 promptID) {
    // A caller-built item title has no resource ID. Lang ID 0 names 7-Zip,
    // so binding it would replace the source "item : Comment" title.
    if (titleID)
        QtBindLangText(this, titleID, ToQString(Title));
    QtBindLangText(findChild<QLabel *>("comboPrompt"), promptID, ToQString(Static));
    QtBindLangText(_comboBox, promptID, ToQString(Static), true);
}

QString CComboDialog::value() const {
    return result() == QDialog::Accepted ? ToQString(Value) : _comboBox->currentText();
}
