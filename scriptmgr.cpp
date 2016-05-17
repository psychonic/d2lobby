#include "scriptmgr.h"

#include "d2lobby.h"

#include <filesystem.h>
#include <vstdlib/random.h>

ScriptMgr g_ScriptMgr;

bool ScriptMgr::OnLoad()
{
	m_pkvHeroes = new KeyValues("DOTAHeroes");
	return m_pkvHeroes->LoadFromFile(filesystem, "scripts/npc/npc_heroes.txt");
}

void ScriptMgr::OnUnload()
{
	m_pkvHeroes->deleteThis();
}

bool ScriptMgr::IsValidHeroName(const char *pszName)
{
	return !!m_pkvHeroes->FindKey(pszName);
}

const char *ScriptMgr::GetRandomHeroName()
{
	CUtlVector<const char *> validHeroes;
	FOR_EACH_SUBKEY(m_pkvHeroes, h)
	{
		if (!h->GetBool("Enabled"))
			continue;

		validHeroes.AddToTail(h->GetName());
	}

	return validHeroes[RandomInt(0, validHeroes.Count() - 1)];
}

const char *ScriptMgr::GetHeroNameByID(int id)
{
	FOR_EACH_SUBKEY(m_pkvHeroes, h)
	{
		if (h->GetInt("HeroID") == id)
			return h->GetName();
	}

	return "";
}