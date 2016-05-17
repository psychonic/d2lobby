#pragma once

#include "constants.h"
#include <jansson.h>
#include <sh_vector.h>

#include <igameevents.h>

#include "pluginsystem.h"

#include "protobuf/dota_gcmessages_common.pb.h"

class CSteamID;
class IRecipientFilter;

enum class EventType : int
{
	GameStateChange,
	PlayerConnect,
	PlayerDisconnect,
	PlayerTeam,
	HeroDeath,
	TowerKill,
	CourierKill,
	RoshanKill,
	RuneBottled,
	RuneUsed,
	AegisPickup,
	AegisSteal,
	Buyback,
	AegisDeny,
	FirstBlood,
	TowerDeny,
	ItemPurchase,
	CallGG,
	CancelGG,
	HeroSelection,
	RaxKill,
};

enum class DotaLane : int
{
	Unknown = 0,
	Top = 1,
	Mid = 2,
	Bot = 3
};

enum class DotaRaxType :int
{
	Unknown = 0,
	Melee = 1,
	Ranged = 2,
};

class EventLogger : public IGameEventListener2, public IPluginSystem
{
public: // IGameEventListener2
	virtual void FireGameEvent(IGameEvent *pEvent) override;
public: // IPluginSystem
	void OnServerActivated() override;
	void OnLevelShutdown() override;
	void OnUserMessage(IRecipientFilter &filter, int type, const google::protobuf::Message &msg) override;
	const char* name() override { return "elogger"; }
public:
	void LogGGCall(uint64 steamId64, int team);
	void LogGGCancel(uint64 steamId64, int team);
	void LogSelectedHero(int playerId, const char *pszHeroName);
private:
	void LogGameStateChange(DOTA_GameState newState);
	void LogPlayerConnect(const char *pszName, int userid);
	void LogPlayerDisconnect(const char *pszName, int userid, int reason);
	void LogPlayerTeam(const char *pszName, int userid, int team);
	void LogHeroKill(int victimId, SourceHook::CVector<int> &killers, uint gold);
	void LogSimplePlayerEvent(EventType type, int playerId);
	void LogCourierKill(int team);
	void LogRoshanKill(int team, uint gold);
	void LogRuneBottle(int playerId, DotaRune rune);
	void LogRuneUse(int playerId, DotaRune rune);
	void LogItemPurchase(int playerId, int itemId);
	void LogTowerKill(CBaseEntity *pTower, int attackerEntIdx);
	void LogRaxKill(CBaseEntity *pRax, int attackerEntIdx);
private:
	json_t *CreateTimedEvent(EventType type);
	void SendAndFreeEvent(json_t *pData);
};

extern EventLogger g_EventLogger;
