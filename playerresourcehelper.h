#pragma once

#include "pluginsystem.h"
#include "constants.h"

#include <basetypes.h>
#include "protobuf/dota_gcmessages_common.pb.h"

struct edict_t;
class CBaseEntity;
class CSteamID;

class PlayerResourceHelper : IPluginSystem
{
public:
	void OnServerActivated() override;
	void OnLevelShutdown() override;
	const char* name() override { return "prh"; }

public:
	void InitPlayer(int playerId, uint32 accountId, const char *pszName, int team, int broadcastChannel = -1, int broadcastSlot = -1);
	void InitBroadcastChannel(int broadcastChannel, const char *pszCountryCode, const char *pszDescription);

public:
	bool IsValidPlayer(int playerId) const;
	bool IsFakeClient(int playerId) const;

	CBaseEntity *GetPlayer(int playerId) const;
	int GetPlayerEntIndex(int playerId) const;
	int GetPlayerTeam(int playerId) const;
	CSteamID &GetPlayerSteamId(int playerId) const;
	const char *GetPlayerName(int playerId) const;
	DOTAConnectionState_t GetPlayerConnectionState(int playerId) const;
	bool IsPlayerConnected(int playerId) const;
	const char *GetPlayerHexColorString(int playerId) const;
	int PlayerIdFromEntIndex(int entidx) const;

	byte GetTeamPlayerCount(int team);
	byte GetTeamConnectedCount(int team);

	edict_t *edict() const;

public:
	void RecheckSelectedHeroes();

private:
	CBaseEntity *m_pResourceEntity;
	static bool s_bHaveOffsets;
	int m_iLastSelectedHero[kMaxGamePlayerIds];

public:
	struct Offsets
	{
		int m_iszPlayerNames;
		int m_iPlayerTeams;
		int m_bFakeClient;
		int m_iPlayerSteamIDs;
		int m_iConnectionState;
		int m_bIsBroadcaster;
		int m_iBroadcasterChannel;
		int m_iBroadcasterChannelSlot;
		int m_iszBroadcasterChannelDescription;
		int m_iszBroadcasterChannelCountryCode;
		int m_nSelectedHeroID;
	};
	
private:
	static Offsets m_Offsets;
};

extern PlayerResourceHelper g_PlayerResource;