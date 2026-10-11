
// MainFrm.h : CMainFrame ��Ľӿ�
//

#pragma once
#include "ChildView.h"
#include "..\..\CZJGUI\CZJGUI\ZJGUI.h"
#include "VertexStruct.h"
#include "ZJGUIDataExchange.h"

class CMainFrame : public CFrameWndEx
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// ����
public:

// ����
public:

// ��д
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

    virtual LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam);

// ʵ��
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // �ؼ���Ƕ���Ա
	CMFCMenuBar       m_wndMenuBar;
	//CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CChildView    m_wndView;

// ���ɵ���Ϣӳ�亯��
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()

public:
    void InitDevice(void);
    void InitGeometry(void);
    void InitFonts(void);
    void InitTextures(void);
    void InitGUI(void);
    void Render(void);
    void Shutdown(void);

    void Update(bool bSaveAndValidate);

    LPDIRECT3D9 GetD3D()
    {
        return m_pd3d;
    }
    LPDIRECT3DDEVICE9 GetDevice()
    {
        return m_pd3dDevice;
    }

    CZJManager                  m_Manager;
    CZJGUI                      m_GUI;
protected:
    LPDIRECT3D9                 m_pd3d;
    LPDIRECT3DDEVICE9           m_pd3dDevice;
    

protected:
    ID3DXMesh *m_pTitleMesh;
    D3DXMATRIX m_matTitlePre;
    D3DMATERIAL9 m_mtrlTitle;

    CZJGUIButton               *m_WSEOptions[3];
    void                        CheckWSEStatus();

public:
    UINT                        IDT_MP_UI_HOST,
                                IDT_BUTTON_DROP,
                                IDT_INTERFACE,
                                IDT_MAINMENU,
                                IDT_PREMENU_BUTTON1,
                                IDT_PREMENU_BUTTON2,
                                IDT_PREMENU_BUTTON3,
                                IDT_LOGO1,
                                IDT_LOGO2,
                                IDT_LOGO3,
                                IDT_MP_UI_BG,
                                IDT_CHARACTER_WINDOW,
                                IDT_BG2,
                                IDT_INVENTORY_WINDOW,
                                IDT_LOADSCREEN;

    UINT                        IDV_MAIN,
                                IDV_MAINMENU,
                                IDV_TOPVIEW,
                                //IDV_CHARACTER,
                                IDV_MODIFY_SAVEDATA_MENU,
                                IDV_MODIFY_CHARACTER,
                                IDV_MODIFY_ITEM,
                                IDV_LOADSCREEN;

    UINT                        IDE_COMBOBOX_ITEM,
                                IDE_SCROLLBAR,
                                IDE_SCROLLBAR_THUMB,
                                IDE_COMBOBOX,
                                IDE_STATIC1,
                                IDE_STATIC2,
                                IDE_STATIC3,
                                IDE_STATIC4,
                                IDE_STATIC5,
                                IDE_PREMENU_BUTTON1,
                                IDE_PREMENU_BUTTON2,
                                IDE_PREMENU_BUTTON3,
                                IDE_MAINMENU_BUTTON,
                                IDE_LOGO,
                                IDE_GAMEMENU_ITEM,
                                IDE_ATTRIBUTES,
                                IDE_SKILLS,
                                IDE_EDITBOX1,
                                IDE_EDITBOX2,
                                IDE_BUTTON1,
                                IDE_CHECKBOX1,
                                IDE_CHECKBOX1_CHECKED,
                                IDE_NPCLIST_ITEM,
                                IDE_PROGRESSBAR,
                                IDE_PROGRESSHANDLE,
                                IDE_LOADSCREENTEXT;

    UINT                        IDF_K18,
                                IDF_K20,
                                IDF_K24,
                                IDF_K17,
                                IDF_K15,
                                IDF_K14,
                                IDF_S18;
    void GUICallBackMainView(UINT Event, int ID, CZJGUIControl* pCtrl);
    
    // Control Pointers
public:
    CZJGUIComboBox               *m_pModulesComboBox;

    // Extra Render
public:
    void RenderTitle();
    void PreRenderLoadScreen();
};


