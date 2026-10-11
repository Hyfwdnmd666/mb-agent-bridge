#include "StdAfx.h"
#include "SMS.h"

CSMS::CSMS(void)
    : m_pMenuData(NULL)
    , m_pLanguageData(NULL)
    , m_numLanguageNodes(0)
    , m_ppLanguageNodes(NULL)
    , m_ppLanguageStrings(NULL)
{
}


CSMS::~CSMS(void)
{
    SAFE_DELETE_ARRAY(m_pMenuData);
    SAFE_DELETE_ARRAY(m_pMenus);

    for (DWORD iNode = 0; iNode < m_numLanguageNodes; ++iNode)
    {
        SAFE_DELETE_ARRAY(m_ppLanguageNodes[iNode]);
        SAFE_DELETE_ARRAY(m_ppLanguageStrings[iNode]);
    }
    SAFE_DELETE_ARRAY(m_ppLanguageNodes);
    SAFE_DELETE_ARRAY(m_ppLanguageStrings);
    SAFE_DELETE_ARRAY(m_pLanguageData);
}

void CSMS::LoadMenuData()
{
    m_ModulePath = _T("C:\\Program Files\\Mount&Blade Warband\\Modules\\Native\\");

    FILE *fp;
    fopen_s(&fp, "C:\\Program Files\\Mount&Blade Warband\\Modules\\Native\\menus.txt","rb");
    fseek(fp, 0L, SEEK_END);
    
    int nFileLength = ftell(fp);
    SAFE_DELETE_ARRAY(m_pMenuData);
    m_pMenuData = new char[nFileLength + 1];

    fseek(fp, 0L, SEEK_SET);
    fread(m_pMenuData, nFileLength, 1, fp);
    fclose(fp);    


    // ���ļ����ݶ��뵽CZJString ��������������
    CZJReader reader;
    reader.Reader(m_pMenuData);
    
    reader.GotoNextLine();
    m_numMenus = reader.GetUINT();
    m_pMenus = new stMenu[m_numMenus];
    //while (reader.NotEnd())
    for (DWORD iMenu = 0; iMenu < m_numMenus; ++iMenu)
    {
        m_pMenus[iMenu].Name = reader.GetLPSTR();
        m_pMenus[iMenu].flag = reader.GetULONGLONG();
        m_pMenus[iMenu].text = reader.GetLPSTR();

        reader.Ignore();
        reader.IgnoreUntil(' ');

        m_pMenus[iMenu].numOperations = reader.GetUINT();
        m_pMenus[iMenu].pOperations = new stOperation[m_pMenus[iMenu].numOperations];
        for (DWORD iOperation = 0; iOperation < m_pMenus[iMenu].numOperations; ++iOperation)
        {
            m_pMenus[iMenu].pOperations[iOperation].id = reader.GetUINT();
            m_pMenus[iMenu].pOperations[iOperation].numArg = reader.GetUINT();
            m_pMenus[iMenu].pOperations[iOperation].pArgs = new LONGLONG[m_pMenus[iMenu].pOperations[iOperation].numArg];
            for (int iArg = 0; iArg < m_pMenus[iMenu].pOperations[iOperation].numArg; ++iArg)
            {
                m_pMenus[iMenu].pOperations[iOperation].pArgs[iArg] = reader.GetLONGLONG();
            }
        }

        m_pMenus[iMenu].numOptions = reader.GetUINT();
        m_pMenus[iMenu].pOptions = new stOption[m_pMenus[iMenu].numOptions];
        for (DWORD iOption = 0; iOption < m_pMenus[iMenu].numOptions; ++iOption)
        {
            m_pMenus[iMenu].pOptions[iOption].Name = reader.GetLPSTR();

            m_pMenus[iMenu].pOptions[iOption].numConditions = reader.GetUINT();
            m_pMenus[iMenu].pOptions[iOption].pConditions = new stOperation[m_pMenus[iMenu].pOptions[iOption].numConditions];
            for (DWORD iCondition = 0; iCondition < m_pMenus[iMenu].pOptions[iOption].numConditions; ++iCondition)
            {
                m_pMenus[iMenu].pOptions[iOption].pConditions[iCondition].id = reader.GetUINT();
                m_pMenus[iMenu].pOptions[iOption].pConditions[iCondition].numArg = reader.GetUINT();
                m_pMenus[iMenu].pOptions[iOption].pConditions[iCondition].pArgs = new LONGLONG[m_pMenus[iMenu].pOptions[iOption].pConditions[iCondition].numArg];
                for (int iArg = 0; iArg < m_pMenus[iMenu].pOptions[iOption].pConditions[iCondition].numArg; ++iArg)
                {
                    m_pMenus[iMenu].pOptions[iOption].pConditions[iCondition].pArgs[iArg] = reader.GetLONGLONG();
                }
            }

            m_pMenus[iMenu].pOptions[iOption].text = reader.GetLPSTR();

            m_pMenus[iMenu].pOptions[iOption].numConsequences = reader.GetUINT();
            m_pMenus[iMenu].pOptions[iOption].pConsequences = new stOperation[m_pMenus[iMenu].pOptions[iOption].numConsequences];
            for (DWORD iConsequence = 0; iConsequence < m_pMenus[iMenu].pOptions[iOption].numConsequences; ++iConsequence)
            {
                m_pMenus[iMenu].pOptions[iOption].pConsequences[iConsequence].id = reader.GetUINT();
                m_pMenus[iMenu].pOptions[iOption].pConsequences[iConsequence].numArg = reader.GetUINT();
                m_pMenus[iMenu].pOptions[iOption].pConsequences[iConsequence].pArgs = new LONGLONG[m_pMenus[iMenu].pOptions[iOption].pConsequences[iConsequence].numArg];
                for (int iArg = 0; iArg < m_pMenus[iMenu].pOptions[iOption].pConsequences[iConsequence].numArg; ++iArg)
                {
                    m_pMenus[iMenu].pOptions[iOption].pConsequences[iConsequence].pArgs[iArg] = reader.GetLONGLONG();
                }
            }


            reader.IgnoreLPSTR(); // door_name;
        }
    }
}



void CSMS::LoadLanguagesData()
{
    FILE *fp;
    fopen_s(&fp, "C:\\Program Files\\Mount&Blade Warband\\Modules\\Native\\languages\\cns\\game_menus.csv","rb");
    fseek(fp, 0L, SEEK_END);
    
    int nFileLength = ftell(fp);
    SAFE_DELETE_ARRAY(m_pLanguageData);
    m_pLanguageData = new char[nFileLength + 1];

    fseek(fp, 0L, SEEK_SET);
    fread(m_pLanguageData, nFileLength, 1, fp);
    fclose(fp);    


    // ���ļ����ݶ��뵽CZJString ��������������
    CZJReader reader;
    reader.Reader(m_pLanguageData);
    
    m_numLanguageNodes = reader.CountAll('|');
    ASSERT(m_numLanguageNodes != 0);
    m_ppLanguageNodes = new char*[m_numLanguageNodes];//new wchar_t*[m_numLanguageNodes];
    m_ppLanguageStrings = new wchar_t*[m_numLanguageNodes];
    char *str;
    int len;
    //ATL::CStringT< char, StrTraitMFC_DLL< char > > sz;
    CString sz;
    wchar_t *wstr;
    for (DWORD iNode = 0; iNode < m_numLanguageNodes; ++iNode)
    {
        m_ppLanguageNodes[iNode] = reader.GetsUntil('|');
        /*
        str = reader.GetsUntil('|');
        len = strlen(str);
        m_ppLanguageNodes[iNode] = new wchar_t[len + 1];
        MultiByteToWideChar(CP_UTF8, 0, str, -1, m_ppLanguageNodes[iNode], len + 1);
        SAFE_DELETE_ARRAY(str);*/

        reader.Ignore();
        
        str = reader.GetsUntil('\n');
        len = strlen(str);
        wstr = new wchar_t[len + 1];
        MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len + 1);
        SAFE_DELETE_ARRAY(str);
        sz = wstr;
        SAFE_DELETE_ARRAY(wstr);
        sz.Replace(_T(" "), _T(""));
        len = sz.GetLength();
        m_ppLanguageStrings[iNode] = new wchar_t[len + 1];
        wcscpy_s(m_ppLanguageStrings[iNode], len + 1, sz.GetBuffer());

        reader.Ignore();
    }
}

//const wchar_t strOperations[][32] =
//{
    
DWORD GetOperationDesc(CString* pStr, stOperation& operation, DWORD depth)
{
}

void CSMS::UpdateOptionInfo(DWORD iOption)
{
    m_strOptionCondition.Empty();
    stOption *pOption = &m_pMenus[m_nCurMenu].pOptions[iOption];
    for (DWORD iCondition; iCondition < pOption->numConditions; ++iCondition)
    {
        GetOperationDesc(

    }
}

CSMS theSMS;