#include "BMPlatform.h"
#include "common.h"
#include <iostream>

int m_bOpened = 0;

CBMPlatformApp::CBMPlatformApp() {
    m_pfCreateChannel = NULL;
    m_pfReleaseChannel = NULL;
    m_hChannelLib = NULL;
}

BOOL CBMPlatformApp::InitInstance() {
    m_hChannelLib = LoadLibrary(_T("Channel9.dll"));

    if (m_hChannelLib == NULL) {
        std::cout << "Failed to load Channel9.dll. Error code: " << GetLastError() << std::endl;
        return FALSE;
    }

    m_pfCreateChannel = (pfCreateChannel)GetProcAddress(m_hChannelLib, "CreateChannel");
    m_pfReleaseChannel = (pfReleaseChannel)GetProcAddress(m_hChannelLib, "ReleaseChannel");

    if (m_pfCreateChannel == NULL || m_pfReleaseChannel == NULL) {
        return FALSE;
    }

    return TRUE;
}

int CBMPlatformApp::ExitInstance() {
    if (m_hChannelLib) {
        FreeLibrary(m_hChannelLib);
        m_hChannelLib = NULL;
    }
    return TRUE;
}

CBMPlatformApp g_theApp;

CBootModeOpr::CBootModeOpr() {
    m_pChannel = NULL;
    g_theApp.InitInstance();
}

CBootModeOpr::~CBootModeOpr() {
    g_theApp.ExitInstance();
}

BOOL CBootModeOpr::Initialize() {
    if (!g_theApp.m_pfCreateChannel) {
        return FALSE;
    }
    if (!g_theApp.m_pfCreateChannel((ICommChannel **)&m_pChannel, CHANNEL_TYPE_COM)) {
        return FALSE;
    }
    return TRUE;
}

void CBootModeOpr::Uninitialize() {
    if (g_theApp.m_pfReleaseChannel) g_theApp.m_pfReleaseChannel(m_pChannel);
    m_pChannel = NULL;
}

int CBootModeOpr::Read(UCHAR *m_RecvData, int max_len, int dwTimeout) {
    ULONGLONG tBegin;
    ULONGLONG tCur;
    tBegin = GetTickCount64();
    do {
        tCur = GetTickCount64();
        DWORD dwRead = m_pChannel->Read(m_RecvData, max_len, dwTimeout);
        if (dwRead) {
            return dwRead;
        }
    } while ((tCur - tBegin) < dwTimeout);
    return 0;
}

int CBootModeOpr::Write(UCHAR *lpData, int iDataSize) {
    return m_pChannel->Write(lpData, iDataSize);
}

BOOL CBootModeOpr::GetProperty(LONG lFlags, DWORD dwPropertyID, LPVOID pValue) {
    return m_pChannel->GetProperty(lFlags, dwPropertyID, pValue);
}

BOOL CBootModeOpr::SetProperty(LONG lFlags, DWORD dwPropertyID, LPCVOID pValue) {
    return m_pChannel->SetProperty(lFlags, dwPropertyID, pValue);
}

BOOL CBootModeOpr::ConnectChannel(DWORD dwPort, ULONG ulMsgId, DWORD Receiver) {
    if (!dwPort) return FALSE;

    if (Receiver) m_pChannel->SetReceiver(ulMsgId, TRUE, reinterpret_cast<LPVOID>(static_cast<uintptr_t>(Receiver)));
    CHANNEL_ATTRIBUTE ca;
    ca.ChannelType = CHANNEL_TYPE_COM;
    ca.Com.dwPortNum = dwPort;
    ca.Com.dwBaudRate = 115200;

    m_bOpened = m_pChannel->Open(&ca);
    if (m_bOpened) {
        std::cout << "Successfully connected to port: " << dwPort << std::endl;
        spdio_t *io = spdio_init(0);
        if (io) {
            io->handle = m_pChannel;
            io->is_open = 1;
            CreateRecvThread(io);
        }
    }
    return m_bOpened;
}

BOOL CBootModeOpr::DisconnectChannel() {
    m_pChannel->Close();
    m_bOpened = 0;
    return TRUE;
}

void CBootModeOpr::Clear() {
    m_pChannel->Clear();
}

void CBootModeOpr::FreeMem(LPVOID pMemBlock) {
    m_pChannel->FreeMem(pMemBlock);
}
