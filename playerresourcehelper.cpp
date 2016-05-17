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

#include "playerresourcehelper.h"
#include "eventlog.h"
#include "scriptmgr.h"
#include "util.h"

#include <game/server/iplayerinfo.h>
#include <server_class.h>

#include <steamclientpublic.h>

extern IPlayerInfoManager *plyrmgr;

bool PlayerResourceHelper::s_bHaveOffsets = false;
PlayerResourceHelper::Offsets PlayerResourceHelper::m_Offsets;

static SendTableProxyFn s_SelectedHeroProxyBackup;

static void *SendProxy_OnSelectedHeroChanged(const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID)
{
	g_PlayerResource.RecheckSelectedHeroes();
	if (s_SelectedHeroProxyBackup)
		return s_SelectedHeroProxyBackup(pProp, pStructBase, pData, pRecipients, objectID);
	else
		return (void*) pData;
}

void PlayerResourceHelper::OnServerActivated()
{
	m_pResourceEntity = UTIL_FindEntityByClassname(nullptr, "dota_player_manager");

	if (!s_bHaveOffsets)
	{
		auto *pSendTable = ((IServerUnknown *) m_pResourceEntity)->GetNetworkable()->GetServerClass()->m_pTable;

		m_Offsets.m_iszPlayerNames = UTIL_FindInSendTable(pSendTable, "m_iszPlayerNames");
		m_Offsets.m_iPlayerTeams = UTIL_FindInSendTable(pSendTable, "m_iPlayerTeams");
		m_Offsets.m_bFakeClient = UTIL_FindInSendTable(pSendTable, "m_bFakeClient");
		m_Offsets.m_iPlayerSteamIDs = UTIL_FindInSendTable(pSendTable, "m_iPlayerSteamIDs");
		m_Offsets.m_iConnectionState = UTIL_FindInSendTable(pSendTable, "m_iConnectionState");
		m_Offsets.m_bIsBroadcaster = UTIL_FindInSendTable(pSendTable, "m_bIsBroadcaster");
		m_Offsets.m_iBroadcasterChannel = UTIL_FindInSendTable(pSendTable, "m_iBroadcasterChannel");
		m_Offsets.m_iBroadcasterChannelSlot = UTIL_FindInSendTable(pSendTable, "m_iBroadcasterChannelSlot");
		m_Offsets.m_iszBroadcasterChannelDescription = UTIL_FindInSendTable(pSendTable, "m_iszBroadcasterChannelDescription");
		m_Offsets.m_iszBroadcasterChannelCountryCode = UTIL_FindInSendTable(pSendTable, "m_iszBroadcasterChannelCountryCode");

		SendProp *pSelectedHeroID;
		m_Offsets.m_nSelectedHeroID = UTIL_FindInSendTable(pSendTable, "m_nSelectedHeroID", 0, &pSelectedHeroID);

		s_SelectedHeroProxyBackup = pSelectedHeroID->GetDataTableProxyFn();
		pSelectedHeroID->SetDataTableProxyFn(SendProxy_OnSelectedHeroChanged);

		s_bHaveOffsets = true;
	}

	for (int &h : m_iLastSelectedHero)
	{
		h = -1;
	}
}

void PlayerResourceHelper::OnLevelShutdown()
{
	m_pResourceEntity = nullptr;
}

void PlayerResourceHelper::InitPlayer(int playerId, uint32 accountId, const char *pszName, int team, int broadcastChannel, int broadcastSlot)
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;

	CSteamID sid(accountId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual);
	((uint64 *) &(plyrRes[m_Offsets.m_iPlayerSteamIDs]))[playerId] = sid.ConvertToUint64();
	((string_t *) &(plyrRes[m_Offsets.m_iszPlayerNames]))[playerId] = UTIL_AllocPooledString(pszName);
	((int32 *) &(plyrRes[m_Offsets.m_iPlayerTeams]))[playerId] = team;
	((bool *) &(plyrRes[m_Offsets.m_bFakeClient]))[playerId] = false;

	bool isBroadcaster = (broadcastChannel > -1);
	((bool *) &(plyrRes[m_Offsets.m_bIsBroadcaster]))[playerId] = isBroadcaster;
	if (isBroadcaster)
	{
		((int32 *) &(plyrRes[m_Offsets.m_iBroadcasterChannel]))[playerId] = broadcastChannel;
		((int32 *) &(plyrRes[m_Offsets.m_iBroadcasterChannelSlot]))[playerId] = broadcastSlot;
	}
}

void PlayerResourceHelper::InitBroadcastChannel(int broadcastChannel, const char *pszCountryCode, const char *pszDescription)
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	((string_t *) &plyrRes[m_Offsets.m_iszBroadcasterChannelCountryCode])[broadcastChannel] = UTIL_AllocPooledString(pszCountryCode);
	((string_t *) &plyrRes[m_Offsets.m_iszBroadcasterChannelDescription])[broadcastChannel] = UTIL_AllocPooledString(pszDescription);
}

bool PlayerResourceHelper::IsValidPlayer(int playerId) const
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	return ((uint64 *) &(plyrRes[m_Offsets.m_iPlayerSteamIDs]))[playerId] != 0;
}

bool PlayerResourceHelper::IsFakeClient(int playerId) const
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	return ((bool *) &(plyrRes[m_Offsets.m_bFakeClient]))[playerId];
}

CBaseEntity *PlayerResourceHelper::GetPlayer(int playerId) const
{
	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; ++i)
	{
		edict_t *pEdict = PEntityOfEntIndex(i);
		if (!pEdict || pEdict->IsFree())
			continue;

		IServerEntity *pServerEntity = pEdict->GetIServerEntity();
		if (!pServerEntity)
			continue;

		CBaseEntity *pEntity = pServerEntity->GetBaseEntity();
		if (!pEntity)
			continue;

		static int m_iPlayerID = UTIL_FindInSendTable(pEdict->GetNetworkable()->GetServerClass()->m_pTable, "m_iPlayerID");

		if (*(int32 *) ((intp) pEntity + m_iPlayerID) == playerId)
			return pEntity;
	}

	return nullptr;
}

bool PlayerResourceHelper::IsPlayerConnected(int playerId) const
{
	return GetPlayerConnectionState(playerId) == DOTA_CONNECTION_STATE_CONNECTED;
}

int PlayerResourceHelper::GetPlayerEntIndex(int playerId) const
{
	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; ++i)
	{
		edict_t *pEdict = PEntityOfEntIndex(i);
		if (!pEdict || pEdict->IsFree())
			continue;

		IServerEntity *pServerEntity = pEdict->GetIServerEntity();
		if (!pServerEntity)
			continue;

		CBaseEntity *pEntity = pServerEntity->GetBaseEntity();
		if (!pEntity)
			continue;

		static int m_iPlayerID = UTIL_FindInSendTable(pEdict->GetNetworkable()->GetServerClass()->m_pTable, "m_iPlayerID");

		if (*(int32 *) ((intp) pEntity + m_iPlayerID) == playerId)
			return i;
	}

	return -1;
}

int PlayerResourceHelper::PlayerIdFromEntIndex(int entidx) const
{
	if (entidx <= 0 || entidx > gpGlobals->maxClients)
		return -1;

	edict_t *pEdict = PEntityOfEntIndex(entidx);
	if (!pEdict || pEdict->IsFree())
		return -1;

	IServerEntity *pServerEntity = pEdict->GetIServerEntity();
	if (!pServerEntity)
		return -1;

	CBaseEntity *pEntity = pServerEntity->GetBaseEntity();
	if (!pEntity)
		return -1;

	static int m_iPlayerID = UTIL_FindInSendTable(pEdict->GetNetworkable()->GetServerClass()->m_pTable, "m_iPlayerID");

	return *(int32 *) ((intp) pEntity + m_iPlayerID);
}

int PlayerResourceHelper::GetPlayerTeam(int playerId) const
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	return ((int32 *) &(plyrRes[m_Offsets.m_iPlayerTeams]))[playerId];
}

byte PlayerResourceHelper::GetTeamPlayerCount(int team)
{
	byte cnt = 0;
	for (int i = 0; i < kMaxGamePlayerIds; ++i)
	{
		if (IsValidPlayer(i) && GetPlayerTeam(i) == team)
			++cnt;
	}

	return cnt;
}

byte PlayerResourceHelper::GetTeamConnectedCount(int team)
{
	byte cnt = 0;
	for (int i = 0; i < kMaxGamePlayerIds; ++i)
	{
		if (IsValidPlayer(i) && GetPlayerTeam(i) == team)
		{
			auto connState = GetPlayerConnectionState(i);
			if (connState == DOTA_CONNECTION_STATE_CONNECTED
				|| connState == DOTA_CONNECTION_STATE_LOADING)
			{
				++cnt;
			}
		}
	}

	return cnt;
}

CSteamID &PlayerResourceHelper::GetPlayerSteamId(int playerId) const
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	return ((CSteamID *) &(plyrRes[m_Offsets.m_iPlayerSteamIDs]))[playerId];
}

const char *PlayerResourceHelper::GetPlayerName(int playerId) const
{
	int entIdx = g_PlayerResource.GetPlayerEntIndex(playerId);
	if (entIdx > 0)
	{
		edict_t *pEdict = PEntityOfEntIndex(entIdx);
		if (pEdict && !pEdict->IsFree())
		{
			auto *pInfo = plyrmgr->GetPlayerInfo(pEdict);
			if (pInfo && pInfo->IsConnected())
				return pInfo->GetName();
		}
	}

	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	return ((string_t *) &(plyrRes[m_Offsets.m_iszPlayerNames]))[playerId].ToCStr();
}

DOTAConnectionState_t PlayerResourceHelper::GetPlayerConnectionState(int playerId) const
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	return ((DOTAConnectionState_t *) &(plyrRes[m_Offsets.m_iConnectionState]))[playerId];
}

void PlayerResourceHelper::RecheckSelectedHeroes()
{
	unsigned char *plyrRes = (unsigned char *) m_pResourceEntity;
	for (int i = 0; i < kMaxGamePlayerIds; ++i)
	{
		int currHero = ((int32 *) &(plyrRes[m_Offsets.m_nSelectedHeroID]))[i];
		if (currHero != m_iLastSelectedHero[i])
		{
			// event
			m_iLastSelectedHero[i] = currHero;
			g_EventLogger.LogSelectedHero(i, g_ScriptMgr.GetHeroNameByID(currHero));
		}
	}
}

static const char *s_szPlayerColors[kMaxGamePlayerIds] = {
	"#3375FF",
	"#66FFBF",
	"#BF00BF",
	"#F3F00B",
	"#FF6B00",
	"#FE86C2",
	"#A1B447",
	"#65D9F7",
	"#008321",
	"#A46900"
};

const char *PlayerResourceHelper::GetPlayerHexColorString(int playerId) const
{
	if (playerId < 0 || playerId >= kMaxGamePlayerIds)
		return "#FFFFFF";

	return s_szPlayerColors[playerId];
}

edict_t *PlayerResourceHelper::edict() const
{
	return ((IServerUnknown *) m_pResourceEntity)->GetNetworkable()->GetEdict();
}

PlayerResourceHelper g_PlayerResource;
