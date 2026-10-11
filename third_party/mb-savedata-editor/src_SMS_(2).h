#pragma once

#include "MainFrm.h"
#include "ZJReader.h"
#include "GrowableArray.h"

#define SMS_MENUDATA "menus"

struct stOperation
{
    int id;
    int numArg;
    LONGLONG *pArgs;

    stOperation()  { pArgs = NULL; }
    ~stOperation() { SAFE_DELETE_ARRAY(pArgs); }
};

struct stOption
{
    char *Name;
    
    UINT numConditions;
    stOperation *pConditions;

    char *text;

    UINT numConsequences;
    stOperation *pConsequences;

    stOption()  { Name = NULL; text = NULL; pConditions = NULL; pConsequences = NULL;}
    ~stOption() { SAFE_DELETE_ARRAY(Name); SAFE_DELETE_ARRAY(text); SAFE_DELETE_ARRAY(pConditions); SAFE_DELETE_ARRAY(pConsequences);}
};

struct stMenu
{
    char *Name;
    ULONGLONG flag;
    char *text;

    UINT numOperations;
    stOperation *pOperations;

    UINT numOptions;
    stOption *pOptions;

    stMenu()  { Name = NULL; text = NULL; pOperations = NULL; pOptions = NULL; }
    ~stMenu() { SAFE_DELETE_ARRAY(Name); SAFE_DELETE_ARRAY(text); SAFE_DELETE_ARRAY(pOperations); SAFE_DELETE_ARRAY(pOptions); }
};



// Modules
namespace SMS
{
    struct stItemMeshInfo{
        stItemMeshInfo() { meshName = NULL; }
        ~stItemMeshInfo(){ SAFE_DELETE_ARRAY(meshName); }
        char *meshName;
        int  meshModifyBit;
    };
    class CItemKind
    {
    public:
        CItemKind()
        {
            Name = NULL;
            text = NULL;
            factions = NULL;
            meshes = NULL;
        }
        ~CItemKind()
        {
            SAFE_DELETE_ARRAY(Name);
            SAFE_DELETE_ARRAY(text);
            SAFE_DELETE_ARRAY(factions);
            SAFE_DELETE_ARRAY(meshes);
        }
        UINT m_id;
        char *Name;
        char *text;

        UINT numMeshes;
        stItemMeshInfo *meshes;

        UINT iMesh;

        ULONGLONG ullProperties;
        ULONGLONG ullCapabilities;
        ULONGLONG ullCost;

        ULONGLONG ullModifyBit;

        float  weight;
        UINT   abundance;
        UINT   headArmor;
        UINT   bodyArmor;
        UINT   legArmor;
        UINT   difficulty;//ϡ�ж�
        UINT   hitPoints;
        UINT   speedRating;
        UINT   missileSpeed;
        UINT   weaponLength;
        UINT   maxAmmo;
        UINT   thrustDamage;
        UINT   swingDamage;

        UINT numFactions;
        int *factions;//������ϵ

        UINT numTriggers;
    };

    class CTroop
    {
    public:
        CTroop()
        {
            Name = NULL;
            text = NULL;
        }
        ~CTroop()
        {
            SAFE_DELETE_ARRAY(Name);
        }

        char *Name;
        wchar_t *text; // same address with LanguageNode
        ULONGLONG ullFlag;
        ULONGLONG scene;
        UINT faction;
        int  items[64];
        UINT strength, agility, intelligence, charisma;
        UINT startingLevel;
        UINT weaponProficiencies[7];
        UINT skillWords[6];
        ULONGLONG facekey1, facekey2;
    };

    class CNPC
    {
    public:
        char facekey[23];
        wchar_t *text;// same address with troop
    };
}


class CLanguageNode
{
public:
    CLanguageNode()
    {
        name = NULL;
        text = NULL;
    }
    ~CLanguageNode()
    {
        SAFE_DELETE_ARRAY(name);
        SAFE_DELETE_ARRAY(text);
    }
    char *name;
    wchar_t *text;
};

class CTextureNode
{
public:
    CTextureNode() : texture(NULL) {}
    ~CTextureNode() { SAFE_RELEASE(texture); }
    char name[MAX_PATH];
    LPDIRECT3DTEXTURE9 texture;
};

class CMeshNode
{
public:
    char name[MAX_PATH];
    //CMyMeshLoader mesh;
};
// ��ƽ����ά���ɣ������Ժ��...


class CSMS
{
public:
    CSMS(void);
    ~CSMS(void);

    void LoadMenuData();

    void UpdateOptionInfo(DWORD iOption);

    CString   m_ModulePath;
    char *m_pMenuData;

    DWORD                      m_numMenus;
    DWORD                      m_nCurMenu;
    stMenu                    *m_pMenus;

    CString m_strOptionCondition;
    CString m_strOptionConsequence;
    
    void                    LoadItemKinds();
    char                   *m_strItemKinds;
    SMS::CItemKind         *m_ItemKinds;
    UINT                    m_numItemKinds;

    CMeshNode              *m_Meshes;
    UINT                    m_numMeshes;
    CTextureNode           *m_Textures;
    UINT                    m_numTextures;


    void                    LoadTroops();
    char                   *m_strTroops;
    SMS::CTroop            *m_Troops;
    UINT                    m_numTroops;

    LPSTR                   GetFaceKeyFlag(ULONGLONG ullFaceKey1, ULONGLONG ullFaceKey2);
    CZJArray <SMS::CNPC>    m_NPCs;


    // LanguageData
    void                    LoadLanguageData(CLanguageNode** ppLng, UINT& num, char** pStrFileData, LPCWSTR strPath);
    bool                    GetLanguageData(CLanguageNode* pLng, UINT& num, LPSTR name, wchar_t **pText);

    char                   *m_strLngTroops;
    CLanguageNode          *m_lngTroops;
    UINT                    m_numLngTroops;
};

extern CSMS theSMS;