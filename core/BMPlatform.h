#pragma once

#include <Windows.h>
#include <tchar.h>

#define INVALID_VALUE ((DWORD)-1)
#define INFINITE_LOGFILE_SIZE (0)

typedef enum {
    SPLOGLV_NONE = 0,
    SPLOGLV_ERROR = 1,
    SPLOGLV_WARN = 2,
    SPLOGLV_INFO = 3,
    SPLOGLV_DATA = 4,
    SPLOGLV_VERBOSE = 5
} SPLOG_LEVEL;

enum {
    LOG_READ = 0,
    LOG_WRITE = 1,
    LOG_ASYNC_READ = 2
};

typedef enum {
    CHANNEL_TYPE_COM,
    CHANNEL_TYPE_USB
} CHANNEL_TYPE;

typedef struct {
    CHANNEL_TYPE ChannelType;
    union {
        struct {
            DWORD dwPortNum;
            DWORD dwBaudRate;
        } Com;
        struct {
            DWORD dwVID;
            DWORD dwPID;
        } Usb;
    };
} CHANNEL_ATTRIBUTE, *PCHANNEL_ATTRIBUTE;

class ISpLog {
public:
    enum OpenFlags {
        modeTimeSuffix = 0x0001,
        modeDateSuffix = 0x0002,
        modeCreate = 0x1000,
        modeNoTruncate = 0x2000,
        typeText = 0x4000,
        typeBinary = 0x8000,
        modeBinNone = 0x0000,
        modeBinRead = 0x0100,
        modeBinWrite = 0x0200,
        modeBinReadWrite = 0x0300,
        defaultTextFlag = modeCreate | modeDateSuffix | typeText,
        defaultBinaryFlag = modeCreate | modeDateSuffix | typeBinary | modeBinRead,
        defaultDualFlag = modeCreate | modeDateSuffix | typeText | typeBinary | modeBinRead
    };

    virtual ~ISpLog() {}

    virtual BOOL Open(LPCTSTR lpszLogPath,
                     UINT uFlags = defaultTextFlag,
                     UINT uLogLevel = SPLOGLV_INFO,
                     UINT uMaxFileSizeInMB = INFINITE_LOGFILE_SIZE) = 0;

    virtual BOOL Close() = 0;

    virtual void Release() = 0;

    virtual BOOL LogHexData(const BYTE *pHexData, DWORD dwHexSize, UINT uFlag = LOG_READ) = 0;

    virtual BOOL LogRawStrA(UINT uLogLevel, LPCSTR lpszString) = 0;
    virtual BOOL LogRawStrW(UINT uLogLevel, LPCWSTR lpszString) = 0;
};

class ICommChannel {
public:
    virtual ~ICommChannel() {}

    virtual BOOL InitLog(LPCWSTR pszLogName, UINT uiLogType,
                         UINT uiLogLevel = INVALID_VALUE, ISpLog *pLogUtil = NULL,
                         LPCWSTR pszBinLogFileExt = NULL) = 0;

    virtual BOOL SetReceiver(ULONG ulMsgId, BOOL bRcvThread, LPCVOID pReceiver) = 0;

    virtual void GetReceiver(ULONG &ulMsgId, BOOL &bRcvThread, LPVOID &pReceiver) = 0;

    virtual BOOL Open(PCHANNEL_ATTRIBUTE pOpenArgument) = 0;

    virtual void Close() = 0;

    virtual BOOL Clear() = 0;

    virtual DWORD Read(LPVOID lpData, DWORD dwDataSize, DWORD dwTimeOut, DWORD dwReserved = 0) = 0;

    virtual DWORD Write(LPVOID lpData, DWORD dwDataSize, DWORD dwReserved = 0) = 0;

    virtual void FreeMem(LPVOID pMemBlock) = 0;

    virtual BOOL GetProperty(LONG lFlags, DWORD dwPropertyID, LPVOID pValue) = 0;

    virtual BOOL SetProperty(LONG lFlags, DWORD dwPropertyID, LPCVOID pValue) = 0;
};

typedef BOOL (*pfCreateChannel)(ICommChannel **, CHANNEL_TYPE);
typedef void (*pfReleaseChannel)(ICommChannel *);

class CBMPlatformApp {
public:
    CBMPlatformApp();

    pfCreateChannel m_pfCreateChannel;
    pfReleaseChannel m_pfReleaseChannel;

    HMODULE m_hChannelLib;

    BOOL InitInstance();
    int ExitInstance();
};

class CBootModeOpr {
public:
    CBootModeOpr();
    ~CBootModeOpr();
    BOOL Initialize();
    void Uninitialize();
    int Read(UCHAR *m_RecvData, int max_len, int dwTimeout);
    int Write(UCHAR *lpData, int iDataSize);
    BOOL ConnectChannel(DWORD dwPort, ULONG ulMsgId, DWORD Receiver);
    BOOL DisconnectChannel();
    BOOL GetProperty(LONG lFlags, DWORD dwPropertyID, LPVOID pValue);
    BOOL SetProperty(LONG lFlags, DWORD dwPropertyID, LPCVOID pValue);
    void Clear();
    void FreeMem(LPVOID pMemBlock);
private:
    ICommChannel *m_pChannel;
};