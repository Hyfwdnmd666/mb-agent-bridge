
#pragma once

#include "main.h"

#include "ZBL/Wnd.h"
#include "ZBL/Input/Input.h"
#include "ZBL/GUI/GUI.h"
#include "ZBL/Cursor.h"


#include "GameData.h"

//#include "IniSet.h"

//#include "World.h"
//#include "ModData.h"
//#include "GameData.h"

#undef CreateWindow


// Textures
class cTextureRecord
{
public:
    cTextureRecord()  { name = NULL; id = 0; }
    ~cTextureRecord() { SAFE_DELETE_ARRAY(name); }

    char *name;
    uint id;
};
class cTexturesManager : public cPointersArray<cTextureRecord*>
{
public:
    uint Get(const char *name)
    {
        const char **pps = &name;
        cTextureRecord** pptr = BSearch(&pps, PAFD_strcmp);
        if (!pptr)
            return 0;
        return (*pptr)->id;
    }

    void LoadCommon(const char *name, const char *filename)
    {
        cTextureRecord *ptr = new cTextureRecord;
        ptr->name = new char[strlen(name) + 1];
        strcpy(ptr->name, name);
        m_pRenderer->CreateTexture(m_strCommonPath + filename, &ptr->id);
        Add(ptr);
    }

    void Load(const char *name, const char *filename)
    {
        cTextureRecord *ptr = new cTextureRecord;
        ptr->name = new char[strlen(name) + 1];
        strcpy(ptr->name, name);
        m_pRenderer->CreateTexture(filename, &ptr->id);
        Add(ptr);
    }
    void Load(const char *name, HMODULE hModule, const char *pSrcResource)
    {
        cTextureRecord *ptr = new cTextureRecord;
        ptr->name = new char[strlen(name) + 1];
        strcpy(ptr->name, name);
        m_pRenderer->CreateTexture(hModule, pSrcResource, &ptr->id);
        Add(ptr);
    }

public:
    IRenderer            *m_pRenderer;
    cString               m_strCommonPath;
};


class cMainWnd : public cWnd
{
public:
                            cMainWnd();
                            ~cMainWnd();

public:
    int                     CreateWindow();
    bool                    InitializeDevice();
    bool                    InitializeRes();
    bool                    SetupGeometry();
    bool                    SetupGUI();
    int                     MainLoop();

    void                    ProcessInput();


    LPDIRECT3DDEVICE9       m_pDevice;
    LPDIRECT3D9             m_pD3D;

    cInput                  m_Input;

    //cIniSet                 m_iniConfig;

    //bool                    m_bFullScreen;

    IRenderer              *m_pRenderer;
    cAnimation              m_aniCursor;
    cCursor                 m_Cursor;
    cGUI                    m_GUI;

    UINT                    IDV_TOPVIEW,
                            IDV_MODULELIST,
                            IDV_MAINMENU;

    UINT                    IDF_K18;

    cTexturesManager        m_Textures;

    UINT                    IDE_SCROLLBAR,
                            IDE_SCROLLBAR_THUMB,
                            IDE_COMBOBOX_ITEM,
                            IDE_COMBOBOX,
                            IDE_STATIC1,
                            IDE_WEIBOB,
                            IDE_WEIBO;

    cButton                 *m_pButton;

    // Mod
    //cModData                m_ModData;
    bool                    InitModData();

    // Game
    //cWorld                  m_World;
    cGameData               m_GameData;
    //void                    StartGame();

    bool                    GetWeiboPic(const char *strSavePath);
};