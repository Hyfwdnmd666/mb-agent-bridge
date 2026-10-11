
#include "MainWnd.h"
#include "idc.h"

void __stdcall GUICallback(UINT Event, int id, cControl *pCtrl)
{
    if (Event == EVENT_BUTTON_PRESSED || Event == EVENT_LISTBOX_ITEM_PRESSED)
    {
        //PlaySound(
        //PlaySound(MAKEINTRESOURCE(IDR_WAVE1), NULL, SND_RESOURCE | SND_ASYNC);
    }

    
    switch (id)
    {
    case IDC_ModuleList_Enter:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            //theCore.LoadModule(m_pModulesComboBox->GetSelectedIndex());
            return;
        }
        break;
    case IDC_Quit:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            PostQuitMessage(0);
            return;
        }
        break;
    case IDC_Weibo:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            ShellExecute(NULL, "open", "http://weibo.com/zhizunmingshuai", NULL, NULL, SW_SHOWNORMAL);
            return;
        }
        break;
    /*
    case IDC_LOADSAVEFILE:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            theWSE.OnFileOpen();
            return;
        }
        break;
    case IDC_MODIFY_SAVEDATA:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            m_GUI.TurnView(IDV_MODIFY_SAVEDATA_MENU);
            return;
        }
        break;
    case IDC_MODIFY_CHARACTER:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            m_GUI.TurnView(IDV_MODIFY_CHARACTER);
            return;
        }
        break;
    case IDC_BACKTOMODIFYMENU:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            Update(true);
            m_GUI.TurnView(IDV_MODIFY_SAVEDATA_MENU);
            return;
        }
        break;
    case IDC_BACKTOMAINMENU:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            m_GUI.TurnView(IDV_MAINMENU);
            return;
        }
        break;

    case IDC_NPCLIST:
        if (Event == EVENT_LISTBOX_SELECTION)
        {
            Update(true);
            if (theWSE.m_pNPCList->GetSelectedIndex() == 0)
                theWSE.pCurTroop = &theWSE.trpPlayer;
            else
                theWSE.pCurTroop = &theWSE.trpNpc[theWSE.m_pNPCList->GetSelectedIndex() - 1];
            Update(false);
        }
        break;
    */
    }
}

bool cMainWnd::SetupGUI()
{
    m_GUI.Create(m_pRenderer);

    cGUIView   *pView;
    cGUIElement e;
    uint        id;
    
    RECT rc, rcNormal, rcOver, rcDown;
    int x, y, w, h, xd, yd;

    // TopView
    {
        m_GUI.AddView(0, 0, 800, 600, &IDV_TOPVIEW, 0, NULL, &pView);
        e.Clear();
        e.SetTexture(ZJGUI_STATE_NORMAL, m_Textures.Get("logo1"));
        e.SetTexture(ZJGUI_STATE_MOUSEOVER, m_Textures.Get("logo2"));
        e.SetTexture(ZJGUI_STATE_PRESSED, m_Textures.Get("logo3"));
        m_GUI.CreateElement(&e, &id);

        pView->AddButton(IDC_Logo, NULL, 695, 15, 88, 33, id);

        pView->SetCallBack(GUICallback);

        m_GUI.SetView(IDV_TOPVIEW, false, 0.3f);
        m_GUI.SetTopView(IDV_TOPVIEW, true);
    }
    // end of topview

    // PreMenu
    {
        RECT rcPadding, rcTrack;
        cScrollBar *pScrollBar;
        cListBox   *pListBox;

        m_GUI.AddView(0, 0, 800, 600, &IDV_MODULELIST, m_Textures.Get("mp_ui_host"), NULL, &pView);

        e.Clear();
        SetRect(&rc, 986, 14, 1006, 687);
        e.SetTexture(ZJGUI_STATE_NORMAL, m_Textures.Get("interface"), &rc);
        m_GUI.CreateElement(&e, &IDE_SCROLLBAR);

        e.Clear();
        SetRect(&rc, 954, 120, 966, 441);
        e.SetTexture(ZJGUI_STATE_NORMAL, m_Textures.Get("interface"), &rc);
        m_GUI.CreateElement(&e, &IDE_SCROLLBAR_THUMB);

        SetRect(&rcPadding, 0, 0, 0, 0);
        SetRect(&rcTrack, 515, 232, 515 + 15, 232 + 176);
        pView->AddScrollBar(0, 510, 230, 25, 180, &rcTrack, IDE_SCROLLBAR, IDE_SCROLLBAR_THUMB, &pScrollBar);

        e.Clear();
        SetRect(&rc, 148, 195, 510, 250);
        e.SetTexture(ZJGUI_STATE_NORMAL, m_Textures.Get("button_drop"), &rc);
        e.SetTexture(ZJGUI_STATE_SELECTED, m_Textures.Get("button_drop"), &rc);
        SetRect(&rc, 148, 130, 510, 185);
        e.SetTexture(ZJGUI_STATE_MOUSEOVER, m_Textures.Get("button_drop"), &rc);
        SetRect(&rc, 148, 260, 510, 315);
        e.SetTexture(ZJGUI_STATE_PRESSED, m_Textures.Get("button_drop"), &rc);

        e.SetFont(IDF_K18);
        e.SetFontColor(ZJGUI_STATE_NORMAL);
        e.SetFontColor(ZJGUI_STATE_SELECTED);
        e.SetFontColor(ZJGUI_STATE_MOUSEOVER);
        e.SetFontColor(ZJGUI_STATE_PRESSED);
        m_GUI.CreateElement(&e, &IDE_COMBOBOX_ITEM);

        SetRect(&rcPadding, 0, 0, 0, 0);
        pView->AddListBox(0, 270, 230, 240, 180, &rcPadding, 0, 240, 30, 1, IDE_COMBOBOX_ITEM, pScrollBar, &pListBox);

        int cnt = m_GameData.m_Modules.GetSize();
        for (int i = 0; i < cnt; ++i)
        {
            pListBox->AddItem(m_GameData.m_Modules[i]->m_strName);
        }
        pListBox->UpdateScrollBar();

        e.Clear();
        SetRect(&rc, 148, 390, 510, 445);
        e.SetTexture(ZJGUI_STATE_NORMAL, m_Textures.Get("button_drop"), &rc);
        e.SetTexture(ZJGUI_STATE_SELECTED, m_Textures.Get("button_drop"), &rc);
        SetRect(&rc, 148, 325, 510, 380);
        e.SetTexture(ZJGUI_STATE_MOUSEOVER, m_Textures.Get("button_drop"), &rc);
        SetRect(&rc, 148, 455, 510, 510);
        e.SetTexture(ZJGUI_STATE_PRESSED, m_Textures.Get("button_drop"), &rc);
        
        e.SetFont(IDF_K18);
        e.SetFontColor(ZJGUI_STATE_NORMAL);
        e.SetFontColor(ZJGUI_STATE_SELECTED);
        e.SetFontColor(ZJGUI_STATE_MOUSEOVER);
        e.SetFontColor(ZJGUI_STATE_PRESSED);
        m_GUI.CreateElement(&e, &IDE_COMBOBOX);

        pView->AddComboBox(0, 270, 200, 254, 30, IDE_COMBOBOX, pListBox, 0);

        e.Clear();
        e.SetFont(IDF_K18, DT_NOCLIP);
        e.SetFontColor(ZJGUI_STATE_NORMAL);
        m_GUI.CreateElement(&e, &IDE_STATIC1);

        pView->AddStaticText(0, "ѡ��籾��", 230, 170, 0, 0, IDE_STATIC1);

        m_GUI.CreateButtonElement(m_Textures.Get("premenu_button1"), 2, &id);
        pView->AddButton(IDC_ModuleList_Enter, NULL, 335, 245, 125, 45, id);

        m_GUI.CreateButtonElement(m_Textures.Get("premenu_button2"), 2, &id);
        pView->AddButton(IDC_ModuleList_Config, NULL, 335, 290, 125, 45, id);

        m_GUI.CreateButtonElement(m_Textures.Get("premenu_button3"), 2, &id);
        pView->AddButton(IDC_Quit, NULL, 335, 335, 125, 45, id);

        m_GUI.CreateButtonElement(m_Textures.Get("weibob"), 2, &IDE_WEIBOB);
        pView->AddButton(IDC_Weibo, NULL, 382, 445, 412, 140, IDE_WEIBOB);

        e.Clear();
        e.SetTexture(ZJGUI_STATE_NORMAL, m_Textures.Get("weibo"));
        m_GUI.CreateElement(&e, &IDE_WEIBO);

        pView->AddPicture(0, 392, 455, 392, 120, IDE_WEIBO);

        pView->SetCallBack(GUICallback);

        m_GUI.SetView(IDV_MODULELIST, true);
    }

    return true;
}