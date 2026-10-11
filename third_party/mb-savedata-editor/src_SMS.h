#pragma once

#include "ZJCommon.h"
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

class CSMS
{
public:
    CSMS(void);
    ~CSMS(void);

    void LoadMenuData();
    void LoadLanguagesData();

    void UpdateOptionInfo(DWORD iOption);

    CString   m_ModulePath;
    char *m_pMenuData;

    DWORD                      m_numMenus;
    DWORD                      m_nCurMenu;
    stMenu                    *m_pMenus;

    char *m_pLanguageData;

    DWORD m_numLanguageNodes;
    char **m_ppLanguageNodes;
    wchar_t **m_ppLanguageStrings;

    CString m_strOptionCondition;
    CString m_strOptionConsequence;
};

extern CSMS theSMS;