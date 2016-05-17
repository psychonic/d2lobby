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

#include "d2lobby.h"
#include "forcedhero.h"
#include "gameruleshelper.h"
#include "playerresourcehelper.h"
#include "scriptmgr.h"

#include <fmtstr.h>

static ForcedHero s_ForcedHero;

CON_COMMAND(set_forced_hero, "set_forced_hero <hero entity name>")
{
	if (args.ArgC() < 2)
	{
		Msg("set_forced_hero <hero entity name>\n");
	}

	s_ForcedHero.SetHero(args[1]);
}

bool ForcedHero::OnLoad()
{
	m_szForcedHero[0] = '\0';
	return true;
}

void ForcedHero::OnServerActivated()
{
	eventmgr->AddListener(this, "game_rules_state_change", true);
}

void ForcedHero::OnLevelShutdown()
{
	eventmgr->RemoveListener(this);
}

void ForcedHero::SetHero(const char *pszHero)
{
	if (!Q_stricmp(pszHero, "random"))
	{
		const char *pszRandomHero = g_ScriptMgr.GetRandomHeroName();
		Q_strncpy(m_szForcedHero, pszRandomHero, sizeof(m_szForcedHero));
	}
	else if (!g_ScriptMgr.IsValidHeroName(pszHero))
	{
		Msg("Cannot find hero named \"%s\"\n", pszHero);
	}
	else
	{
		Q_strncpy(m_szForcedHero, pszHero, sizeof(m_szForcedHero));
	}
}

void ForcedHero::FireGameEvent(IGameEvent *pEvent)
{
	if (!Q_strcmp(pEvent->GetName(), "game_rules_state_change"))
	{
		if (g_GameRules.GetState() == DOTA_GAMERULES_STATE_HERO_SELECTION && m_szForcedHero[0])
		{
			CCommand args;
			args.Tokenize(CFmtStr("dota_select_hero %s reserve", m_szForcedHero));

			for (int i = 0; i < kMaxGamePlayerIds; ++i)
			{
				if (!g_PlayerResource.IsValidPlayer(i))
					continue;

				int idx = g_PlayerResource.GetPlayerEntIndex(i);
				serverclients->ClientCommand(CEntityIndex(idx), args);
			}
		}
	}
}

META_RES ForcedHero::OnClientCommand(CEntityIndex ent, const CCommand &args)
{
	if (m_szForcedHero[0])
	{
		if (args.ArgC() >= 2 && !Q_stricmp(args[0], "dota_select_hero") && !Q_stricmp(args[1], "repick"))
		{
			return MRES_SUPERCEDE;
		}
	}

	return MRES_IGNORED;
}
