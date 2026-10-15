#include "createarchivedialog.h"

#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "archivecodeccatalog.h"
#include "qtdialogscreenservices.h"
#include "qtguimessages.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QEvent>
#include <QProxyStyle>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QStyle>

namespace {
// Compact Add is a form: its scroll bars need a separate gutter, including
// when the native style normally flashes an auto-hiding overlay at startup.
class ArchiveFormScrollStyle final : public QProxyStyle {
  public:
    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                  QStyleHintReturn *data) const override {
        if (hint == SH_ScrollBar_Transient)
            return false;
        return QProxyStyle::styleHint(hint, option, widget, data);
    }
    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                    const QWidget *widget) const override {
        if (metric == PM_ScrollView_ScrollBarOverlap)
            return 0;
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};

class ArchiveFormScrollArea final : public QScrollArea {
  public:
    explicit ArchiveFormScrollArea(QWidget *parent) : QScrollArea(parent) {}

  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        const bool result = QScrollArea::eventFilter(object, event);
        if (object == widget() && event->type() == QEvent::LayoutRequest) {
            // Native metrics and translated labels can settle after show.
            // Keep the gutter outside the form even when its minimum grows.
            setMinimumWidth(widget()->minimumSizeHint().width() +
                            verticalScrollBar()->sizeHint().width());
            if (window()->layout())
                window()->layout()->activate();
        }
        return result;
    }
};
} // namespace

CreateArchiveDialog::CreateArchiveDialog(const QString &directory,
                                         const QStringList &initialSources, QWidget *parent,
                                         std::shared_ptr<const ArchiveCodecCatalog> codecs,
                                         const CCompressDialog *sourceDialog)
    : QDialog(parent), currentDirectory_(directory), StartDirPrefix(directory),
      codecs_(std::move(codecs)) {
    setObjectName("createArchiveDialog");
    auto *layout = new QVBoxLayout(this);
    // GuiCommon.rc uses an eight-dialog-unit outer margin. Native Cocoa's
    // larger default would add horizontal scrolling at the compact size.
    const QSize units = QtDialogScreenServices(this).GetDialogBaseUnits();
    const int horizontalMargin = 8 * units.width() / 4;
    const int verticalMargin = 8 * units.height() / 8;
    layout->setContentsMargins(horizontalMargin, verticalMargin, horizontalMargin, verticalMargin);
    auto *scroll = new ArchiveFormScrollArea(this);
    scroll->setObjectName("archiveOptionsScrollArea");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *scrollStyle = new ArchiveFormScrollStyle;
    scrollStyle->setParent(scroll);
    scroll->verticalScrollBar()->setStyle(scrollStyle);
    scroll->horizontalScrollBar()->setStyle(scrollStyle);
    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    m_RegistryInfo.Load(compressionSettingsPath());
    fixedSources_ = sourceDialog != nullptr;
    if (sourceDialog)
        archiveNameState_ = *sourceDialog;
    else
        archiveNameState_.DirPrefix = currentDirectory_;
    prepareInitialSources(initialSources);
    createArchiveForm(contentLayout);
    scroll->setWidget(content);
    layout->addWidget(scroll);
    createFooter(layout);
    // Cocoa layout-item margins can extend a combo or group frame past its
    // allocated column/scroll area. Keep the complete source controls exposed.
    for (auto *combo : findChildren<QComboBox *>())
        combo->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    for (auto *group : findChildren<QGroupBox *>())
        group->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    // Source MY_CHECKBOX/PushButton controls participate in Tab traversal.
    // QMacStyle otherwise gives buttons only the restricted TabFocus policy.
    for (auto *button : findChildren<QAbstractButton *>())
        button->setFocusPolicy(Qt::StrongFocus);
    BindLanguage();
    if (fixedSources_)
        initializeSourceControls(sourceDialog->Info);
    // Size the source form to its controls rather than reserving the height
    // of the former file picker. Leave room for the native window frame and
    // taskbar; the options can scroll on a smaller screen, the footer cannot.
    const QMargins margins = layout->contentsMargins();
    const int width =
        qMax(640, content->minimumSizeHint().width() + margins.left() + margins.right());
    const int height = content->sizeHint().height() + buttons_->sizeHint().height() +
                       layout->spacing() + margins.top() + margins.bottom();
    const int frameHeight = 2 * style()->pixelMetric(QStyle::PM_TitleBarHeight);
    const QSize available = screen()->availableGeometry().size() - QSize(0, frameHeight);
    resize(QSize(width, height).boundedTo(available).expandedTo(minimumSizeHint()));
    // The original modal's first editable combo receives initial dialog focus
    // and selects its text. Qt's scroll hierarchy needs an explicit adapter.
    name_->setFocus(Qt::OtherFocusReason);
    name_->selectAll();
}

void CreateArchiveDialog::createArchiveForm(QVBoxLayout *layout) {
    auto *form = new QFormLayout;
    auto *archiveField = new QWidget(this);
    archiveField->setObjectName("archiveNameField");
    auto *fieldLayout = new QVBoxLayout(archiveField);
    fieldLayout->setContentsMargins(0, 0, 0, 0);
    archiveFolder_ =
        new QLabel(QDir::toNativeSeparators(archiveNameState_.DirPrefix), archiveField);
    archiveFolder_->setObjectName("archiveFolder");
    archiveFolder_->setTextFormat(Qt::PlainText);
    archiveFolder_->setWordWrap(true);
    // A Windows prefix can be one unbreakable word. It must not impose its
    // full text width on the scroll content and clip the actual inputs.
    archiveFolder_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    fieldLayout->addWidget(archiveFolder_);
    auto *archiveRow = new QWidget(this);
    archiveRow->setObjectName("archivePathRow");
    auto *archiveLayout = new QHBoxLayout(archiveRow);
    archiveLayout->setContentsMargins(0, 0, 0, 0);
    archivePath_ = new QComboBox(this);
    archivePath_->setObjectName("archivePath");
    archivePath_->setEditable(true);
    archivePath_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    archivePath_->setMinimumContentsLength(20);
    archivePath_->setInsertPolicy(QComboBox::NoInsert);
    for (unsigned i = 0; i < m_RegistryInfo.ArcPaths.Size() && i < 20; i++)
        archivePath_->addItem(m_RegistryInfo.ArcPaths[i]);
    name_ = archivePath_->lineEdit();
    name_->setObjectName("archiveName");
    archiveLayout->addWidget(archivePath_, 1);
    auto *browse = new QPushButton("...", archiveRow);
    browse->setObjectName("archiveBrowse");
    // GuiCommon.rc's bxsDots is a narrow browse button, not a footer button.
    browse->setFixedWidth(
        qMax(browse->sizeHint().height(), browse->fontMetrics().averageCharWidth() * 5));
    archiveLayout->addWidget(browse);
    archiveRow->setFocusProxy(archivePath_);
    fieldLayout->addWidget(archiveRow);
    auto *archiveLabel = new QLabel("Архив:", this);
    archiveLabel->setObjectName("archiveLabel");
    archiveLabel->setBuddy(archivePath_);
    archiveLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // CompressDialog.rc: folder and combo share xArcFolderOffs; the caption
    // remains in the left column of this two-line field.
    auto *archiveHeader = new QHBoxLayout;
    archiveHeader->addWidget(archiveLabel, 0, Qt::AlignVCenter);
    archiveHeader->addWidget(archiveField, 1);
    form->addRow(archiveHeader);
    format_ = new QComboBox(this);
    format_->setObjectName("archiveFormat");
    populateArchiveFormats();
    if (!codecs_) {
        const int savedFormat = format_->findText(m_RegistryInfo.ArcType, Qt::MatchFixedString);
        format_->setCurrentIndex(savedFormat >= 0 ? savedFormat : 0);
    }
    if (codecs_ && codecs_->result() != S_OK)
        format_->setToolTip(codecs_->error());
    configureArchiveName(format_->currentText());
    archiveNameState_.SetArchiveName(suggestedArchiveName_);
    name_->setText(archiveNameState_.m_ArchivePath);
    {
        updateMode_ = new QComboBox(this);
        updateMode_->setObjectName("archiveUpdateMode");
        updateMode_->addItem("Добавить и заменить", int(NCompressDialog::NUpdateMode::kAdd));
        updateMode_->addItem("Обновить и добавить", int(NCompressDialog::NUpdateMode::kUpdate));
        updateMode_->addItem("Обновить существующие", int(NCompressDialog::NUpdateMode::kFresh));
        updateMode_->addItem("Синхронизировать", int(NCompressDialog::NUpdateMode::kSync));
    }
    pathMode_ = new QComboBox(this);
    pathMode_->setObjectName("archivePathMode");
    pathMode_->addItem("Относительные пути", int(NWildcard::k_RelatPath));
    pathMode_->addItem("Полные пути", int(NWildcard::k_FullPath));
    pathMode_->addItem("Абсолютные пути", int(NWildcard::k_AbsPath));
    {
        // CompressDialog.rc places compression on the left and encryption on
        // the right. Keep both columns at compact sizes; scrolling keeps
        // the source form usable when the available screen is smaller.
        auto *columns = new QHBoxLayout;
        auto *compressionColumn = new QWidget(this);
        compressionColumn->setObjectName("archiveCompressionColumn");
        auto *compressionForm = new QFormLayout(compressionColumn);
        compressionForm->setContentsMargins(0, 0, 0, 0);
        // CompressDialog.rc gives the six combo rows a shared g1x/g1xs.
        // Cocoa's default keeps each field at its individual size hint.
        compressionForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        compressionForm->setRowWrapPolicy(QFormLayout::WrapLongRows);
        auto *encryptionColumn = new QWidget(this);
        encryptionColumn->setObjectName("archiveEncryptionColumn");
        auto *encryptionForm = new QFormLayout(encryptionColumn);
        encryptionForm->setContentsMargins(0, 0, 0, 0);
        encryptionForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        encryptionForm->setRowWrapPolicy(QFormLayout::WrapLongRows);
        compressionForm->addRow("Формат:", format_);
        createCompressionOptions(compressionForm);
        createVolumeOptions(compressionForm);
        createAdvancedOptions(compressionForm);
        encryptionForm->addRow("Режим обновления:", updateMode_);
        encryptionForm->addRow("Режим путей:", pathMode_);
        createLifecycleOptions(encryptionForm);
        createEncryptionOptions(encryptionForm);
        columns->addWidget(compressionColumn, 1, Qt::AlignTop);
        columns->addWidget(encryptionColumn, 1, Qt::AlignTop);
        form->addRow(columns);
    }
    layout->addLayout(form);
    connect(browse, &QPushButton::clicked, this, [this] {
        CCompressDialog path = archiveNameState_;
        path.Format = format_->currentText();
        path.m_ArchivePath = name_->text();
        path.Info.SFXMode = sfx_ && sfx_->isChecked();
        if (!path.OnButtonSetArchive(this))
            return;
        format_->setCurrentText(path.Format);
        archiveNameState_.DirPrefix = path.DirPrefix;
        archiveFolder_->setText(QDir::toNativeSeparators(path.DirPrefix));
        name_->setText(path.m_ArchivePath);
    });
    connect(format_, &QComboBox::currentTextChanged, this, [this] {
        archiveNameState_.m_ArchivePath = name_->text();
        configureArchiveName(format_->currentText());
        archiveNameState_.SetArchiveName2();
        name_->setText(archiveNameState_.m_ArchivePath);
    });
    connect(archivePath_, &QComboBox::activated, this, [this](int index) {
        CCompressDialog path = archiveNameState_;
        const QString selectedPath = archivePath_->itemText(index);
        path.Format = format_->currentText();
        path.ArcPath_WasChanged(selectedPath);
        format_->setCurrentText(path.Format);
        path.SetArcPathFields(selectedPath);
        archiveNameState_.DirPrefix = path.DirPrefix;
        archiveFolder_->setText(QDir::toNativeSeparators(path.DirPrefix));
        name_->setText(path.m_ArchivePath);
    });
}

void CreateArchiveDialog::createFooter(QVBoxLayout *layout) {
    buttons_ = new QtLangButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    const auto showHelp = [this] {
        CCompressDialog dialog;
        dialog.OnHelp(this);
    };
    connect(buttons_, &QDialogButtonBox::helpRequested, this, showHelp);
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, showHelp);
    layout->addWidget(buttons_);
    if (format_->currentIndex() < 0)
        buttons_->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons_, &QDialogButtonBox::accepted, this, &CreateArchiveDialog::validateAndAccept);
}

QString CreateArchiveDialog::archivePath() const {
    CCompressDialog path;
    path.DirPrefix = archiveNameState_.DirPrefix;
    path.StartDirPrefix = StartDirPrefix;
    path.m_ArchivePath = name_->text();
    QString result;
    path.GetFinalPath_Smart(result);
    return result;
}

void CreateArchiveDialog::validateAndAccept() {
    if (password_) {
        const auto error = CCompressDialog::CheckPassword(
            password_->text(), passwordConfirmation_->text(), showPassword_->isChecked(),
            format_->currentText(), GetEncryptionMethodSpec());
        if (error != CCompressDialog::kPasswordOK) {
            const UInt32 id = error == CCompressDialog::kPasswordUseAscii  ? IDS_PASSWORD_USE_ASCII
                              : error == CCompressDialog::kPasswordTooLong ? IDS_PASSWORD_TOO_LONG
                                                                           : IDS_PASSWORD_NOT_MATCH;
            ShowErrorMessageHwndResQt(this, id);
            return;
        }
    }
    if (!checkMemoryUsage())
        return;
    SaveOptionsInMem();
    const QString resolved = archivePath();
    if (resolved.isEmpty()) {
        // Original OnOK validates GetFinalPath_Smart only. Source scanning and
        // UpdateArchive own input errors and CreateComplexDir for new parents.
        ShowErrorMessageQt(this, L"Incorrect archive path");
        return;
    }
    if (!checkVolumeSizes())
        return;
    // Existing outputs are opened/validated by source UpdateArchive after OK,
    // just as for the original form. The dialog must not reject update intent.
    target_ = resolved;
    CCompressDialog path;
    path.m_ArchivePath = target_;
    path.Format = format_->currentText();
    path.ShowPassword = showPassword_ && showPassword_->isChecked();
    path.EncryptHeaders = encryptHeaders_ && encryptHeaders_->isChecked();
    path.OnOK(m_RegistryInfo);
    m_RegistryInfo.Save(compressionSettingsPath());
    accept();
}

CompressionOptions CreateArchiveDialog::compressionOptions() const {
    CompressionOptions options;
    options.SFXMode = sfx_ && sfx_->isEnabled() && sfx_->isChecked();
    if (options.SFXMode)
        options.SfxModule = sfxModule_.path();
    options.format = format_->currentText();
    CCompressDialog metadata;
    if (fixedSources_)
        metadata.Info = archiveNameState_.Info;
    if (codecs_ && codecs_->result() == S_OK) {
        const int index = codecs_->formatIndex(registryFormat_);
        if (index >= 0)
            metadata.ArcInfo = codecs_->nativeCodecs().Formats[unsigned(index)];
    }
    metadata.LoadMetadataOptions(m_RegistryInfo);
    NCompression::CInfo metadataRegistry = m_RegistryInfo;
    metadata.SaveMetadataOptions(metadataRegistry);
    options.SymLinks = metadata.Info.SymLinks;
    options.HardLinks = metadata.Info.HardLinks;
    options.AltStreams = metadata.Info.AltStreams;
    options.NtSecurity = metadata.Info.NtSecurity;
    options.OpenShareForWrite =
        sharedFiles_ && sharedFiles_->isEnabled() && sharedFiles_->isChecked();
    options.DeleteAfterCompressing = deleteAfterCompressing_ &&
                                     deleteAfterCompressing_->isEnabled() &&
                                     deleteAfterCompressing_->isChecked();
#ifdef Q_OS_WIN
    options.PreserveATime = m_RegistryInfo.PreserveATime;
#endif
    if (parameters_) {
        options.Options = parameters_->text().toStdWString().c_str();
        options.Options.Trim();
    }
    const int index = FindRegistryFormat(registryFormat_);
    if (index >= 0) {
        const auto &fo = m_RegistryInfo.Formats[index];
        options.TimePrec = fo.TimePrec;
        options.MTime = fo.MTime;
        options.CTime = fo.CTime;
        options.ATime = fo.ATime;
        options.SetArcMTime = fo.SetArcMTime;
    }
    if (volume_ && volume_->isEnabled())
        CCompressDialog::OnOK(volume_->currentText(), options);
    if (updateMode_)
        options.UpdateMode =
            static_cast<NCompressDialog::NUpdateMode::EEnum>(updateMode_->currentData().toInt());
    options.PathMode = static_cast<NWildcard::ECensorPathMode>(pathMode_->currentData().toInt());
    if (level_) {
        if (encryption_)
            options.EncryptionMethod = GetEncryptionMethodSpec();
        options.Level = level_->currentData().toInt();
        options.Method = archiveNameState_.GetMethodSpec();
        options.OrderMode = NCompressDialog::GetOrderMode(archiveNameState_.GetMethodID());
        const UInt32 threads = threads_->currentData().toUInt();
        options.NumThreads = threads ? threads : UInt32(-1);
        options.MemUsage = Get_MemUse_Spec();
        const UInt64 dictionary =
            dictionary_->isEnabled() ? dictionary_->currentData().toULongLong() : 0;
        options.Dict64 = dictionary ? dictionary : UInt64(-1);
        const UInt32 order = wordSize_->isEnabled() ? wordSize_->currentData().toUInt() : 0;
        options.Order = order ? order : UInt32(-1);
        options.SolidIsSpecified = solidBlock_->isEnabled();
        NCompressDialog::OnOK(
            solidBlock_->isEnabled() ? solidBlock_->currentData().toUInt() : UInt32(-1), options);
    }
    return options;
}
