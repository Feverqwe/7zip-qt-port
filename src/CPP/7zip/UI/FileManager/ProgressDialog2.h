// Adapted from 7-Zip CPP/7zip/UI/FileManager/ProgressDialog2.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Complete native CProgressSync and source presentation subset; full modal
// dialog/thread and priority/default-button integration follow separately.
#ifndef ZIP7_INC_PROGRESS_DIALOG_2_H
#define ZIP7_INC_PROGRESS_DIALOG_2_H

#include "Common/MyCom.h"
#include "Windows/ErrorMsg.h"
#include "Windows/Synchronization.h"
#include "qtprogresscontrols.h"
#include <QPointer>
#ifdef _WIN32
#include "7zip/UI/FileManager/MyWindowsNew.h"
#else
#include "qttaskbarprogress.h"
#endif

void GetTimeString(UInt64 timeValue, wchar_t *s);

struct CProgressMessageBoxPair {
    UString Title;
    UString Message;
};

struct CProgressFinalMessage {
    CProgressMessageBoxPair ErrorMessage;
    CProgressMessageBoxPair OkMessage;

    bool ThereIsMessage() const {
        return !ErrorMessage.Message.IsEmpty() || !OkMessage.Message.IsEmpty();
    }
};

class CProgressSync {
    bool _stopped;
    bool _paused;

  public:
    bool _filesProgressMode;
    bool _isDir;
    UInt64 _totalBytes;
    UInt64 _completedBytes;
    UInt64 _totalFiles;
    UInt64 _curFiles;
    UInt64 _inSize;
    UInt64 _outSize;

    UString _titleFileName;
    UString _status;
    UString _filePath;

    UStringVector Messages;
    CProgressFinalMessage FinalMessage;

    NWindows::NSynchronization::CCriticalSection _cs;

    CProgressSync();
    bool Get_Stopped() {
        NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
        return _stopped;
    }
    void Set_Stopped(bool val) {
        NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
        _stopped = val;
    }
    bool Get_Paused();
    void Set_Paused(bool val) {
        NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
        _paused = val;
    }
    void Set_FilesProgressMode(bool filesProgressMode) {
        NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
        _filesProgressMode = filesProgressMode;
    }
    HRESULT CheckStop();
    void Clear_Stop_Status();
    HRESULT ScanProgress(UInt64 numFiles, UInt64 totalSize, const FString &fileName,
                         bool isDir = false);
    HRESULT Set_NumFilesTotal(UInt64 val);
    void Set_NumBytesTotal(UInt64 val);
    void Set_NumFilesCur(UInt64 val);
    HRESULT Set_NumBytesCur(const UInt64 *val);
    HRESULT Set_NumBytesCur(UInt64 val);
    void Set_Ratio(const UInt64 *inSize, const UInt64 *outSize);
    void Set_TitleFileName(const UString &fileName);
    void Set_Status(const UString &s);
    HRESULT Set_Status2(const UString &s, const wchar_t *path, bool isDir = false);
    void Set_FilePath(const wchar_t *path, bool isDir = false);
    void AddError_Message(const wchar_t *message);
    void AddError_Message_Name(const wchar_t *message, const wchar_t *name);
    // void AddError_Code_Name(DWORD systemError, const wchar_t *name);
    void AddError_Code_Name(HRESULT systemError, const wchar_t *name);
    bool ThereIsMessage() const { return !Messages.IsEmpty() || FinalMessage.ThereIsMessage(); }
};

// Source presentation subset. Full modal/thread/cancel and priority/default-button
// services remain mandatory subsequent ports; this view is not yet an app owner.
class CProgressDialog : public CQtProgressDialog {
    bool _isDir;
    bool _wasCreated;
    bool _needClose;
    bool _errorsWereDisplayed;
    bool _waitCloseByCancelButton;
    bool _cancelWasPressed;
    bool _inCancelMessageBox;
    bool _externalCloseMessageWasReceived;
    bool _background;

  public:
    bool WaitMode;
    bool MessagesDisplayed;
    bool CompressingMode;
    bool ShowCompressionInfo;

  private:
    unsigned _numPostedMessages;
    unsigned _numAutoSizeMessages;
    unsigned _numMessages;
    UString _titleFileName;
    UString _filePath;
    UString _status;
    UString _background_String;
    UString _backgrounded_String;
    UString _foreground_String;
    UString _pause_String;
    UString _continue_String;
    UString _paused_String;
    int _buttonSizeX;
    int _buttonSizeY;
    int _timer;
    UString _title;

    class CU64ToI32Converter {
        unsigned _numShiftBits;
        UInt64 _range;

      public:
        CU64ToI32Converter() : _numShiftBits(0), _range(1) {}
        void Init(UInt64 range) {
            _range = range;
            // Windows CE doesn't like big number for ProgressBar.
            for (_numShiftBits = 0; range >= ((UInt32)1 << 15); _numShiftBits++)
                range >>= 1;
        }
        int Count(UInt64 val) {
            int res = (int)(val >> _numShiftBits);
            if (val == _range)
                res++;
            return res;
        }
    };
    CU64ToI32Converter _progressConv;
    UInt64 _progressBar_Pos;
    UInt64 _progressBar_Range;
    CQtProgressBar m_ProgressBar;
    CQtProgressListView _messageList;
    UStringVector _messageStrings;
#ifdef _WIN32
    CMyComPtr<ITaskbarList3> _taskbarList;
#else
    std::unique_ptr<CQtTaskbarProgress> _taskbarList;
#endif
    // Guarded Qt window lifetime; convert to actual HWND only at the Win32 call.
    QPointer<QWidget> _hwndForTaskbar;
    UInt32 _prevTime;
    UInt64 _elapsedTime;
    UInt64 _prevPercentValue;
    UInt64 _prevElapsedSec;
    UInt64 _prevRemainingSec;
    UInt64 _totalBytes_Prev;
    UInt64 _processed_Prev;
    UInt64 _packed_Prev;
    UInt64 _ratio_Prev;
    UString _filesStr_Prev;
    UString _filesTotStr_Prev;
    unsigned _numReduceSymbols;
    unsigned _prevSpeed_MoveBits;
    UInt64 _prevSpeed;
    NWindows::NSynchronization::CManualResetEvent _createDialogEvent;
    NWindows::NSynchronization::CManualResetEvent _dialogCreatedEvent;

    void AddToTitle(const wchar_t *s);
    void SetTaskbarProgressState(TBPFLAG tbpFlags) {
        if (_taskbarList && _hwndForTaskbar) {
#ifdef _WIN32
            _taskbarList->SetProgressState(GetNativeTaskbarWindow(_hwndForTaskbar), tbpFlags);
#else
            _taskbarList->SetProgressState(_hwndForTaskbar, tbpFlags);
#endif
        }
    }
    void SetTaskbarProgressState();
    void SetProgressRange(UInt64 range);
    void SetProgressPos(UInt64 pos);
    void ShowSize(unsigned id, UInt64 val, UInt64 &prev);
    void UpdateStatInfo(bool showAll);
    bool OnSize(int xSize, int ySize);
    void SetTitleText();
    void SetPauseText();
    void SetPriorityText();
    void OnPauseButton();
    void EnableErrorsControls(bool enable);
    void AddMessageDirect(const wchar_t *message, bool needNumber);
    void AddMessage(const wchar_t *message);
    void UpdateMessagesDialog();
    void CopyToClipboard();
    void OnOK();
    bool OnTimer();
    bool OnNotify(QKeyEvent *event);
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

  public:
    CProgressSync Sync;
    int IconID;
    QPointer<QWidget> MainWindow;
    UString MainTitle;
    UString MainAddTitle;
    explicit CProgressDialog(QWidget *parent = nullptr, const UString &title = UString());
    ~CProgressDialog() override;
    bool OnInit();
    void accept() override;
    void reject() override;
};

#endif
