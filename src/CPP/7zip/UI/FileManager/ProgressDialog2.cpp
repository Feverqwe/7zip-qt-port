// Adapted from 7-Zip CPP/7zip/UI/FileManager/ProgressDialog2.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "ProgressDialog2.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "LangUtils.h"
#include "ViewSettings.h"
#include "qtlistview.h"
#include <QKeyEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimerEvent>
#ifndef _WIN32
#include <QThread>
#endif

using namespace NWindows;

static const DWORD kPauseSleepTime = 100;

#define UNDEFINED_VAL ((UInt64)(Int64) - 1)
#define INIT_AS_UNDEFINED(v) v = UNDEFINED_VAL;
#define IS_UNDEFINED_VAL(v) ((v) == UNDEFINED_VAL)
#define IS_DEFINED_VAL(v) ((v) != UNDEFINED_VAL)

CProgressSync::CProgressSync()
    : _stopped(false), _paused(false), _filesProgressMode(false), _isDir(false),
      _totalBytes(UNDEFINED_VAL), _completedBytes(0), _totalFiles(UNDEFINED_VAL), _curFiles(0),
      _inSize(UNDEFINED_VAL), _outSize(UNDEFINED_VAL) {}

#define CHECK_STOP                                                                                 \
    if (_stopped)                                                                                  \
        return E_ABORT;                                                                            \
    if (!_paused)                                                                                  \
        return S_OK;
#define CRITICAL_LOCK NSynchronization::CCriticalSectionLock lock(_cs);

bool CProgressSync::Get_Paused() {
    CRITICAL_LOCK
    return _paused;
}

HRESULT CProgressSync::CheckStop() {
    for (;;) {
        {
            CRITICAL_LOCK
            CHECK_STOP
        }
#ifdef _WIN32
        ::Sleep(kPauseSleepTime);
#else
        QThread::msleep(kPauseSleepTime);
#endif
    }
}

void CProgressSync::Clear_Stop_Status() {
    CRITICAL_LOCK
    if (_stopped)
        _stopped = false;
}

HRESULT CProgressSync::ScanProgress(UInt64 numFiles, UInt64 totalSize, const FString &fileName,
                                    bool isDir) {
    {
        CRITICAL_LOCK
        _totalFiles = numFiles;
        _totalBytes = totalSize;
        _filePath = fs2us(fileName);
        _isDir = isDir;
        // _completedBytes = 0;
        CHECK_STOP
    }
    return CheckStop();
}

HRESULT CProgressSync::Set_NumFilesTotal(UInt64 val) {
    {
        CRITICAL_LOCK
        _totalFiles = val;
        CHECK_STOP
    }
    return CheckStop();
}

void CProgressSync::Set_NumBytesTotal(UInt64 val) {
    CRITICAL_LOCK
    _totalBytes = val;
}

void CProgressSync::Set_NumFilesCur(UInt64 val) {
    CRITICAL_LOCK
    _curFiles = val;
}

HRESULT CProgressSync::Set_NumBytesCur(const UInt64 *val) {
    {
        CRITICAL_LOCK
        if (val)
            _completedBytes = *val;
        CHECK_STOP
    }
    return CheckStop();
}

HRESULT CProgressSync::Set_NumBytesCur(UInt64 val) {
    {
        CRITICAL_LOCK
        _completedBytes = val;
        CHECK_STOP
    }
    return CheckStop();
}

void CProgressSync::Set_Ratio(const UInt64 *inSize, const UInt64 *outSize) {
    CRITICAL_LOCK
    if (inSize)
        _inSize = *inSize;
    if (outSize)
        _outSize = *outSize;
}

void CProgressSync::Set_TitleFileName(const UString &fileName) {
    CRITICAL_LOCK
    _titleFileName = fileName;
}

void CProgressSync::Set_Status(const UString &s) {
    CRITICAL_LOCK
    _status = s;
}

HRESULT CProgressSync::Set_Status2(const UString &s, const wchar_t *path, bool isDir) {
    {
        CRITICAL_LOCK
        _status = s;
        if (path)
            _filePath = path;
        else
            _filePath.Empty();
        _isDir = isDir;
    }
    return CheckStop();
}

void CProgressSync::Set_FilePath(const wchar_t *path, bool isDir) {
    CRITICAL_LOCK
    if (path)
        _filePath = path;
    else
        _filePath.Empty();
    _isDir = isDir;
}

void CProgressSync::AddError_Message(const wchar_t *message) {
    CRITICAL_LOCK
    Messages.Add(message);
}

void CProgressSync::AddError_Message_Name(const wchar_t *message, const wchar_t *name) {
    UString s;
    if (name && *name != 0)
        s += name;
    if (message && *message != 0) {
        if (!s.IsEmpty())
            s.Add_LF();
        s += message;
        if (!s.IsEmpty() && s.Back() == L'\n')
            s.DeleteBack();
    }
    AddError_Message(s);
}

void CProgressSync::AddError_Code_Name(HRESULT systemError, const wchar_t *name) {
    UString s = NError::MyFormatMessage(systemError);
    if (systemError == 0)
        s = "Error";
    AddError_Message_Name(s, name);
}

#define UINT_TO_STR_2(val)                                                                         \
    {                                                                                              \
        s[0] = (wchar_t)('0' + (val) / 10);                                                        \
        s[1] = (wchar_t)('0' + (val) % 10);                                                        \
        s += 2;                                                                                    \
    }

void GetTimeString(UInt64 timeValue, wchar_t *s) {
    UInt64 hours = timeValue / 3600;
    UInt32 seconds = (UInt32)(timeValue - hours * 3600);
    UInt32 minutes = seconds / 60;
    seconds %= 60;
    if (hours > 99) {
        ConvertUInt64ToString(hours, s);
        for (; *s != 0; s++)
            ;
    } else {
        UInt32 hours32 = (UInt32)hours;
        UINT_TO_STR_2(hours32)
    }
    *s++ = ':';
    UINT_TO_STR_2(minutes)
    *s++ = ':';
    UINT_TO_STR_2(seconds)
    *s = 0;
}

static void ReduceString(UString &s, unsigned size) {
    if (s.Len() <= size)
        return;
    s.Delete(size / 2, s.Len() - size);
    s.Insert(size / 2, L" ... ");
}

static void ConvertSizeToString(UInt64 v, wchar_t *s) {
    Byte c = 0;
    if (v >= ((UInt64)100000 << 20)) {
        v >>= 30;
        c = 'G';
    } else if (v >= ((UInt64)100000 << 10)) {
        v >>= 20;
        c = 'M';
    } else if (v >= ((UInt64)100000 << 0)) {
        v >>= 10;
        c = 'K';
    }
    ConvertUInt64ToString(v, s);
    if (c != 0) {
        s += MyStringLen(s);
        *s++ = ' ';
        *s++ = c;
        *s++ = 'B';
        *s++ = 0;
    }
}

static void GetChangedString(const UString &newStr, UString &prevStr, bool &hasChanged) {
    hasChanged = !(prevStr == newStr);
    if (hasChanged)
        prevStr = newStr;
}

static unsigned GetPower32(UInt32 val) {
    const unsigned kStart = 32;
    UInt32 mask = ((UInt32)1 << (kStart - 1));
    for (unsigned i = kStart;; i--) {
        if (i == 0 || (val & mask) != 0)
            return i;
        mask >>= 1;
    }
}

static unsigned GetPower64(UInt64 val) {
    UInt32 high = (UInt32)(val >> 32);
    if (high == 0)
        return GetPower32((UInt32)val);
    return GetPower32(high) + 32;
}

static UInt64 MyMultAndDiv(UInt64 mult1, UInt64 mult2, UInt64 divider) {
    unsigned pow1 = GetPower64(mult1);
    unsigned pow2 = GetPower64(mult2);
    while (pow1 + pow2 > 64) {
        if (pow1 > pow2) {
            pow1--;
            mult1 >>= 1;
        } else {
            pow2--;
            mult2 >>= 1;
        }
        divider >>= 1;
    }
    UInt64 res = mult1 * mult2;
    if (divider != 0)
        res /= divider;
    return res;
}

static unsigned GetNumDigits(unsigned val) {
    unsigned i = 0;
    for (;;) {
        i++;
        val /= 10;
        if (val == 0)
            return i;
    }
}

static const unsigned kTitleFileNameSizeLimit = 36;
static const unsigned kCurrentFileNameSizeLimit = 82;

CProgressDialog::CProgressDialog(QWidget *parent, const UString &title)
    : CQtProgressDialog(parent), _isDir(false), _wasCreated(false), _needClose(false),
      _errorsWereDisplayed(false), _waitCloseByCancelButton(false), _cancelWasPressed(false),
      _inCancelMessageBox(false), _externalCloseMessageWasReceived(false), _background(false),
      WaitMode(false), MessagesDisplayed(false), CompressingMode(true), ShowCompressionInfo(true),
      _numPostedMessages(0), _numAutoSizeMessages(0), _numMessages(0), _timer(0), _title(title),
      IconID(-1), MainWindow(nullptr) {
    if (_dialogCreatedEvent.Create() != S_OK)
        throw 1334987;
    if (_createDialogEvent.Create() != S_OK)
        throw 1334987;
    connect(static_cast<QPushButton *>(GetItem(IDB_PAUSE)), &QPushButton::clicked, this,
            &CProgressDialog::OnPauseButton);
    GetItem(IDL_PROGRESS_MESSAGES)->installEventFilter(this);
}

CProgressDialog::~CProgressDialog() { AddToTitle(L""); }

void CProgressDialog::AddToTitle(const wchar_t *s) {
    if (MainWindow)
        MainWindow->setWindowTitle(ToQString((UString)s + MainTitle));
}

void CProgressDialog::EnableErrorsControls(bool enable) {
    ShowItem_Bool(IDT_PROGRESS_ERRORS, enable);
    ShowItem_Bool(IDT_PROGRESS_ERRORS_VAL, enable);
    ShowItem_Bool(IDL_PROGRESS_MESSAGES, enable);
}

bool CProgressDialog::OnInit() {
    INIT_AS_UNDEFINED(_progressBar_Range)
    INIT_AS_UNDEFINED(_progressBar_Pos)
    INIT_AS_UNDEFINED(_prevPercentValue)
    INIT_AS_UNDEFINED(_prevElapsedSec)
    INIT_AS_UNDEFINED(_prevRemainingSec)
    INIT_AS_UNDEFINED(_prevSpeed)
    _prevSpeed_MoveBits = 0;
    _prevTime = GetTickCount();
    _elapsedTime = 0;
    INIT_AS_UNDEFINED(_totalBytes_Prev)
    INIT_AS_UNDEFINED(_processed_Prev)
    INIT_AS_UNDEFINED(_packed_Prev)
    INIT_AS_UNDEFINED(_ratio_Prev)
    _filesStr_Prev.Empty();
    _filesTotStr_Prev.Empty();
    m_ProgressBar.Attach(GetItem(IDC_PROGRESS1));
    _messageList.Attach(GetItem(IDL_PROGRESS_MESSAGES));
    _wasCreated = true;
    _dialogCreatedEvent.Set();
    GetItemText(IDB_PROGRESS_BACKGROUND, _background_String);
    _backgrounded_String = _background_String;
    _backgrounded_String.RemoveChar(L'&');
    GetItemText(IDB_PAUSE, _pause_String);
    LangString(IDS_PROGRESS_FOREGROUND, _foreground_String);
    LangString(IDS_CONTINUE, _continue_String);
    LangString(IDS_PROGRESS_PAUSED, _paused_String);
    SetText(_title);
    SetPauseText();
    SetPriorityText();
    _messageList.InsertColumn(0, L"", 40);
    _messageList.InsertColumn(1, L"", 460);
    _messageList.SetColumnWidthAuto(0);
    _messageList.SetColumnWidthAuto(1);
    EnableErrorsControls(false);
    GetItemSizes(2, _buttonSizeX, _buttonSizeY);
    _numReduceSymbols = kCurrentFileNameSizeLimit;
    NormalizeSize(true);
    if (!ShowCompressionInfo) {
        HideItem(IDT_PROGRESS_PACKED);
        HideItem(IDT_PROGRESS_PACKED_VAL);
        HideItem(IDT_PROGRESS_RATIO);
        HideItem(IDT_PROGRESS_RATIO_VAL);
    }
    _timer = startTimer(200);
    // Taskbar/icon and CheckNeedClose follow in mandatory b/b3.
    return true;
}

static const UINT kIDs[] = {IDT_PROGRESS_ELAPSED,
                            IDT_PROGRESS_ELAPSED_VAL,
                            IDT_PROGRESS_REMAINING,
                            IDT_PROGRESS_REMAINING_VAL,
                            IDT_PROGRESS_FILES,
                            IDT_PROGRESS_FILES_VAL,
                            0,
                            IDT_PROGRESS_FILES_TOTAL,
                            IDT_PROGRESS_ERRORS,
                            IDT_PROGRESS_ERRORS_VAL,
                            IDT_PROGRESS_TOTAL,
                            IDT_PROGRESS_TOTAL_VAL,
                            IDT_PROGRESS_SPEED,
                            IDT_PROGRESS_SPEED_VAL,
                            IDT_PROGRESS_PROCESSED,
                            IDT_PROGRESS_PROCESSED_VAL,
                            IDT_PROGRESS_PACKED,
                            IDT_PROGRESS_PACKED_VAL,
                            IDT_PROGRESS_RATIO,
                            IDT_PROGRESS_RATIO_VAL};

bool CProgressDialog::OnSize(int xSize, int ySize) {
    int sY;
    int sStep;
    int mx, my;
    {
        CQtProgressRect r;
        GetClientRectOfItem(IDT_PROGRESS_ELAPSED, r);
        mx = r.left;
        my = r.top;
        sY = r.bottom - r.top;
        GetClientRectOfItem(IDT_PROGRESS_REMAINING, r);
        sStep = r.top - my;
    }
    update();
    const int xSizeClient = xSize - mx * 2;
    {
        unsigned i;
        for (i = 800; i > 40; i = i * 9 / 10)
            if (Units_To_Pixels_X((int)i) <= xSizeClient)
                break;
        _numReduceSymbols = i / 4;
    }
    int yPos = ySize - my - _buttonSizeY;
    ChangeSubWindowSizeX(GetItem(IDT_PROGRESS_STATUS), xSize - mx * 2);
    ChangeSubWindowSizeX(GetItem(IDT_PROGRESS_FILE_NAME), xSize - mx * 2);
    ChangeSubWindowSizeX(GetItem(IDC_PROGRESS1), xSize - mx * 2);
    int bSizeX = _buttonSizeX;
    int mx2 = mx;
    for (;; mx2--) {
        const int bSize2 = bSizeX * 3 + mx2 * 2;
        if (bSize2 <= xSizeClient)
            break;
        if (mx2 < 5) {
            bSizeX = (xSizeClient - mx2 * 2) / 3;
            break;
        }
    }
    if (bSizeX < 2)
        bSizeX = 2;
    {
        CQtProgressRect r;
        GetClientRectOfItem(IDL_PROGRESS_MESSAGES, r);
        const int y = r.top;
        int ySize2 = yPos - my - y;
        const int kMinYSize = _buttonSizeY + _buttonSizeY * 3 / 4;
        int xx = xSize - mx * 2;
        if (ySize2 < kMinYSize) {
            ySize2 = kMinYSize;
            if (xx > bSizeX * 2)
                xx -= bSizeX;
        }
        _messageList.Move(mx, y, xx, ySize2);
    }
    {
        int xPos = xSize - mx;
        xPos -= bSizeX;
        MoveItem(2, xPos, yPos, bSizeX, _buttonSizeY);
        xPos -= (mx2 + bSizeX);
        MoveItem(IDB_PAUSE, xPos, yPos, bSizeX, _buttonSizeY);
        xPos -= (mx2 + bSizeX);
        MoveItem(IDB_PROGRESS_BACKGROUND, xPos, yPos, bSizeX, _buttonSizeY);
    }
    int valueSize;
    int labelSize;
    int padSize;
    labelSize = Units_To_Pixels_X(MY_PROGRESS_LABEL_UNITS_MIN);
    valueSize = Units_To_Pixels_X(MY_PROGRESS_VAL_UNITS);
    padSize = Units_To_Pixels_X(MY_PROGRESS_PAD_UNITS);
    const int requiredSize = (labelSize + valueSize) * 2 + padSize;
    int gSize;
    {
        if (requiredSize < xSizeClient) {
            const int incr = (xSizeClient - requiredSize) / 3;
            labelSize += incr;
        } else
            labelSize = (xSizeClient - valueSize * 2 - padSize) / 2;
        if (labelSize < 0)
            labelSize = 0;
        gSize = labelSize + valueSize;
        padSize = xSizeClient - gSize * 2;
    }
    labelSize = gSize - valueSize;
    yPos = my;
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(kIDs); i += 2) {
        int x = mx;
        const unsigned kNumColumn1Items = 5 * 2;
        if (i >= kNumColumn1Items) {
            if (i == kNumColumn1Items)
                yPos = my;
            x = mx + gSize + padSize;
        }
        if (kIDs[i] != 0)
            MoveItem(kIDs[i], x, yPos, labelSize, sY);
        MoveItem(kIDs[i + 1], x + labelSize, yPos, valueSize, sY);
        yPos += sStep;
    }
    return false;
}

void CProgressDialog::SetProgressRange(UInt64 range) {
    if (range == _progressBar_Range)
        return;
    _progressBar_Range = range;
    INIT_AS_UNDEFINED(_progressBar_Pos)
    _progressConv.Init(range);
    m_ProgressBar.SetRange32(0, _progressConv.Count(range));
}

void CProgressDialog::SetProgressPos(UInt64 pos) {
    if (pos >= _progressBar_Range || pos <= _progressBar_Pos ||
        pos - _progressBar_Pos >= (_progressBar_Range >> 10)) {
        m_ProgressBar.SetPos(_progressConv.Count(pos));
        // Taskbar progress-value service follows in mandatory b.
        _progressBar_Pos = pos;
    }
}

void CProgressDialog::ShowSize(unsigned id, UInt64 val, UInt64 &prev) {
    if (val == prev)
        return;
    prev = val;
    wchar_t s[40];
    s[0] = 0;
    if (IS_DEFINED_VAL(val))
        ConvertSizeToString(val, s);
    SetItemText(id, s);
}

void CProgressDialog::UpdateStatInfo(bool showAll) {
    UInt64 total, completed, totalFiles, completedFiles, inSize, outSize;
    bool filesProgressMode;
    bool titleFileName_Changed;
    bool curFilePath_Changed;
    bool status_Changed;
    unsigned numErrors;
    {
        NSynchronization::CCriticalSectionLock lock(Sync._cs);
        total = Sync._totalBytes;
        completed = Sync._completedBytes;
        totalFiles = Sync._totalFiles;
        completedFiles = Sync._curFiles;
        inSize = Sync._inSize;
        outSize = Sync._outSize;
        filesProgressMode = Sync._filesProgressMode;
        GetChangedString(Sync._titleFileName, _titleFileName, titleFileName_Changed);
        GetChangedString(Sync._filePath, _filePath, curFilePath_Changed);
        GetChangedString(Sync._status, _status, status_Changed);
        if (_isDir != Sync._isDir) {
            curFilePath_Changed = true;
            _isDir = Sync._isDir;
        }
        numErrors = Sync.Messages.Size();
    }
    UInt32 curTime = GetTickCount();
    const UInt64 progressTotal = filesProgressMode ? totalFiles : total;
    const UInt64 progressCompleted = filesProgressMode ? completedFiles : completed;
    {
        if (IS_UNDEFINED_VAL(progressTotal)) {
            // SetPos(0);
            // SetRange(progressCompleted);
        } else {
            if (_progressBar_Pos != 0 || progressCompleted != 0 ||
                (_progressBar_Range == 0 && progressTotal != 0)) {
                SetProgressRange(progressTotal);
                SetProgressPos(progressCompleted);
            }
        }
    }
    ShowSize(IDT_PROGRESS_TOTAL_VAL, total, _totalBytes_Prev);
    _elapsedTime += (curTime - _prevTime);
    _prevTime = curTime;
    UInt64 elapsedSec = _elapsedTime / 1000;
    bool elapsedChanged = false;
    if (elapsedSec != _prevElapsedSec) {
        _prevElapsedSec = elapsedSec;
        elapsedChanged = true;
        wchar_t s[40];
        GetTimeString(elapsedSec, s);
        SetItemText(IDT_PROGRESS_ELAPSED_VAL, s);
    }
    bool needSetTitle = false;
    if (elapsedChanged || showAll) {
        if (numErrors > _numPostedMessages) {
            UpdateMessagesDialog();
            wchar_t s[32];
            ConvertUInt64ToString(numErrors, s);
            SetItemText(IDT_PROGRESS_ERRORS_VAL, s);
            if (!_errorsWereDisplayed) {
                _errorsWereDisplayed = true;
                EnableErrorsControls(true);
                // Taskbar error-state service follows in mandatory b.
            }
        }
        if (progressCompleted != 0) {
            if (IS_UNDEFINED_VAL(progressTotal)) {
                if (IS_DEFINED_VAL(_prevRemainingSec)) {
                    INIT_AS_UNDEFINED(_prevRemainingSec)
                    SetItemText(IDT_PROGRESS_REMAINING_VAL, L"");
                }
            } else {
                UInt64 remainingTime = 0;
                if (progressCompleted < progressTotal)
                    remainingTime = MyMultAndDiv(_elapsedTime, progressTotal - progressCompleted,
                                                 progressCompleted);
                UInt64 remainingSec = remainingTime / 1000;
                if (remainingSec != _prevRemainingSec) {
                    _prevRemainingSec = remainingSec;
                    wchar_t s[40];
                    GetTimeString(remainingSec, s);
                    SetItemText(IDT_PROGRESS_REMAINING_VAL, s);
                }
            }
            {
                const UInt64 elapsedTime = (_elapsedTime == 0) ? 1 : _elapsedTime;
                // 22.02: progressCompleted can be for number of files
                UInt64 v = (completed * 1000) / elapsedTime;
                Byte c = 0;
                unsigned moveBits = 0;
                if (v >= ((UInt64)10000 << 10)) {
                    moveBits = 20;
                    c = 'M';
                } else if (v >= ((UInt64)10000 << 0)) {
                    moveBits = 10;
                    c = 'K';
                }
                v >>= moveBits;
                if (moveBits != _prevSpeed_MoveBits || v != _prevSpeed) {
                    _prevSpeed_MoveBits = moveBits;
                    _prevSpeed = v;
                    wchar_t s[40];
                    ConvertUInt64ToString(v, s);
                    unsigned pos = MyStringLen(s);
                    s[pos++] = ' ';
                    if (moveBits != 0)
                        s[pos++] = c;
                    s[pos++] = 'B';
                    s[pos++] = '/';
                    s[pos++] = 's';
                    s[pos++] = 0;
                    SetItemText(IDT_PROGRESS_SPEED_VAL, s);
                }
            }
        }
        {
            UInt64 percent = 0;
            {
                if (IS_DEFINED_VAL(progressTotal)) {
                    percent = progressCompleted * 100;
                    if (progressTotal != 0)
                        percent /= progressTotal;
                }
            }
            if (percent != _prevPercentValue) {
                _prevPercentValue = percent;
                needSetTitle = true;
            }
        }
        {
            wchar_t s[64];
            ConvertUInt64ToString(completedFiles, s);
            if (_filesStr_Prev != s) {
                _filesStr_Prev = s;
                SetItemText(IDT_PROGRESS_FILES_VAL, s);
            }
            s[0] = 0;
            if (IS_DEFINED_VAL(totalFiles)) {
                MyStringCopy(s, L" / ");
                ConvertUInt64ToString(totalFiles, s + MyStringLen(s));
            }
            if (_filesTotStr_Prev != s) {
                _filesTotStr_Prev = s;
                SetItemText(IDT_PROGRESS_FILES_TOTAL, s);
            }
        }
        const UInt64 packSize = CompressingMode ? outSize : inSize;
        const UInt64 unpackSize = CompressingMode ? inSize : outSize;
        if (IS_UNDEFINED_VAL(unpackSize) && IS_UNDEFINED_VAL(packSize)) {
            ShowSize(IDT_PROGRESS_PROCESSED_VAL, completed, _processed_Prev);
            ShowSize(IDT_PROGRESS_PACKED_VAL, UNDEFINED_VAL, _packed_Prev);
        } else {
            ShowSize(IDT_PROGRESS_PROCESSED_VAL, unpackSize, _processed_Prev);
            ShowSize(IDT_PROGRESS_PACKED_VAL, packSize, _packed_Prev);
            if (IS_DEFINED_VAL(packSize) && IS_DEFINED_VAL(unpackSize) && unpackSize != 0) {
                wchar_t s[32];
                UInt64 ratio = packSize * 100 / unpackSize;
                if (_ratio_Prev != ratio) {
                    _ratio_Prev = ratio;
                    ConvertUInt64ToString(ratio, s);
                    MyStringCat(s, L"%");
                    SetItemText(IDT_PROGRESS_RATIO_VAL, s);
                }
            }
        }
    }
    if (needSetTitle || titleFileName_Changed)
        SetTitleText();
    if (status_Changed) {
        UString s = _status;
        ReduceString(s, _numReduceSymbols);
        SetItemText(IDT_PROGRESS_STATUS, s);
    }
    if (curFilePath_Changed) {
        UString s1, s2;
        if (_isDir)
            s1 = _filePath;
        else {
            int slashPos = _filePath.ReverseFind_PathSepar();
            if (slashPos >= 0) {
                s1.SetFrom(_filePath, (unsigned)(slashPos + 1));
                s2 = _filePath.Ptr((unsigned)(slashPos + 1));
            } else
                s2 = _filePath;
        }
        ReduceString(s1, _numReduceSymbols);
        ReduceString(s2, _numReduceSymbols);
        s1.Add_LF();
        s1 += s2;
        SetItemText(IDT_PROGRESS_FILE_NAME, s1);
    }
}

void CProgressDialog::SetTitleText() {
    UString s;
    if (Sync.Get_Paused()) {
        s += _paused_String;
        s.Add_Space();
    }
    if (IS_DEFINED_VAL(_prevPercentValue)) {
        s.Add_UInt64(_prevPercentValue);
        s.Add_Char('%');
    }
    if (_background) {
        s.Add_Space();
        s += _backgrounded_String;
    }
    s.Add_Space();
    {
        unsigned len = s.Len();
        s += MainAddTitle;
        AddToTitle(s);
        s.DeleteFrom(len);
    }
    s += _title;
    if (!_titleFileName.IsEmpty()) {
        UString fileName = _titleFileName;
        ReduceString(fileName, kTitleFileNameSizeLimit);
        s.Add_Space();
        s += fileName;
    }
    SetText(s);
}

void CProgressDialog::SetPauseText() {
    SetItemText(IDB_PAUSE, Sync.Get_Paused() ? _continue_String : _pause_String);
    SetTitleText();
}

void CProgressDialog::OnPauseButton() {
    bool paused = !Sync.Get_Paused();
    Sync.Set_Paused(paused);
    UInt32 curTime = GetTickCount();
    if (paused)
        _elapsedTime += (curTime - _prevTime);
    // Taskbar paused-state service follows in mandatory b.
    _prevTime = curTime;
    SetPauseText();
}

void CProgressDialog::SetPriorityText() {
    SetItemText(IDB_PROGRESS_BACKGROUND, _background ? _foreground_String : _background_String);
    SetTitleText();
}

void CProgressDialog::AddMessageDirect(const wchar_t *message, bool needNumber) {
    wchar_t sz[16];
    sz[0] = 0;
    if (needNumber)
        ConvertUInt32ToString(_numMessages + 1, sz);
    const unsigned itemIndex = _messageStrings.Size();
    if (_messageList.InsertItem(itemIndex, sz) == (int)itemIndex) {
        _messageList.SetSubItem(itemIndex, 1, message);
        _messageStrings.Add(message);
    }
}

void CProgressDialog::AddMessage(const wchar_t *message) {
    UString s = message;
    bool needNumber = true;
    while (!s.IsEmpty()) {
        const int pos = s.Find(L'\n');
        if (pos < 0)
            break;
        AddMessageDirect(s.Left((unsigned)pos), needNumber);
        needNumber = false;
        s.DeleteFrontal((unsigned)pos + 1);
    }
    AddMessageDirect(s, needNumber);
    _numMessages++;
}

void CProgressDialog::UpdateMessagesDialog() {
    UStringVector messages;
    {
        NSynchronization::CCriticalSectionLock lock(Sync._cs);
        const unsigned num = Sync.Messages.Size();
        if (num > _numPostedMessages) {
            messages.ClearAndReserve(num - _numPostedMessages);
            for (unsigned i = _numPostedMessages; i < num; i++)
                messages.AddInReserved(Sync.Messages[i]);
            _numPostedMessages = num;
        }
    }
    if (!messages.IsEmpty()) {
        FOR_VECTOR(i, messages)
        AddMessage(messages[i]);
        if (_numPostedMessages < 1000000 || _numAutoSizeMessages < 100)
            if (_numAutoSizeMessages < 100 ||
                GetNumDigits(_numPostedMessages) > GetNumDigits(_numAutoSizeMessages)) {
                _messageList.SetColumnWidthAuto(0);
                _messageList.SetColumnWidthAuto(1);
                _numAutoSizeMessages = _numPostedMessages;
            }
    }
}

static void ListView_GetSelected(CQtProgressListView &listView, CUIntVector &vector) {
    vector.Clear();
    int index = -1;
    for (;;) {
        index = listView.GetNextSelectedItem(index);
        if (index < 0)
            break;
        vector.Add((unsigned)index);
    }
}

void CProgressDialog::CopyToClipboard() {
    CUIntVector indexes;
    ListView_GetSelected(_messageList, indexes);
    UString s;
    unsigned numIndexes = indexes.Size();
    if (numIndexes == 0)
        numIndexes = (unsigned)_messageList.GetItemCount();
    for (unsigned i = 0; i < numIndexes; i++) {
        const unsigned index = (i < indexes.Size() ? indexes[i] : i);
        s += _messageStrings[index];
        {
            s +=
#ifdef _WIN32
                "\r\n"
#else
                "\n"
#endif
                ;
        }
    }
    ClipboardSetText(s);
}

void CProgressDialog::showEvent(QShowEvent *event) {
    if (!_wasCreated)
        OnInit();
    OnSize(width(), height());
    CQtProgressDialog::showEvent(event);
}
void CProgressDialog::resizeEvent(QResizeEvent *event) {
    if (_wasCreated)
        OnSize(event->size().width(), event->size().height());
    CQtProgressDialog::resizeEvent(event);
}
void CProgressDialog::timerEvent(QTimerEvent *event) {
    if (event->timerId() == _timer) {
        OnTimer();
        return;
    }
    CQtProgressDialog::timerEvent(event);
}
bool CProgressDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == GetItem(IDL_PROGRESS_MESSAGES) && event->type() == QEvent::KeyPress &&
        OnNotify(static_cast<QKeyEvent *>(event)))
        return true;
    return CQtProgressDialog::eventFilter(watched, event);
}
bool CProgressDialog::OnTimer() {
    if (Sync.Get_Paused())
        return true;
    // CheckNeedClose/worker completion follows in mandatory b3.
    UpdateStatInfo(false);
    return true;
}
bool CProgressDialog::OnNotify(QKeyEvent *key) {
    const bool control = key->modifiers().testFlag(Qt::ControlModifier) ||
                         key->modifiers().testFlag(Qt::MetaModifier);
    switch (key->key()) {
    case Qt::Key_A:
        if (control) {
            static_cast<CListView *>(GetItem(IDL_PROGRESS_MESSAGES))->selectAll();
            return true;
        }
        break;
    case Qt::Key_Insert:
    case Qt::Key_C:
        if (control) {
            CopyToClipboard();
            return true;
        }
        break;
    }
    return false;
}
void CProgressDialog::OnOK() {}
void CProgressDialog::accept() { OnOK(); }
void CProgressDialog::reject() {} // Full source cancel transition follows in b3.
