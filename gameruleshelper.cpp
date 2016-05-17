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

#include "gameruleshelper.h"
#include "constants.h"
#include "util.h"

#include <server_class.h>

bool GameRulesHelper::s_bHaveOffsets = false;
GameRulesHelper::Offsets GameRulesHelper::m_Offsets;
CUtlVector<char *> bannedHeroes;

char** clslist = new char*[127]
{
	"npc_dota_hero_base",
		"npc_dota_hero_antimage",
		"npc_dota_hero_axe",
		"npc_dota_hero_bane",
		"npc_dota_hero_bloodseeker",
		"npc_dota_hero_crystal_maiden",
		"npc_dota_hero_drow_ranger",
		"npc_dota_hero_earthshaker",
		"npc_dota_hero_juggernaut",
		"npc_dota_hero_mirana",
		"npc_dota_hero_morphling",
		"npc_dota_hero_nevermore",
		"npc_dota_hero_phantom_lancer",
		"npc_dota_hero_puck",
		"npc_dota_hero_pudge",
		"npc_dota_hero_razor",
		"npc_dota_hero_sand_king",
		"npc_dota_hero_storm_spirit",
		"npc_dota_hero_sven",
		"npc_dota_hero_tiny",
		"npc_dota_hero_vengefulspirit",
		"npc_dota_hero_windrunner",
		"npc_dota_hero_zuus",
		"npc_dota_hero_kunkka",
		"npc_dota_nohero?",
		"npc_dota_hero_lina",
		"npc_dota_hero_lion",
		"npc_dota_hero_shadow_shaman",
		"npc_dota_hero_slardar",
		"npc_dota_hero_tidehunter",
		"npc_dota_hero_witch_doctor",
		"npc_dota_hero_lich",
		"npc_dota_hero_riki",
		"npc_dota_hero_enigma",
		"npc_dota_hero_tinker",
		"npc_dota_hero_sniper",
		"npc_dota_hero_necrolyte",
		"npc_dota_hero_warlock",
		"npc_dota_hero_beastmaster",
		"npc_dota_hero_queenofpain",
		"npc_dota_hero_venomancer",
		"npc_dota_hero_faceless_void",
		"npc_dota_hero_skeleton_king",
		"npc_dota_hero_death_prophet",
		"npc_dota_hero_phantom_assassin",
		"npc_dota_hero_pugna",
		"npc_dota_hero_templar_assassin",
		"npc_dota_hero_viper",
		"npc_dota_hero_luna",
		"npc_dota_hero_dragon_knight",
		"npc_dota_hero_dazzle",
		"npc_dota_hero_rattletrap",
		"npc_dota_hero_leshrac",
		"npc_dota_hero_furion",
		"npc_dota_hero_life_stealer",
		"npc_dota_hero_dark_seer",
		"npc_dota_hero_clinkz",
		"npc_dota_hero_omniknight",
		"npc_dota_hero_enchantress",
		"npc_dota_hero_huskar",
		"npc_dota_hero_night_stalker",
		"npc_dota_hero_broodmother",
		"npc_dota_hero_bounty_hunter",
		"npc_dota_hero_weaver",
		"npc_dota_hero_jakiro",
		"npc_dota_hero_batrider",
		"npc_dota_hero_chen",
		"npc_dota_hero_spectre",
		"npc_dota_hero_ancient_apparition",
		"npc_dota_hero_doom_bringer",
		"npc_dota_hero_ursa",
		"npc_dota_hero_spirit_breaker",
		"npc_dota_hero_gyrocopter",
		"npc_dota_hero_alchemist",
		"npc_dota_hero_invoker",
		"npc_dota_hero_silencer",
		"npc_dota_hero_obsidian_destroyer",
		"npc_dota_hero_lycan",
		"npc_dota_hero_brewmaster",
		"npc_dota_hero_shadow_demon",
		"npc_dota_hero_lone_druid",
		"npc_dota_hero_chaos_knight",
		"npc_dota_hero_meepo",
		"npc_dota_hero_treant",
		"npc_dota_hero_ogre_magi",
		"npc_dota_hero_undying",
		"npc_dota_hero_rubick",
		"npc_dota_hero_disruptor",
		"npc_dota_hero_nyx_assassin",
		"npc_dota_hero_naga_siren",
		"npc_dota_hero_keeper_of_the_light",
		"npc_dota_hero_wisp",
		"npc_dota_hero_visage",
		"npc_dota_hero_slark",
		"npc_dota_hero_medusa",
		"npc_dota_hero_troll_warlord",
		"npc_dota_hero_centaur",
		"npc_dota_hero_magnataur",
		"npc_dota_hero_shredder",
		"npc_dota_hero_bristleback",
		"npc_dota_hero_tusk",
		"npc_dota_hero_skywrath_mage",
		"npc_dota_hero_abaddon",
		"npc_dota_hero_elder_titan",
		"npc_dota_hero_legion_commander",
		"npc_dota_hero_ember_spirit",
		"npc_dota_hero_earth_spirit",
		"npc_dota_hero_abyssal_underlord",
		"npc_dota_hero_terrorblade",
		"npc_dota_hero_phoenix",
		"npc_dota_hero_oracle"
};

void GameRulesHelper::BanThem()
{
	printf("banning %d heroes!\n", bannedHeroes.Count());
	for (int i = 0; i < bannedHeroes.Count(); i++)
	{
		const char* ban = bannedHeroes[i];
		printf("%s\n", ban);
		for (int i = 0; i < 109; i++)
		{
			if (!Q_stricmp(clslist[i], ban))
			{
				printf("Banning %s with heroid %d!\n", ban, i);
				SetCurrentHeroAvailable(i, false);
				SetStableHeroAvailable(i, false);
				SetCulledHero(i, true);
			}
		}
	}
}

void GameRulesHelper::OnServerActivated()
{
	
	printf("Doin stuf\n");
	m_pGameRulesProxy = UTIL_FindEntityByClassname(nullptr, "dota_gamerules");
	m_pGameManagerProxy = UTIL_FindEntityByClassname(nullptr, "dota_gamemanager");

	auto *pSendTable = ((IServerUnknown *) m_pGameRulesProxy)->GetNetworkable()->GetServerClass()->m_pTable;
	auto *pManagerSendTable = ((IServerUnknown *)m_pGameManagerProxy)->GetNetworkable()->GetServerClass()->m_pTable;

	if (!s_bHaveOffsets)
	{
		m_Offsets.m_nSeriesType = UTIL_FindInSendTable(pSendTable, "m_nSeriesType");
		m_Offsets.m_nRadiantSeriesWins = UTIL_FindInSendTable(pSendTable, "m_nRadiantSeriesWins");
		m_Offsets.m_nDireSeriesWins = UTIL_FindInSendTable(pSendTable, "m_nDireSeriesWins");
		m_Offsets.m_nGameState = UTIL_FindInSendTable(pSendTable, "m_nGameState");
		m_Offsets.m_fGameTime = UTIL_FindInSendTable(pSendTable, "m_fGameTime");
		m_Offsets.m_nGGTeam = UTIL_FindInSendTable(pSendTable, "m_nGGTeam");
		m_Offsets.m_flGGEndsAtTime = UTIL_FindInSendTable(pSendTable, "m_flGGEndsAtTime");
		m_Offsets.m_iGameMode = UTIL_FindInSendTable(pSendTable, "m_iGameMode");
		m_Offsets.m_bGamePaused = UTIL_FindInSendTable(pSendTable, "m_bGamePaused");
		m_Offsets.m_iPauseTeam = UTIL_FindInSendTable(pSendTable, "m_iPauseTeam");
		m_Offsets.m_StableHeroAvailable = UTIL_FindInSendTable(pManagerSendTable, "m_StableHeroAvailable");
		m_Offsets.m_CurrentHeroAvailable = UTIL_FindInSendTable(pManagerSendTable, "m_CurrentHeroAvailable");
		m_Offsets.m_CulledHeroes = UTIL_FindInSendTable(pManagerSendTable, "m_CulledHeroes");

		s_bHaveOffsets = true;
	}

	m_pGameRules = nullptr;
	m_pGameManager = nullptr;

	for (int i = 0; i < pSendTable->GetNumProps(); i++)
	{
		auto pProp = pSendTable->GetProp(i);

		if (pProp->GetDataTable() && !Q_strcmp("dota_gamerules_data", pProp->GetName()))
		{
			auto proxyFn = pProp->GetDataTableProxyFn();
			if (proxyFn)
			{
				CSendProxyRecipients recp;
				m_pGameRules = proxyFn(NULL, NULL, NULL, &recp, 0);
			}

			break;
		}
	}

	for (int i = 0; i < pManagerSendTable->GetNumProps(); i++)
	{
		auto pProp = pManagerSendTable->GetProp(i);

		if (pProp->GetDataTable() && !Q_strcmp("dota_gamemanager_data", pProp->GetName()))
		{
			auto proxyFn = pProp->GetDataTableProxyFn();

			if (proxyFn)
			{
				CSendProxyRecipients recp;
				m_pGameManager = proxyFn(NULL, NULL, NULL, &recp, 0);
				printf("Manager name: %d\n", m_pGameManager);
			}

			break;
		}
	}

	const char *pszBannedHeroes = CommandLine()->ParmValue("-d2bannedheroes", "");
	if (pszBannedHeroes && pszBannedHeroes[0])
	{
		V_SplitString(pszBannedHeroes, ",", bannedHeroes);
	}

	BanThem();
}

void GameRulesHelper::OnLevelShutdown()
{
	m_pGameRules = nullptr;
	m_pGameRulesProxy = nullptr;
}

CON_COMMAND(make_team_lose, "")
{
	g_GameRules.ForceGameEnd(args.ArgC() > 1 ? Q_atoi(args[1]) : 3);
}

void GameRulesHelper::ForceGameEnd(int losingTeam)
{
	SetData(m_Offsets.m_nGGTeam, losingTeam);
	SetData(m_Offsets.m_flGGEndsAtTime, 1.f);
}

DOTA_GameMode GameRulesHelper::GetMode() const
{
	if (m_pGameRules == nullptr || !s_bHaveOffsets)
	{
		return DOTA_GAMEMODE_AP;
	}

	return *(DOTA_GameMode *) ((intp) m_pGameRules + m_Offsets.m_iGameMode);
}

DOTA_GameState GameRulesHelper::GetState() const
{
	if (m_pGameRules == nullptr || !s_bHaveOffsets)
	{
		return DOTA_GAMERULES_STATE_INIT;
	}

	return *(DOTA_GameState *) ((intp) m_pGameRules + m_Offsets.m_nGameState);
}

float GameRulesHelper::GetGameTime() const
{
	if (m_pGameRules == nullptr || !s_bHaveOffsets)
	{
		return -5.f;
	}

	return *(float *) ((intp) m_pGameRules + m_Offsets.m_fGameTime);
}

void GameRulesHelper::SetSeriesData(SeriesType type, uint8 radiantWins, uint8 direWins)
{
	SetData(m_Offsets.m_nSeriesType, type);
	SetData(m_Offsets.m_nRadiantSeriesWins, radiantWins);
	SetData(m_Offsets.m_nDireSeriesWins, direWins);
}

void GameRulesHelper::SetCurrentHeroAvailable(int heroid, byte avail)
{
	SetManagerData<byte>(m_Offsets.m_CurrentHeroAvailable + (heroid), avail);
}

void GameRulesHelper::SetStableHeroAvailable(int heroid, byte avail)
{
	SetManagerData<byte>(m_Offsets.m_StableHeroAvailable + (heroid), avail);
}

void GameRulesHelper::SetCulledHero(int heroid, byte cull)
{
	printf("Culled..? %d\n", m_Offsets.m_CulledHeroes);
	SetManagerData<byte>(m_Offsets.m_CulledHeroes + (heroid), cull);
}

extern ConVar d2lobby_gg_time;
void GameRulesHelper::CallGG(int team)
{
	if (d2lobby_gg_time.GetInt() == -1)
		return;

	if (*(int *) ((intp) m_pGameRules + m_Offsets.m_nGGTeam) <= 0)
	{
		SetData(m_Offsets.m_nGGTeam, team);
		SetData(m_Offsets.m_flGGEndsAtTime, GetGameTime() + d2lobby_gg_time.GetFloat());
	}
}

int GameRulesHelper::GetGGTeam() const
{
	int currentGGTeam = *(int *) ((intp) m_pGameRules + m_Offsets.m_nGGTeam);
	float currentGGTime = *(float *) ((intp) m_pGameRules + m_Offsets.m_flGGEndsAtTime);
	if (currentGGTeam != 0 && currentGGTime != 0.0f && currentGGTime >= GetGameTime())
	{
		return currentGGTeam;
	}

	return 0;
}

bool GameRulesHelper::IsPaused() const
{
	return *(bool *) ((intp) m_pGameRules + m_Offsets.m_bGamePaused);
}

void GameRulesHelper::SetPaused(bool bPaused)
{
	SetData(m_Offsets.m_bGamePaused, bPaused);
}

int GameRulesHelper::GetPauseTeam() const
{
	return *(int *) ((intp) m_pGameRules + m_Offsets.m_iPauseTeam);
}

void GameRulesHelper::SetPauseTeam(int team)
{
	SetData(m_Offsets.m_iPauseTeam, team);
}

bool GameRulesHelper::IsBanned(const char *cls)
{
	for (int i = 0; i < bannedHeroes.Count(); i++)
	{
		const char* ban = bannedHeroes[i];
		if (!Q_stricmp(ban, cls))
			return true;
	}
	return false;
}

META_RES GameRulesHelper::OnClientCommand(CEntityIndex ent, const CCommand &args)
{
		if (args.ArgC() >= 2 && !Q_stricmp(args[0], "dota_select_hero") && IsBanned(args[1]))
		{
			return MRES_SUPERCEDE;
		}
	

	return MRES_IGNORED;
}

GameRulesHelper g_GameRules;
