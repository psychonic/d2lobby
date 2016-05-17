/**
 * =============================================================================
 * D2Lobby
 * Copyright (C) 2014 Nicholas Hastings
 * =============================================================================
 */

#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include "constants.h"
#include "gameruleshelper.h"

// MM:S
#include <ISmmPlugin.h>

// SourceHook
#include <sh_list.h>
#include <sh_vector.h>

// SDK
#include <igameevents.h>

#include <steam_gameserver.h>
#include <isteamgamecoordinator.h>

#ifdef D2L_MATCHDATA
#include <jansson.h>

#include "protobuf/dota_gcmessages_server.pb.h"
#endif

#undef SendMessage

class CSteamID;
class IServerTools;

class D2Lobby : public ISmmPlugin,
	public IGameEventListener2
{
public: // ISmmPlugin
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	const char *GetAuthor();
	const char *GetName();
	const char *GetDescription();
	const char *GetURL();
	const char *GetLicense();
	const char *GetVersion();
	const char *GetDate();
	const char *GetLogTag();

public: // IGameEventListener2
	void FireGameEvent(IGameEvent *event) override;

public:
	bool AddHLTVSpectator(uint32 accountId);
	bool AddRadiantPlayer(uint32 accountId, const char *pszName);
	bool AddDirePlayer(uint32 accountId, const char *pszName);
	bool AddSpectatorPlayer(uint32 accountId, const char *pszName);
	bool AddBroadcastChannel(const char *pszCountryCode, const char *pszDesc,
		uint32 caster1, const char *pszName1, uint32 caster2, const char *pszName2,
		uint32 caster3, const char *pszName3, uint32 caster4, const char *pszName4);
	void SetSeriesData(SeriesType type, uint8 radiantWins, uint8 direWins);
#ifdef D2L_MATCHDATA
	inline const char * const GetMatchId() const { return m_szMatchId; }
	void SetMatchId(const char *pszId);
#endif
	void ResetAll();
	void PrintDebug();

private:
	bool InitGlobals(char *error, size_t maxlen);
	void InitHooks();
	void ShutdownHooks();
	void UnhookGC();

private:
	void PopulatePlayerResourceData();

	int GetPlayerTeam(uint32 accountId) const;
	void SetWaitForPlayersCount();

#ifdef D2L_MATCHDATA
	void SendMatchData();
#endif
	void BeginShutdown();

private:
	bool Hook_SteamIDAllowedToConnect(const CSteamID &steamId) const;
	bool Hook_GameInitPost();
	void Hook_GameShutdown();
	void Hook_ServerActivate();
	void Hook_ServerActivatePost();
	void Hook_LevelShutdown();
	void Hook_GameServerSteamAPIActivatedPost();
	void Hook_SetServerHibernationPost(bool bHibernating);
	void Hook_SetCommandClient(int clientslot) { m_iCmdClientEntIdx = clientslot + 1; }
	void Hook_OnClientCommand(CEntityIndex ent, const CCommand &args);
	void Hook_SayDispatch(const CCommandContext &context, const CCommand &command);
	void Hook_OnBotDisconnect(int reason);
#if defined( D2L_EVENTS )
	void Hook_SendUserMessagePost(IRecipientFilter &filter, int message, const google::protobuf::Message &msg);
#endif
#ifdef D2L_EVENTS
	void Hook_CancelGGDispatch(const CCommandContext &context, const CCommand &command);
#endif
#ifdef D2L_MATCHDATA
	EGCResults Hook_SendMessage(uint32 unMsgType, const void *pubData, uint32 cubData);
	EGCResults Hook_RetrieveMessage(uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize);
	void Hook_Think(bool bFinalTick);
#endif

private:
	void Event_PlayerConnect(IGameEvent *pEvent);
	void Event_PlayerDisconnect(IGameEvent *pEvent);
	void Event_PlayerActivate(IGameEvent *pEvent);

private:
	SourceHook::CVector<int> m_GlobalHooks;
	SourceHook::CVector<int> m_GameHooks;

	struct Player
	{
		uint32 AccountId;
		char name[kMaxPlayerNameLength];
	};

	SourceHook::CVector<Player *> m_RadiantPlayers;
	SourceHook::CVector<Player *> m_DirePlayers;
	SourceHook::CVector<Player *> m_SpectatorPlayers;

	struct BroadcastChannel
	{
		char CountryCode[3];
		char Description[80];
		Player Casters[kMaxBroadcastChannelSlots];
		int CasterCount;
	};
	BroadcastChannel m_BroadcastChannels[kMaxBroadcastChannels];
	uint8 m_iBroadcastChannelCnt;

#ifdef D2L_MATCHDATA
	char m_szMatchId[128];
#endif

	struct SeriesData
	{
		SeriesType Type;
		uint8 RadiantWins;
		uint8 DireWins;
	};
	SeriesData m_SeriesData;

#ifdef D2L_MATCHDATA
	float m_flStartupTime;
	float m_flShutdownTime;
	float m_flRadiantDisconnectTime = 0.f;
	float m_flDireDisconnectTime = 0.f;
	float m_flAllDisconnectTime = 0.f;

	json_t *m_MatchData = nullptr;
#endif

private:
#ifdef D2L_MATCHDATA
	enum class D2LState
	{
		Init,
		PreWaitingForPlayers,
		MatchRunning,
		MatchOver,
		ShuttingDown,
	};
	void ChangeMatchState(D2LState newState);
	D2LState m_State = D2LState::Init;

	int m_iSendMessageHook = 0;
	int m_iRetrieveMessageHook = 0;

	DOTA_GameState m_LastState = DOTA_GAMERULES_STATE_INIT;
	float m_LastStateChangeTime = 0.f;
#endif
	int m_iCmdClientEntIdx = 0;
};

extern D2Lobby g_D2Lobby;
extern IVEngineServer *engine;
extern IServerGameDLL *gamedll;
extern IServerGameClients *serverclients;
extern IGameEventManager2 *eventmgr;
extern IServerTools *servertools;
extern IFileSystem *filesystem;
extern CGlobalVars *gpGlobals;
extern ISteamHTTP *http;
extern ConVar match_post_url;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
