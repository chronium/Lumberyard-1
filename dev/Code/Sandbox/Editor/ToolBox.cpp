/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : ToolBox Macro System


#include "StdAfx.h"
#include "ToolBox.h"
#include "Util/BoostPythonHelpers.h"
#include "IconManager.h"
#include "ActionManager.h"

//////////////////////////////////////////////////////////////////////////
// CToolBoxCommand
//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Save(XmlNodeRef commandNode) const
{
    commandNode->setAttr("type", (int)m_type);
    commandNode->setAttr("text", m_text.toLatin1().data());
    commandNode->setAttr("bVariableToggle", m_bVariableToggle);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Load(XmlNodeRef commandNode)
{
    int type = 0;
    commandNode->getAttr("type", type);
    m_type = CToolBoxCommand::EType(type);
    m_text = commandNode->getAttr("text");
    commandNode->getAttr("bVariableToggle", m_bVariableToggle);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Execute() const
{
    if (m_type == CToolBoxCommand::eT_SCRIPT_COMMAND)
    {
        PyScript::Execute(m_text.toLatin1().data());
    }
    else if (m_type == CToolBoxCommand::eT_CONSOLE_COMMAND)
    {
        if (m_bVariableToggle)
        {
            // Toggle the variable.
            float val = GetIEditor()->GetConsoleVar(m_text.toLatin1().data());
            bool bOn = val != 0;
            GetIEditor()->SetConsoleVar(m_text.toLatin1().data(), (bOn) ? 0 : 1);
        }
        else
        {
            GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(m_text.toLatin1().data());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CToolBoxMacro
//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Save(XmlNodeRef macroNode) const
{
    for (size_t i = 0; i < m_commands.size(); ++i)
    {
        XmlNodeRef commandNode = macroNode->newChild("command");
        m_commands[i]->Save(commandNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Load(XmlNodeRef macroNode)
{
    for (int i = 0; i < macroNode->getChildCount(); ++i)
    {
        XmlNodeRef commandNode = macroNode->getChild(i);
        m_commands.push_back(new CToolBoxCommand);
        m_commands[i]->Load(commandNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::AddCommand(CToolBoxCommand::EType type, const QString& command, bool bVariableToggle)
{
    CToolBoxCommand* pNewCommand = new CToolBoxCommand;
    pNewCommand->m_type = type;
    pNewCommand->m_text = command;
    pNewCommand->m_bVariableToggle = bVariableToggle;
    m_commands.push_back(pNewCommand);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Clear()
{
    for (size_t i = 0; i < m_commands.size(); ++i)
    {
        delete m_commands[i];
    }

    m_commands.clear();
}

//////////////////////////////////////////////////////////////////////////
const CToolBoxCommand* CToolBoxMacro::GetCommandAt(int index) const
{
    assert(0 <= index && index < m_commands.size());

    return m_commands[index];
}

//////////////////////////////////////////////////////////////////////////
CToolBoxCommand* CToolBoxMacro::GetCommandAt(int index)
{
    assert(0 <= index && index < m_commands.size());

    return m_commands[index];
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::SwapCommand(int index1, int index2)
{
    assert(0 <= index1 && index1 < m_commands.size());
    assert(0 <= index2 && index2 < m_commands.size());
    std::swap(m_commands[index1], m_commands[index2]);
}

void CToolBoxMacro::RemoveCommand(int index)
{
    assert(0 <= index && index < m_commands.size());

    m_commands.erase(m_commands.begin() + index);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Execute() const
{
    for (size_t i = 0; i < m_commands.size(); ++i)
    {
        m_commands[i]->Execute();
    }
}

//////////////////////////////////////////////////////////////////////////
// CToolBoxManager
//////////////////////////////////////////////////////////////////////////
int CToolBoxManager::GetMacroCount(bool bToolbox) const
{
    if (bToolbox)
    {
        return int(m_macros.size());
    }
    return int(m_shelveMacros.size());
}

//////////////////////////////////////////////////////////////////////////
const CToolBoxMacro* CToolBoxManager::GetMacro(int iIndex, bool bToolbox) const
{
    if (bToolbox)
    {
        assert(0 <= iIndex && iIndex < m_macros.size());
        return m_macros[iIndex];
    }
    else
    {
        assert(0 <= iIndex && iIndex < m_shelveMacros.size());
        return m_shelveMacros[iIndex];
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro* CToolBoxManager::GetMacro(int iIndex, bool bToolbox)
{
    if (bToolbox)
    {
        assert(0 <= iIndex && iIndex < m_macros.size());
        return m_macros[iIndex];
    }
    else
    {
        assert(0 <= iIndex && iIndex < m_shelveMacros.size());
        return m_shelveMacros[iIndex];
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
int CToolBoxManager::GetMacroIndex(const QString& title, bool bToolbox) const
{
    if (bToolbox)
    {
        for (size_t i = 0; i < m_macros.size(); ++i)
        {
            if (QString::compare(m_macros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return int(i);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < m_shelveMacros.size(); ++i)
        {
            if (QString::compare(m_shelveMacros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return int(i);
            }
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro* CToolBoxManager::NewMacro(const QString& title, bool bToolbox, int* newIdx)
{
    if (bToolbox)
    {
        const int macroCount = m_macros.size();
        if (macroCount > ID_TOOL_LAST - ID_TOOL_FIRST + 1)
        {
            return NULL;
        }

        for (size_t i = 0; i < macroCount; ++i)
        {
            if (QString::compare(m_macros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return NULL;
            }
        }

        CToolBoxMacro* pNewTool = new CToolBoxMacro(title);
        if (newIdx)
        {
            *newIdx = macroCount;
        }
        m_macros.push_back(pNewTool);
        return pNewTool;
    }
    else
    {
        const int shelveMacroCount = m_shelveMacros.size();
        if (shelveMacroCount > ID_TOOL_SHELVE_LAST - ID_TOOL_SHELVE_FIRST + 1)
        {
            return NULL;
        }

        CToolBoxMacro* pNewTool = new CToolBoxMacro(title);
        if (newIdx)
        {
            *newIdx = shelveMacroCount;
        }
        m_shelveMacros.push_back(pNewTool);
        return pNewTool;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::SetMacroTitle(int index, const QString& title, bool bToolbox)
{
    if (bToolbox)
    {
        assert(0 <= index && index < m_macros.size());
        for (size_t i = 0; i < m_macros.size(); ++i)
        {
            if (i == index)
            {
                continue;
            }

            if (QString::compare(m_macros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return false;
            }
        }

        m_macros[index]->SetTitle(title);
    }
    else
    {
        assert(0 <= index && index < m_shelveMacros.size());
        for (size_t i = 0; i < m_shelveMacros.size(); ++i)
        {
            if (i == index)
            {
                continue;
            }

            if (QString::compare(m_shelveMacros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return false;
            }
        }

        m_shelveMacros[index]->SetTitle(title);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Load(ActionManager* actionManager)
{
    Clear();

    QString path;
    GetSaveFilePath(path);
    Load(path, nullptr, true, nullptr);

    if (actionManager)
    {
        QByteArray array = gSettings.strEditorEnv.toLatin1();
        AZStd::string actualPath = AZStd::string::format("@devroot@/%s", array.constData());
        XmlNodeRef envNode = XmlHelpers::LoadXmlFromFile(actualPath.c_str());
        if (envNode)
        {
            int childrenCount = envNode->getChildCount();
            for (int idx = 0; idx < childrenCount; ++idx)
            {
                XmlNodeRef child = envNode->getChild(idx);
                if (child->haveAttr("scriptPath") && child->haveAttr("shelvesPath"))
                {
                    LoadShelves(child->getAttr("scriptPath"), child->getAttr("shelvesPath"), actionManager);
                }
            }
        }
    }

    UpdateShortcutsAndIcons();
}

void CToolBoxManager::LoadShelves(QString scriptPath, QString shelvesPath, ActionManager* actionManager)
{
    GetIEditor()->ExecuteCommand("general.run_file_parameters 'addToSysPath.py' '%s'", scriptPath.toLatin1().data());
    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(shelvesPath, "*.xml", files);

    const int shelfCount = files.size();
    for (int idx = 0; idx < shelfCount; ++idx)
    {
        if (Path::GetExt(files[idx].filename) != "xml")
        {
            continue;
        }

        QString shelfName(PathUtil::GetFileName(files[idx].filename.toLatin1().data()));

        AmazonToolbar toolbar(shelfName, shelfName);
        Load(shelvesPath + QString("/") + files[idx].filename, &toolbar, false, actionManager);

        m_toolbars.push_back(toolbar);
    }
}

void CToolBoxManager::Load(QString xmlpath, AmazonToolbar* pToolbar, bool bToolbox, ActionManager* actionManager)
{
    XmlNodeRef toolBoxNode = XmlHelpers::LoadXmlFromFile(xmlpath.toLatin1().data());
    if (toolBoxNode == NULL)
    {
        return;
    }

    if (!pToolbar)
    {
        GetIEditor()->GetSettingsManager()->AddSettingsNode(toolBoxNode);
    }
    else
    {
        const char* PRETTY_NAME_ATTR = "prettyName";
        const char* SHOW_BY_DEFAULT_ATTR = "showByDefault";
        const char* SHELF_NAME_ATTR = "shelfName";

        QString shelfName = pToolbar->GetName();

        if (toolBoxNode->haveAttr(SHELF_NAME_ATTR))
        {
            shelfName = toolBoxNode->getAttr(SHELF_NAME_ATTR);
        }

        QString prettyName = shelfName;

        if (toolBoxNode->haveAttr(PRETTY_NAME_ATTR))
        {
            prettyName = toolBoxNode->getAttr(PRETTY_NAME_ATTR);
        }

        pToolbar->SetName(shelfName, prettyName);

        if (toolBoxNode->haveAttr(SHOW_BY_DEFAULT_ATTR))
        {
            const char* showByDefaultAttrString = toolBoxNode->getAttr(SHOW_BY_DEFAULT_ATTR);

            QString showByDefaultString = QString(showByDefaultAttrString);
            showByDefaultString = showByDefaultString.trimmed();

            bool hideByDefault = (QString::compare(showByDefaultString, "false", Qt::CaseInsensitive) == 0) || (QString::compare(showByDefaultString, "0") == 0);
            pToolbar->SetShowByDefault(!hideByDefault);
        }
    }

    for (int i = 0; i < toolBoxNode->getChildCount(); ++i)
    {
        XmlNodeRef macroNode = toolBoxNode->getChild(i);
        QString title = macroNode->getAttr("title");
        QString shortcutName = macroNode->getAttr("shortcut");
        QString iconPath = macroNode->getAttr("icon");

        int idx = -1;
        CToolBoxMacro* pMacro = NewMacro(title, bToolbox, &idx);
        if (!pMacro || idx == -1)
        {
            continue;
        }

        pMacro->Load(macroNode);
        pMacro->SetShortcutName(shortcutName.toLatin1().data());
        pMacro->SetIconPath(iconPath.toLatin1().data());
        pMacro->SetToolbarId(-1);

        if (!pToolbar)
        {
            continue;
        }

        string shelfPath = PathUtil::GetParentDirectory(xmlpath.toLatin1().data());
        string fullIconPath = PathUtil::AddSlash(shelfPath.c_str());
        fullIconPath.append(iconPath.toLatin1().data());

        pMacro->SetIconPath(fullIconPath);

        QString toolTip(macroNode->getAttr("tooltip"));
        pMacro->action()->setToolTip(toolTip);

        int actionId;

        if (pMacro->GetCommandCount() == 0 || pMacro->GetCommandAt(0)->m_type == CToolBoxCommand::eT_INVALID_COMMAND)
        {
            actionId = ID_TOOLBAR_SEPARATOR;
        }
        else
        {
            actionId = bToolbox ? ID_TOOL_FIRST + idx : ID_TOOL_SHELVE_FIRST + idx;

            // KDAB: handler will be executed when ActionManager passes the id to MainFrm. We need to
            // disconnect the already connected handler here or it will be executed twice.
            QObject::disconnect(pMacro->action(), &QAction::triggered, nullptr, nullptr);

            actionManager->AddAction(actionId, pMacro->action());
        }

        pToolbar->AddAction(actionId);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::UpdateShortcutsAndIcons()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CXTPShortcutManager* pShortcutMgr = nullptr;
#ifdef KDAB_TEMPORARILY_REMOVED
    // Port to KeyboardCustomizationDialog after porting to Qt/QAction, don't use XTPShortcutManager()
    /// Shortcuts
    pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
#endif
    if (pShortcutMgr == NULL)
    {
        return;
    }

    CToolBoxManager* pToolBoxMgr(GetIEditor()->GetToolBoxManager());
    if (pToolBoxMgr == NULL)
    {
        return;
    }

    CXTPShortcutManagerAccelTable* pAccelTable = pShortcutMgr->GetDefaultAccelerator();
    for (int i = 0; i < pAccelTable->GetCount(); )
    {
        CXTPShortcutManagerAccel* pAccel = pAccelTable->GetAt(i);
        if ((pAccel->cmd >= ID_TOOL_FIRST && pAccel->cmd <= ID_TOOL_LAST) || (pAccel->cmd >= ID_TOOL_SHELVE_FIRST && pAccel->cmd <= ID_TOOL_SHELVE_LAST))
        {
            pAccelTable->RemoveAt(i);
            continue;
        }
        ++i;
    }


    const int macroCount = pToolBoxMgr->GetMacroCount(true);
    for (int i = 0; i < macroCount; ++i)
    {
        CToolBoxMacro* pMacro = pToolBoxMgr->GetMacro(i, true);
        if (!pMacro)
        {
            continue;
        }

        QString shortcutName(pMacro->GetShortcutName().toString());
        if (shortcutName.isEmpty())
        {
            continue;
        }

        if (!CToolBoxManager::AddShortcut(ID_TOOL_FIRST + i, QtUtil::ToCString(shortcutName)))
        {
            pMacro->SetShortcutName("");
        }
    }

    const int shelveMacroCount = pToolBoxMgr->GetMacroCount(false);
    for (int i = 0; i < shelveMacroCount; ++i)
    {
        CToolBoxMacro* pMacro = pToolBoxMgr->GetMacro(i, false);
        if (!pMacro)
        {
            continue;
        }

        QString shortcutName(pMacro->GetShortcutName().toString());
        if (shortcutName.isEmpty())
        {
            continue;
        }

        if (!CToolBoxManager::AddShortcut(ID_TOOL_SHELVE_FIRST + i, QtUtil::ToCString(shortcutName)))
        {
            pMacro->SetShortcutName("");
        }
    }
#endif
}


//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Save() const
{
    XmlNodeRef toolBoxNode = XmlHelpers::CreateXmlNode(TOOLBOXMACROS_NODE);
    for (size_t i = 0; i < m_macros.size(); ++i)
    {
        if (m_macros[i]->GetToolbarId() != -1)
        {
            continue;
        }

        XmlNodeRef macroNode = toolBoxNode->newChild("macro");
        macroNode->setAttr("title", m_macros[i]->GetTitle().toLatin1().data());
        macroNode->setAttr("shortcut", m_macros[i]->GetShortcutName().toString().toLatin1().data());
        macroNode->setAttr("icon", m_macros[i]->GetIconPath().toLatin1().data());
        m_macros[i]->Save(macroNode);
    }
    QString path;
    GetSaveFilePath(path);
    XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), toolBoxNode, path.toLatin1().data());
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Clear()
{
    for (size_t i = 0; i < m_macros.size(); ++i)
    {
        RemoveMacroShortcut(i, true);
        delete m_macros[i];
    }
    m_macros.clear();
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro(int iIndex, bool bToolbox) const
{
    if (iIndex >= 0 && iIndex < GetMacroCount(bToolbox) && GetMacro(iIndex, bToolbox))
    {
        GetMacro(iIndex, bToolbox)->Execute();
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro(const QString& name, bool bToolbox) const
{
    if (bToolbox)
    {
        // Find tool with this name.
        for (size_t i = 0; i < m_macros.size(); ++i)
        {
            if (QString::compare(m_macros[i]->GetTitle(), name, Qt::CaseInsensitive) == 0)
            {
                ExecuteMacro(int(i), bToolbox);
                break;
            }
        }
    }
    else
    {
        // Find tool with this name.
        for (size_t i = 0; i < m_shelveMacros.size(); ++i)
        {
            if (QString::compare(m_shelveMacros[i]->GetTitle(), name, Qt::CaseInsensitive) == 0)
            {
                ExecuteMacro(int(i), bToolbox);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::SwapMacro(int index1, int index2, bool bToolbox)
{
    assert(0 <= index1 && index1 < GetMacroCount(bToolbox));
    assert(0 <= index2 && index2 < GetMacroCount(bToolbox));
    if (bToolbox)
    {
        std::swap(m_macros[index1], m_macros[index2]);
    }
    else
    {
        std::swap(m_shelveMacros[index1], m_shelveMacros[index2]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::RemoveMacro(int index, bool bToolbox)
{
    assert(0 <= index && index < GetMacroCount(bToolbox));

    RemoveMacroShortcut(index, bToolbox);
    if (bToolbox)
    {
        delete m_macros[index];
        m_macros[index] = nullptr;
        m_macros.erase(m_macros.begin() + index);
    }
    else
    {
        delete m_shelveMacros[index];
        m_shelveMacros[index] = nullptr;
        m_shelveMacros.erase(m_shelveMacros.begin() + index);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::RemoveMacroShortcut(int index, bool bToolbox)
{
    if (index >= GetMacroCount(bToolbox))
    {
        return;
    }

#ifdef KDAB_TEMPORARILY_REMOVED
    CXTPShortcutManager* pShortcutMgr = nullptr;
#ifdef KDAB_TEMPORARILY_REMOVED
    // Port to KeyboardCustomizationDialog after porting to Qt/QAction, don't use XTPShortcutManager()
    CXTPShortcutManager* pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
#endif
    if (pShortcutMgr == NULL)
    {
        return;
    }

    CXTPShortcutManagerAccelTable* pAccelTable = pShortcutMgr->GetDefaultAccelerator();
    if (pAccelTable == NULL)
    {
        return;
    }

    pAccelTable->RemoveAt(bToolbox ? ID_TOOL_FIRST + index : ID_TOOL_SHELVE_FIRST + index);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::GetSaveFilePath(QString& outPath) const
{
    outPath = Path::GetUserSandboxFolder();
    outPath += "Macros.xml";
}

#ifdef KDAB_TEMPORARILY_REMOVED
//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::AddShortcut(CXTPShortcutManagerAccel& accel)
{
    CXTPShortcutManager* pShortcutMgr = nullptr;
#ifdef KDAB_TEMPORARILY_REMOVED
    // Port to KeyboardCustomizationDialog after porting to Qt/QAction, don't use XTPShortcutManager()
    pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
#endif
    if (pShortcutMgr == NULL)
    {
        return false;
    }

    QString shortcutName(pShortcutMgr->Format(&accel, NULL));
    bool isPossible = IsPossibleToAddShortcut(shortcutName);

    if (isPossible)
    {
        CXTPShortcutManagerAccelTable* pAccelTable = pShortcutMgr->GetDefaultAccelerator();
        pAccelTable->Add(accel);
    }

    return isPossible;
}
#endif

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::AddShortcut(int cmdID, const QString& shortcutName)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CXTPShortcutManager* pShortcutMgr = nullptr;
#ifdef KDAB_TEMPORARILY_REMOVED
    // Port to KeyboardCustomizationDialog after porting to Qt/QAction, don't use XTPShortcutManager()
    pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
#endif
    if (pShortcutMgr == NULL)
    {
        return false;
    }

    bool isPossible = IsPossibleToAddShortcut(shortcutName);

    if (isPossible)
    {
        pShortcutMgr->AddShortcut(cmdID, shortcutName);
    }

    return isPossible;
#endif
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::IsPossibleToAddShortcut(const QString& shortcutName)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CXTPShortcutManager* pShortcutMgr = nullptr;
#ifdef KDAB_TEMPORARILY_REMOVED
    // Port to KeyboardCustomizationDialog after porting to Qt/QAction, don't use XTPShortcutManager()
    pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
#endif
    if (pShortcutMgr == NULL)
    {
        return false;
    }

    CXTPShortcutManagerAccel accel;
    return pShortcutMgr->ParseShortcut(shortcutName, &accel);
#endif
    return false;
}

//////////////////////////////////////////////////////////////////////////

const std::vector<AmazonToolbar>& CToolBoxManager::GetToolbars() const
{
    return m_toolbars;
}
