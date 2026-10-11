#pragma once

#include "MainFrm.h"
#include "SMS.h"
#include "BrfLoader.h"

class CModuleInfo
{
public:
    CModuleInfo() {}

    CString Path;
};


class CMainFrame;
class CMainCore
{
public:
    CMainCore(void);
    ~CMainCore(void);

    

    void LoadModule(UINT iModule);

    void LoadIni();

public:

    CString                  m_strGamePath;
    CString                  m_strGameVersion;

    CZJArray <CModuleInfo>   m_Modules;
    
    LPWSTR                   m_ModulePath;

    CMainFrame              *m_pMainFrm;
    void                     SetMainFrm(CMainFrame* pMainFrm)
    {
        m_pMainFrm = pMainFrm;
        m_BrfLoader.m_pDevice = pMainFrm->GetDevice();
        m_BrfLoader.m_pManager = &pMainFrm->m_Manager;
    }

    char                    *m_strIniFile;
    CZJPointerArray <char*>  m_BrfFileNames;

    // Thread
    //HANDLE                  m_hThread;
    //DWORD                   m_ThreadID;
    void                    _LoadModule();

    CBrfLoader              m_BrfLoader;

    // For Render
    UINT                    m_nLoadedBrfFiles;
    UINT                    m_nTotalBrfFiles;
    CZJGUIProgressBar*      m_pProgressBar;
    CZJGUIStatic*           m_pLoadingFileText;
#if defined(DEBUG)
    DWORD                   m_dwStartTime;
    CZJGUIStatic*           m_pCostTimeText;
#endif
};

extern CMainCore theCore;


const wchar_t tmp_Skls[][16] = 
{
    L"����",
    L"����",
    L"ǿ��",
    L"ǿ��",
    L"����",
    L"�ܷ�",
    L"�ܶ�",
    L"����",
    L"����",
    L"����",
    L"����",
    L"����",
    L"ı��",
    L"�о�",
    L"��Ұ",
    L"��Ʒ����",
    L"����",
    L"ҽ��",
    L"����",
    L"����ѧ",
    L"˵��",
    L"��²����",
    L"ͳ��",
    L"����"
};