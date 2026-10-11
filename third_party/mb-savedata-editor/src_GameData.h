#pragma once

#define MAX_PATH 260

#include "ZBL/String.h"
#include "ZBL/PointersArray.h"

class cModuleInfo
{
public:
    cString m_strName;
    cString m_strPath;
};

class cGameData
{
public:
    cGameData();
    ~cGameData();

public:
    bool                            Initialize();

    bool                            GetGamePath();
    bool                            GetModules();

public:
    //char                        m_strPath[MAX_PATH + 1];
    cString                         m_strPath;
    char                            m_strVersion[8];

public:
    cPointersArray<cModuleInfo*>    m_Modules;
};