#include "StdAfx.h"
#include "SMS.h"
#include "MainCore.h"
#include "ZJ_Algorithm.h"

using namespace SMS;

CSMS::CSMS(void)
    : m_pMenuData(NULL)
{
    m_strItemKinds = NULL;
    m_ItemKinds = NULL;

    m_strTroops = NULL;
    m_Troops = NULL;

    InitAlgorithm();

    m_strLngTroops = NULL;
    m_lngTroops = NULL;

    m_Meshes = NULL;
    m_Textures = NULL;
}


CSMS::~CSMS(void)
{
    SAFE_DELETE_ARRAY(m_pMenuData);
    SAFE_DELETE_ARRAY(m_pMenus);

    SAFE_DELETE_ARRAY(m_strItemKinds);
    SAFE_DELETE_ARRAY(m_ItemKinds);

    SAFE_DELETE_ARRAY(m_strTroops);
    SAFE_DELETE_ARRAY(m_Troops);


    SAFE_DELETE_ARRAY(m_strLngTroops);
    SAFE_DELETE_ARRAY(m_lngTroops);

    SAFE_DELETE_ARRAY(m_Meshes);
    SAFE_DELETE_ARRAY(m_Textures);
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

int _cdecl vLngCmp(const void *a, const void *b)
{
    return strcmp(((CLanguageNode*)a)->name, ((CLanguageNode*)b)->name);
}

void CSMS::LoadLanguageData(CLanguageNode** ppLng, UINT& num, char** pStrFileData, LPCWSTR strPath)
{
    FILE *fp;
    _wfopen_s(&fp, strPath, L"rb");
    fseek(fp, 0L, SEEK_END);
    
    int nFileLength = ftell(fp);
    SAFE_DELETE_ARRAY(*pStrFileData);
    *pStrFileData = new char[nFileLength + 1];

    fseek(fp, 0L, SEEK_SET);
    fread(*pStrFileData, nFileLength + 1, 1, fp);
    fclose(fp);


    // ���ļ����ݶ��뵽CZJReader ��������
    CZJReader reader;
    reader.Reader(*pStrFileData);
    
    num = reader.CountAll('|');
    ASSERT(num != 0);
    SAFE_DELETE_ARRAY(*ppLng);
    *ppLng = new CLanguageNode[num];
    char *str;
    int len;
    //ATL::CStringT< char, StrTraitMFC_DLL< char > > sz;
    CString sz;
    wchar_t *wstr;
    CLanguageNode *pLng = *ppLng;
    for (DWORD iNode = 0; iNode < num; ++iNode)
    {
        pLng[iNode].name = reader.GetsUntil('|');
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

        pLng[iNode].text = new wchar_t[len + 1];
        wcscpy_s(pLng[iNode].text, len + 1, sz.GetBuffer());

        reader.Ignore();
    }
    qsort(pLng, num, sizeof(CLanguageNode), vLngCmp);
}

bool CSMS::GetLanguageData(CLanguageNode* pLng, UINT& num, LPSTR name, wchar_t **pText)
{
    CLanguageNode t;
    t.name = name;
    CLanguageNode *pt = (CLanguageNode*)bsearch(&t, pLng, num, sizeof(CLanguageNode), vLngCmp);
    t.name = NULL; // forbidden the ~() delete the 'name'
    if (!pt)
        return false;
    *pText = pt->text;
    return true;
}

//const wchar_t strOperations[][32] =
//{
    
DWORD GetOperationDesc(CString* pStr, stOperation& operation, DWORD depth)
{
    return 0;
}

void CSMS::UpdateOptionInfo(DWORD iOption)
{
    m_strOptionCondition.Empty();
    stOption *pOption = &m_pMenus[m_nCurMenu].pOptions[iOption];
    for (DWORD iCondition = 0; iCondition < pOption->numConditions; ++iCondition)
    {
        //GetOperationDesc(

    }
}


void CSMS::LoadItemKinds()
{
    FILE *fp;
    int len = wcslen(theCore.m_ModulePath);
    LPCWSTR src = L"item_kinds1.txt";
    len += wcslen(src);

    LPWSTR path = new WCHAR[len + 1];
    wcscpy_s(path, len + 1, theCore.m_ModulePath);
    
    wcscat_s(path, len + 1, src);
    _wfopen_s(&fp, path, L"rb");
    SAFE_DELETE_ARRAY(path);

    fseek(fp, 0L, SEEK_END);
    int nFileLength = ftell(fp);
    SAFE_DELETE_ARRAY(m_strItemKinds);
    m_strItemKinds = new char[nFileLength + 1];

    fseek(fp, 0L, SEEK_SET);
    fread(m_strItemKinds, nFileLength + 1, 1, fp);
    fclose(fp);

    // ���ļ����ݶ��뵽CZJReader ����������
    CZJReader reader;
    reader.Reader(m_strItemKinds);

    SAFE_DELETE_ARRAY(m_ItemKinds);

    reader.GotoNextLine();
    m_numItemKinds = reader.GetUINT();

    m_ItemKinds = new CItemKind[m_numItemKinds];

    SAFE_DELETE_ARRAY(m_Meshes);
    SAFE_DELETE_ARRAY(m_Textures);

    m_Meshes = new CMeshNode[m_numItemKinds];
    m_Textures = new CTextureNode[m_numTextures];
    m_numTextures = 0;

    m_numMeshes = 1;// 0 is Empty;

    for (DWORD iItem = 0; iItem < m_numItemKinds; ++iItem)
    {
        m_ItemKinds[iItem].m_id = iItem;
        m_ItemKinds[iItem].Name = reader.GetLPSTR();
#if 0
        m_ItemKinds[iItem].text = reader.GetLPSTR();
#else
        reader.IgnoreLPSTR();
#endif
        reader.IgnoreLPSTR();
        m_ItemKinds[iItem].numMeshes = reader.GetUINT();
        SAFE_DELETE_ARRAY(m_ItemKinds[iItem].meshes);
        m_ItemKinds[iItem].meshes = new stItemMeshInfo[m_ItemKinds[iItem].numMeshes];

        if (m_ItemKinds[iItem].numMeshes > 0)
        {
            UINT iMesh;
            for (iMesh = 0; iMesh < m_ItemKinds[iItem].numMeshes; ++iMesh)
            {
                m_ItemKinds[iItem].meshes[iMesh].meshName = reader.GetLPSTR();
                m_ItemKinds[iItem].meshes[iMesh].meshModifyBit = reader.GetUINT();
            }

            for (iMesh = 0; iMesh < m_numMeshes; ++iMesh)
            {
                if (strcmp(m_ItemKinds[iItem].meshes[0].meshName, m_Meshes[iMesh].name) == 0)
                {
                    m_ItemKinds[iItem].iMesh = iMesh;
                    break;
                }
            }
            if (iMesh >= m_numMeshes)
            {
                if (strcmp(m_ItemKinds[iItem].meshes[0].meshName, "invalid_item") == 0)
                    m_ItemKinds[iItem].iMesh = 0;
                else
                {
                    m_ItemKinds[iItem].iMesh = iMesh = m_numMeshes++;
                    strcpy_s(m_Meshes[iMesh].name, m_ItemKinds[iItem].meshes[0].meshName);
                    //m_Meshes[iMesh].mesh.Create(theCore.m_pMainFrm->GetDevice(), m_Meshes[iMesh].name);
                }
            }
        }

        m_ItemKinds[iItem].ullProperties = reader.GetULONGLONG();
        m_ItemKinds[iItem].ullCapabilities = reader.GetULONGLONG();
        m_ItemKinds[iItem].ullCost = reader.GetULONGLONG();
        m_ItemKinds[iItem].ullModifyBit = reader.GetULONGLONG();

        m_ItemKinds[iItem].weight = reader.GetFloat();
        m_ItemKinds[iItem].abundance = reader.GetUINT();
        m_ItemKinds[iItem].headArmor = reader.GetUINT();
        m_ItemKinds[iItem].bodyArmor = reader.GetUINT();
        m_ItemKinds[iItem].legArmor = reader.GetUINT();
        m_ItemKinds[iItem].difficulty = reader.GetUINT();
        m_ItemKinds[iItem].hitPoints = reader.GetUINT();
        m_ItemKinds[iItem].speedRating = reader.GetUINT();
        m_ItemKinds[iItem].missileSpeed = reader.GetUINT();
        m_ItemKinds[iItem].weaponLength = reader.GetUINT();
        m_ItemKinds[iItem].maxAmmo = reader.GetUINT();
        m_ItemKinds[iItem].thrustDamage = reader.GetUINT();
        m_ItemKinds[iItem].swingDamage = reader.GetUINT();

        reader.GotoNextLine();
        m_ItemKinds[iItem].numFactions = reader.GetUINT();
        SAFE_DELETE_ARRAY(m_ItemKinds[iItem].factions);
        m_ItemKinds[iItem].factions = new int[m_ItemKinds[iItem].numFactions];
        if (m_ItemKinds[iItem].numFactions > 0)
        {
            reader.GotoNextLine();
        }
        for (DWORD iFaction = 0; iFaction < m_ItemKinds[iItem].numFactions; ++iFaction)
        {
            m_ItemKinds[iItem].factions[iFaction] = (int)reader.GetUINT();
        }

        reader.GotoNextLine();
        m_ItemKinds[iItem].numTriggers = reader.GetUINT();
        for (DWORD iTrigger = 0; iTrigger < m_ItemKinds[iItem].numTriggers; ++iTrigger)
        {
            reader.GotoNextLine();
        }
        reader.GotoNextLine();
    }
}



void CSMS::LoadTroops()
{
    FILE *fp;

#if 1

    wchar_t path[MAX_PATH];
    swprintf_s(path, L"%s%s", theCore.m_ModulePath, L"troops.txt");
    _wfopen_s(&fp, path, L"rb");

#else

    size_t len = wcslen(theCore.m_ModulePath);
    LPCWSTR src = L"troops.txt";
    len += wcslen(src);

    LPWSTR path = new WCHAR[len + 1];
    wcscpy_s(path, len + 1, theCore.m_ModulePath);
    
    wcscat_s(path, len + 1, src);
    _wfopen_s(&fp, path, L"rb");
    SAFE_DELETE_ARRAY(path);

#endif

    fseek(fp, 0L, SEEK_END);
    int nFileLength = ftell(fp);
    SAFE_DELETE_ARRAY(m_strTroops);
    m_strTroops = new char[nFileLength + 1];

    fseek(fp, 0L, SEEK_SET);
    fread(m_strTroops, nFileLength + 1, 1, fp);
    fclose(fp);
    
    
    swprintf_s(path, L"%slanguages\\%s\\%s", theCore.m_ModulePath, L"cns", L"troops.csv");
    LoadLanguageData(&m_lngTroops, m_numLngTroops, &m_strLngTroops, path);
        

    // ���ļ����ݶ��뵽CZJReader ����������
    CZJReader reader;
    reader.Reader(m_strTroops);

    SAFE_DELETE_ARRAY(m_Troops);

    reader.GotoNextLine();
    m_numTroops = reader.GetUINT();

    m_Troops = new CTroop[m_numTroops];

    for (DWORD iTroop = 0; iTroop < m_numTroops; ++iTroop)
    {
        m_Troops[iTroop].Name = reader.GetLPSTR();
#if 0
        m_ItemKinds[iItem].text = reader.GetLPSTR();
#else
        if (GetLanguageData(m_lngTroops, m_numLngTroops, m_Troops[iTroop].Name, &m_Troops[iTroop].text))
            reader.IgnoreLPSTR();
        else
            reader.IgnoreLPSTR(); // just for now
#endif
        reader.IgnoreLPSTR();

        reader.IgnoreLPSTR();
        m_Troops[iTroop].ullFlag = reader.GetULONGLONG();
        m_Troops[iTroop].scene = reader.GetULONGLONG();
        reader.IgnoreLPSTR();
        m_Troops[iTroop].faction = reader.GetUINT();
        reader.IgnoreLPSTR();
        reader.IgnoreLPSTR();

        reader.GotoNextLine();

        for (int iItem = 0; iItem < 64; ++iItem)
        {
            m_Troops[iTroop].items[iItem] = reader.GetInt();
            reader.IgnoreLPSTR();
        }

        reader.GotoNextLine();
        
        //for (int iAttrib = 0; iAttrib
        m_Troops[iTroop].strength = reader.GetUINT();
        m_Troops[iTroop].agility = reader.GetUINT();
        m_Troops[iTroop].intelligence = reader.GetUINT();
        m_Troops[iTroop].charisma = reader.GetUINT();
        m_Troops[iTroop].startingLevel = reader.GetUINT();
        
        reader.GotoNextLine();

        for (int iWP = 0; iWP < 7; ++iWP)
        {
            m_Troops[iTroop].weaponProficiencies[iWP] = reader.GetUINT();
        }

        reader.GotoNextLine();

        for (int iSW = 0; iSW < 6; ++iSW)
        {
            m_Troops[iTroop].skillWords[iSW] = reader.GetUINT();
        }

        reader.GotoNextLine();

        reader.IgnoreLPSTR();
        m_Troops[iTroop].facekey1 = reader.GetULONGLONG();
        m_Troops[iTroop].facekey2 = reader.GetULONGLONG();

        reader.GotoNextLine();
    }
    

    LPCSTR npcPre = "trp_npc";
    size_t len = strlen(npcPre);
    char *str;
    int i;
    for (DWORD iTroop = 0; iTroop < m_numTroops; ++iTroop)
    {
        if (strlen(m_Troops[iTroop].Name) < len)
            continue;
        for (i = 0; i < len; ++i)
        {
            if (m_Troops[iTroop].Name[i] != npcPre[i])
                break;
        }
        if (i < len)
            continue;
        i = m_NPCs.GetSize();
        m_NPCs.Increase();
        str = GetFaceKeyFlag(m_Troops[iTroop].facekey1, m_Troops[iTroop].facekey2);
        strcpy_s(m_NPCs[i].facekey, str);
        SAFE_DELETE_ARRAY(str);

        m_NPCs[i].text = m_Troops[iTroop].text;
    }

}

LPSTR CSMS::GetFaceKeyFlag(ULONGLONG ullFaceKey1, ULONGLONG ullFaceKey2)
{
    char *str1 = GetHexFromQWORD(ullFaceKey1);
    char *str2 = GetHexFromQWORD(ullFaceKey2);
    
    char *str = new char[23];
    strcpy_s(str, 23, str1);
    str2[6] = '\0';
    strcat_s(str, 23, str2);
    SAFE_DELETE_ARRAY(str1);
    SAFE_DELETE_ARRAY(str2);
    return str;
}

CSMS theSMS;