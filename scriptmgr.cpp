/**
 * D2Lobby - A Metamod:Source plugin for Dota 2 matches on dedicated servers.
 * Copyright (C) 2015  Nicholas Hastings <nshastings@gmail.com>
 * 
 * This file is part of D2Lobby.
 * 
 * D2Lobby is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * D2Lobby is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

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