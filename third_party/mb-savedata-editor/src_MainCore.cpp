#include "StdAfx.h"
#include "MainCore.h"


CMainCore::CMainCore(void)
{
    m_ModulePath = NULL;
    m_strIniFile = NULL;
}


CMainCore::~CMainCore(void)
{
    SAFE_DELETE_ARRAY(m_ModulePath);
    SAFE_DELETE_ARRAY(m_strIniFile);
}

CMainCore theCore;

void __cdecl _LoadModule(LPVOID lpThreadParameter)
{
    theCore._LoadModule();
}

void CMainCore::LoadModule(UINT iModule)
{
    if (m_Modules[iModule].Path.Right(1) != _T("\\"))
        m_Modules[iModule].Path += _T("\\");
    int len = m_Modules[iModule].Path.GetLength();
    m_ModulePath = new WCHAR[len + 1];
    wcscpy_s(m_ModulePath, len + 1, m_Modules[iModule].Path.GetBuffer());
    m_Modules.RemoveAll();

    m_dwStartTime = GetTickCount();
    m_pMainFrm->m_GUI.TurnView(m_pMainFrm->IDV_LOADSCREEN);

    LoadIni();

    _beginthread(::_LoadModule, 0, NULL);
    /*
    m_hThread = CreateThread(NULL,
                             0,
                             ::_LoadModule,
                             NULL,
                             0,
                             &m_ThreadID);
                             */
}


void CMainCore::LoadIni()
{
    FILE *fp;

    wchar_t path[MAX_PATH];
    wcscpy_s(path, m_ModulePath);
    wcscat_s(path, L"module.ini");

    _wfopen_s(&fp, path, L"rb");

    fseek(fp, 0L, SEEK_END);
    int nFileLength = ftell(fp);
    SAFE_DELETE_ARRAY(m_strIniFile);
    m_strIniFile = new char[nFileLength + 1];

    fseek(fp, 0L, SEEK_SET);
    fread(m_strIniFile, nFileLength + 1, 1, fp);
    fclose(fp);

    // ���ļ����ݶ��뵽CZJReader ����������
    CZJReader reader;
    reader.Reader(m_strIniFile);

    m_BrfFileNames.RemoveAll();

    char *str;
    
    while (reader.NotEnd())
    {
        str = reader.GetLPSTR();

        if (0 == strcmp(str, "load_resource"))
        {
            reader.IgnoreUntil('=');
            reader.Ignore();

            m_BrfFileNames.Add(reader.GetLPSTR());
        }
        else if (0 == strcmp(str, "load_mod_resource"))
        {
            reader.IgnoreUntil('=');
            reader.Ignore();

            m_BrfFileNames.Add(reader.GetLPSTR());
        }


        delete[] str;

        reader.GotoNextLine();
    }

    m_nTotalBrfFiles = m_BrfFileNames.GetSize();
}

void CMainCore::_LoadModule()
{
    wchar_t path[MAX_PATH], wstr[MAX_PATH];
    m_pLoadingFileText->SetText(_T("������Դ..."));
    for (int i = 0; i < m_nTotalBrfFiles; ++i)
    {
        m_pProgressBar->SetProgress(float(i) / m_nTotalBrfFiles);
        MultiByteToWideChar(CP_ACP, 0, m_BrfFileNames[i], -1, wstr, MAX_PATH);
        //m_pLoadingFileText->SetText(wstr);
        swprintf_s(path, L"%sCommonRes\\%s.brf", m_strGamePath.GetBuffer(), wstr);
        m_BrfLoader.Load(path);
    }
    m_pProgressBar->SetProgress(1.0f);

    theSMS.LoadItemKinds();
    theSMS.LoadTroops();
    
    
    m_pMainFrm->m_GUI.TurnView(m_pMainFrm->IDV_MAINMENU);

#if defined(DEBUG)
    CString sz;
    sz.Format(_T("���غ�ʱ%d"), GetTickCount() - m_dwStartTime);
    AfxMessageBox(sz);
#endif

    _endthread();
    //ExitThread(0);
}