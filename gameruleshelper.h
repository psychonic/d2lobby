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

#pragma once

#include "pluginsystem.h"

#include <basetypes.h>
#include <edict.h>
#include "protobuf/dota_gcmessages_common.pb.h"

class CBaseEntity;

enum class SeriesType : uint8
{
	None,
	BO3,
	BO5,
};

class GameRulesHelper : IPluginSystem
{
public:
	void OnServerActivated() override;
	void OnLevelShutdown() override;

public:
	const char* name() override { return "GRH"; }
	void ForceGameEnd(int losingTeam);
	DOTA_GameMode GetMode() const;
	DOTA_GameState GetState() const;
	void SetSeriesData(SeriesType type, uint8 radiantWins, uint8 direWins);
	void CallGG(int team);
	int GetGGTeam() const;
	float GetGameTime() const;
	bool IsPaused() const;
	void SetPaused(bool bPaused);
	int GetPauseTeam() const;
	void SetPauseTeam(int team);
	void SetStableHeroAvailable(int team, byte avail);
	void SetCurrentHeroAvailable(int team, byte avail);
	void SetCulledHero(int heroid, byte cull);
	META_RES OnClientCommand(CEntityIndex ent, const CCommand &args) override;
	bool IsBanned(const char* cls);

private:
	template <typename T>
	void SetData(int offset, T value);
	template <typename T>
	void SetManagerData(int offset, T value);
	void BanThem();

private:
	CBaseEntity *m_pGameRulesProxy = nullptr;
	CBaseEntity *m_pGameManagerProxy = nullptr;
	void *m_pGameRules = nullptr;
	void *m_pGameManager = nullptr;
	static bool s_bHaveOffsets;

public:
	struct Offsets
	{
		int m_nSeriesType;
		int m_nRadiantSeriesWins;
		int m_nDireSeriesWins;
		int m_nGameState;
		int m_fGameTime;
		int m_nGGTeam;
		int m_flGGEndsAtTime;
		int m_fExtraTimeRemaining;
		int m_iPlayerIDsInControl;
		int m_BannedHeroes;
		int m_SelectedHeroes;
		int m_iGameMode;
		int m_bGamePaused;
		int m_iPauseTeam;
		int m_StableHeroAvailable;
		int m_CurrentHeroAvailable;
		int m_CulledHeroes;
	};
	
private:
	static Offsets m_Offsets;
};

template <typename T>
inline void GameRulesHelper::SetData(int offset, T value)
{
	auto *pProxyEdict = ((IServerUnknown *) m_pGameRulesProxy)->GetNetworkable()->GetEdict();

	*(T *) ((intp) m_pGameRules + offset) = value;
	*(T *) ((intp) m_pGameRulesProxy + offset) = value;
	pProxyEdict->StateChanged(offset);
}

template <typename T>
inline void GameRulesHelper::SetManagerData(int offset, T value)
{
	auto *pProxyEdict = ((IServerUnknown *)m_pGameManagerProxy)->GetNetworkable()->GetEdict();

	*(T *)((intp)m_pGameManager + offset) = value;
	*(T *)((intp)m_pGameManagerProxy + offset) = value;
	pProxyEdict->StateChanged(offset);
}

extern GameRulesHelper g_GameRules;
