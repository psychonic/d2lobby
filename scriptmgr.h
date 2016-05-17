#pragma once

#include "pluginsystem.h"

class ScriptMgr : public IPluginSystem
{
public:
	bool OnLoad() override;
	const char* name() override { return "scr"; }
	void OnUnload() override;

public:
	bool IsValidHeroName(const char *pszName);
	const char *GetRandomHeroName();
	const char *GetHeroNameByID(int id);
private:
	KeyValues *m_pkvHeroes = nullptr;
};

extern ScriptMgr g_ScriptMgr;