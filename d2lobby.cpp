/**
 * =============================================================================
 * D2Lobby
 * Copyright (C) 2014 Nicholas Hastings
 * =============================================================================
 */

// Self
#include "d2lobby.h"
#include "playerresourcehelper.h"
#include "recipientfilters.h"
#include "util.h"

#include "CDetour\detour.h"
#include "CDetour\defines.h"

#if defined( D2L_MATCHDATA ) || defined( D2L_EVENTS )
#include "httpmgr.h"
#endif

#ifdef D2L_EVENTS
#include "eventlog.h"
#endif

#ifdef D2L_MATCHDATA
#include "pb2json.h"
#endif

#include "pluginsystem.h"

// SourceHook
#include <sh_memory.h>

// SDK
#include <game/server/iplayerinfo.h>
#include <icvar.h>
#include <iclient.h>
#include <iserver.h>
#include <server_class.h>
#include <tier0/icommandline.h>
#include <tier0/platform.h>
#include <tier1/fmtstr.h>
#include <tier1/iconvar.h>
#include <toolframework/itoolentity.h>
#include <vstdlib/random.h>

// Steam
#include <steam_gameserver.h>

#ifdef D2L_MATCHDATA
#include <isteamgamecoordinator.h>
#endif

#ifdef D2L_LOGGING
#include <filesystem.h>
#endif

#include "protobuf/dota_gcmessages_common.pb.h"
#include "protobuf/usermessages.pb.h"

#ifdef D2L_MATCHDATA
#include "protobuf/dota_gcmessages_server.pb.h"
#endif



#undef SendMessage

SH_DECL_HOOK1(IServerGCLobby, SteamIDAllowedToConnect, const, 0, bool, const CSteamID &);
SH_DECL_HOOK0(IServerGameDLL, GameInit, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, GameShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK0_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIActivated, SH_NOATTRIB, 0);
SH_DECL_HOOK1_void(IServerGameDLL, SetServerHibernation, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, CEntityIndex, const CCommand &);
#ifdef D2L_MATCHDATA
SH_DECL_HOOK1_void(IServerGameDLL, Think, SH_NOATTRIB, 0, bool);
#endif
#if defined( D2L_EVENTS )
SH_DECL_HOOK3_void(IVEngineServer, SendUserMessage, SH_NOATTRIB, 0, IRecipientFilter &, int, const google::protobuf::Message &);
#endif

SH_DECL_HOOK3(ISteamGameCoordinator, SendMessage, SH_NOATTRIB, 0, EGCResults, uint32, const void *, uint32);
SH_DECL_HOOK4(ISteamGameCoordinator, RetrieveMessage, SH_NOATTRIB, 0, EGCResults, uint32 *, void *, uint32, uint32 *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK2_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommandContext &, const CCommand &);
SH_DECL_HOOK1_void(IClient, Disconnect, SH_NOATTRIB, 0, int);

D2Lobby g_D2Lobby;
IVEngineServer *engine = nullptr;
IGameEventManager2 *eventmgr = nullptr;
IServerGameDLL *gamedll = nullptr;
IServerGameClients *serverclients = nullptr;
IServerTools *servertools = nullptr;
IPlayerInfoManager *plyrmgr = nullptr;
IFileSystem *filesystem = nullptr;
CGlobalVars *gpGlobals = nullptr;

CDetour *AttemptToBuyDetour;
void *attempttobuyptr;
CUtlVector<char *> restrictedItems;





#ifdef D2L_MATCHDATA
ISteamHTTP *http = nullptr;
static ISteamGameCoordinator *gamecoordinator = nullptr;
#endif
static IServer *server = nullptr;

#ifdef D2L_LOGGING
FileHandle_t g_pLogFile;
#endif

ConVar d2lobby_gg_time("d2lobby_gg_time", "10", FCVAR_RELEASE, "Time in seconds that game ends after calling \"gg\". -1 to disable gg call.");

#ifdef D2L_MATCHDATA
ConVar match_post_url("match_post_url", "", FCVAR_RELEASE);

ConVar *dota_wait_for_players_to_load_timeout;
#endif

CON_COMMAND(d2l_cvar, "d2l_cvar <cvar> [value]")
{
	if (args.ArgC() != 2 && args.ArgC() != 3)
	{
		Msg("d2l_cvar <cvar> [value]\n");
		return;
	}

	auto *pVar = g_pCVar->FindVar(args[1]);
	if (!pVar)
	{
		Msg("ConVar \"%s\" not found!\n", args[1]);
		return;
	}

	if (args.ArgC() == 3)
	{
		pVar->SetValue(args[2]);
		Msg("Set \"%s\" value to \"%s\"\n", args[1], args[2]);
	}
	else
	{
		Msg("Value of \"%s\" is \"%s\"\n", args[1], pVar->GetString());
	}
}

static class BaseAccessor : public IConCommandBaseAccessor
{
public:
	bool RegisterConCommandBase(ConCommandBase *pVar)
	{
		return META_REGCVAR(pVar);
	}
} s_BaseAccessor;

CSharedEdictChangeInfo *g_pSharedChangeInfo = nullptr;

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor(IndexOfEdict((const edict_t *)this));
}

static void SetEdictStateChanged(edict_t *pEdict, unsigned short offset)
{
	if (g_pSharedChangeInfo != nullptr)
	{
		if (offset)
		{
			pEdict->StateChanged(offset);
		}
		else
		{
			pEdict->StateChanged();
		}
	}
	else
	{
		pEdict->m_fStateFlags |= FL_EDICT_CHANGED;
	}
}

PLUGIN_EXPOSE(D2Lobby, g_D2Lobby);

DETOUR_DECL_MEMBER3(HookAttemptToBuy, int, char*, classname, int, unk2, int, unk3)
{
	for (int i = 0; i < restrictedItems.Count(); i++)
	{
		if (!Q_stricmp(restrictedItems[i], classname))
		{
			return -1;
		}
	}

	return DETOUR_MEMBER_CALL(HookAttemptToBuy)(classname, unk2, unk3);
}

static void *FindAddress(const char *sig, size_t len)
{
	bool found;
	char *ptr, *end;

	MEMORY_BASIC_INFORMATION mem;

	LPCVOID startAddr = NULL;
	startAddr = g_SMAPI->GetServerFactory(false);

	if (!startAddr)
		return NULL;

	if (!VirtualQuery(startAddr, &mem, sizeof(mem)))
		return NULL;

	IMAGE_DOS_HEADER *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(mem.AllocationBase);
	IMAGE_NT_HEADERS *pe = reinterpret_cast<IMAGE_NT_HEADERS *>((intptr_t)dos + dos->e_lfanew);

	if (pe->Signature != IMAGE_NT_SIGNATURE)
	{
		// GetDllMemInfo failedpe points to a bad location
		return NULL;
	}

	ptr = reinterpret_cast<char *>(mem.AllocationBase);
	end = ptr + pe->OptionalHeader.SizeOfImage - len;

	while (ptr < end)
	{
		found = true;
		for (size_t i = 0; i < len; i++)
		{
			if (sig[i] != '\x2A' && sig[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
			return ptr;

		ptr++;
	}

	return NULL;
}



bool D2Lobby::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	ResetAll();

	PLUGIN_SAVEVARS();

#ifdef D2L_MATCHDATA
	if (!!Q_strcmp(STEAMHTTP_INTERFACE_VERSION, "STEAMHTTP_INTERFACE_VERSION002"))
	{
		Q_strncpy("Plugin compiled against ISteamHTTP version \"" STEAMHTTP_INTERFACE_VERSION "\" instead of \"STEAMHTTP_INTERFACE_VERSION002\"", error, maxlen);
		return false;
	}
#endif

	if (!InitGlobals(error, maxlen))
	{
		ismm->Format(error, maxlen, "Failed to setup globals");
		return false;
	}

	InitHooks();

	for (auto p : PluginSystems())
	{
		if (!p->OnLoad())
			return false;
	}

	if (server)
	{
		eventmgr->AddListener(this, "player_connect", true);
		eventmgr->AddListener(this, "player_disconnect", true);
	}

	eventmgr->AddListener(this, "player_activate", true);
	eventmgr->AddListener(this, "server_pre_shutdown", true);

	const char *pszRestrictedItemList = CommandLine()->ParmValue("-d2itemrestrict", "");
	if (pszRestrictedItemList && pszRestrictedItemList[0])
	{
		V_SplitString(pszRestrictedItemList, ",", restrictedItems);
	}



	//NPC is not near a shop that sells "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x2C\x53\x56\x8B\xD9\x57\x89\x5C\x24\x20\xFF\x75\x0C\xE8\x2A\x2A\x2A\x2A\x89\x44\x24\x10"
	static const char * const s_attemptobuy = "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x34\x53\x56\x57\xFF\x75\x0C\x8B\xF1\x89\x74\x24\x18\xE8\x2A\x2A\x2A\x2A\x8B\xD8\x89\x5C";
	attempttobuyptr = FindAddress(s_attemptobuy, 30);
	printf("attempt %d\n", attempttobuyptr);
	AttemptToBuyDetour = new DETOUR_CREATE_MEMBER_PTR(HookAttemptToBuy, attempttobuyptr);
	AttemptToBuyDetour->EnableDetour();
	return true;
}

bool D2Lobby::InitGlobals(char *error, size_t maxlen)
{
	// For compat with GET_V_IFACE macros
	ISmmAPI *ismm = g_SMAPI;

	gpGlobals = ismm->GetCGlobals();

	GET_V_IFACE_CURRENT(GetServerFactory, gamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, eventmgr, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, plyrmgr, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE_CURRENT(GetServerFactory, serverclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	ICvar *icvar;
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	g_pCVar = icvar;
	ConVar_Register(0, &s_BaseAccessor);

#if defined(WIN32)
	static const char s_IServerFinder [] = "\x55\x8B\xEC\x56\xFF\x2A\x2A\xB9\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B\xF0";
	static const int s_IServerOffs = 8;
	
	server = (IServer *) FindPatchAddress(s_IServerFinder, sizeof(s_IServerFinder) - 1, g_SMAPI->GetEngineFactory(false));
	if (server)
	{
		server = *(IServer **) ((intp) server + s_IServerOffs);
	}
	else
	{
		UTIL_MsgAndLog("Warning: Failed to find IServer. SourceTV kick fix will not be available.\n");
	}
#endif

	g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();

	return true;
}

void D2Lobby::InitHooks()
{
	int h;

	h = SH_ADD_HOOK(IServerGCLobby, SteamIDAllowedToConnect, gamedll->GetServerGCLobby(), SH_MEMBER(this, &D2Lobby::Hook_SteamIDAllowedToConnect), false);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameDLL, GameInit, gamedll, SH_MEMBER(this, &D2Lobby::Hook_GameInitPost), true);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameDLL, GameShutdown, gamedll, SH_MEMBER(this, &D2Lobby::Hook_GameShutdown), false);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameDLL, ServerActivate, gamedll, SH_MEMBER(this, &D2Lobby::Hook_ServerActivate), false);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameDLL, ServerActivate, gamedll, SH_MEMBER(this, &D2Lobby::Hook_ServerActivatePost), true);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameDLL, LevelShutdown, gamedll, SH_MEMBER(this, &D2Lobby::Hook_LevelShutdown), false);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameDLL, GameServerSteamAPIActivated, gamedll, SH_MEMBER(this, &D2Lobby::Hook_GameServerSteamAPIActivatedPost), true);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameDLL, SetServerHibernation, gamedll, SH_MEMBER(this, &D2Lobby::Hook_SetServerHibernationPost), true);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameClients, ClientCommand, serverclients, SH_MEMBER(this, &D2Lobby::Hook_OnClientCommand), false);
	m_GlobalHooks.push_back(h);

	h = SH_ADD_HOOK(IServerGameClients, SetCommandClient, serverclients, SH_MEMBER(this, &D2Lobby::Hook_SetCommandClient), false);
	m_GlobalHooks.push_back(h);

#if defined( D2L_EVENTS )
	h = SH_ADD_HOOK(IVEngineServer, SendUserMessage, engine, SH_MEMBER(this, &D2Lobby::Hook_SendUserMessagePost), true);
	m_GlobalHooks.push_back(h);
#endif

#ifdef D2L_MATCHDATA
	h = SH_ADD_HOOK(IServerGameDLL, Think, gamedll, SH_MEMBER(this, &D2Lobby::Hook_Think), true);
	m_GlobalHooks.push_back(h);
#endif
}

void D2Lobby::ShutdownHooks()
{
	UnhookGC();

	for (int h : m_GlobalHooks)
	{
		SH_REMOVE_HOOK_ID(h);
	}

	m_GlobalHooks.clear();
}

bool D2Lobby::Unload(char *error, size_t maxlen)
{
	eventmgr->RemoveListener(this);

	ShutdownHooks();

	for (auto p : PluginSystems())
	{
		p->OnUnload();
	}

#ifdef D2L_LOGGING
	if (g_pLogFile)
		filesystem->Close(g_pLogFile);
#endif

	return true;
}

void D2Lobby::UnhookGC()
{
#ifdef D2L_MATCHDATA
	if (m_iSendMessageHook != 0)
	{
		SH_REMOVE_HOOK_ID(m_iSendMessageHook);
		m_iSendMessageHook = 0;
	}

	if (m_iRetrieveMessageHook != 0)
	{
		SH_REMOVE_HOOK_ID(m_iRetrieveMessageHook);
		m_iRetrieveMessageHook = 0;
	}

	gamecoordinator = nullptr;
#endif
}

void D2Lobby::FireGameEvent(IGameEvent *event)
{
	if (!strcmp(event->GetName(), "player_connect"))
	{
		Event_PlayerConnect(event);
	}
	else if (!strcmp(event->GetName(), "player_disconnect"))
	{
		Event_PlayerDisconnect(event);
	}
	else if (!strcmp(event->GetName(), "player_activate"))
	{
		Event_PlayerActivate(event);
	}
	else if (!strcmp(event->GetName(), "server_pre_shutdown"))
	{
#ifdef D2L_MATCHDATA
		http = nullptr;
#endif
		UnhookGC();
	}
#ifdef D2L_MATCHDATA
	else if (!strcmp(event->GetName(), "game_rules_state_change"))
	{
		DOTA_GameState newState = g_GameRules.GetState();
		UTIL_LogToFile("Recevied game_rules_change from %s to %s\n", DOTA_GameState_Name(m_LastState).c_str(), DOTA_GameState_Name(newState).c_str());
		if (newState != m_LastState)
		{
			m_LastStateChangeTime = gpGlobals->curtime;
			if (m_LastState == DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS_TO_LOAD)
			{
				if (newState == DOTA_GAMERULES_STATE_DISCONNECT)
				{
					UTIL_LogToFile("Time out waiting for everyone to join, sending match data.\n");
					//Msg("GameRules state changed to %s\n", DOTA_GameState_Name(newState).c_str());
					m_MatchData = json_object();
					json_object_set_new(m_MatchData, "match_id", json_string(m_szMatchId));
					json_object_set_new(m_MatchData, "status", json_string("load_failed"));

					auto *pConnectedPlayers = json_array();
					auto *pFailedPlayers = json_array();
					for (int i = 0; i < kMaxTotalPlayerIds; ++i)
					{
						if (!g_PlayerResource.IsValidPlayer(i))
							continue;

						if (g_PlayerResource.GetPlayerConnectionState(i) == DOTA_CONNECTION_STATE_FAILED)
						{
							json_array_append_new(pFailedPlayers, json_integer(g_PlayerResource.GetPlayerSteamId(i).ConvertToUint64()));
						}
						else
						{
							json_array_append_new(pConnectedPlayers, json_integer(g_PlayerResource.GetPlayerSteamId(i).ConvertToUint64()));
						}
					}

					json_object_set_new(m_MatchData, "connected_players", pConnectedPlayers);
					json_object_set_new(m_MatchData, "failed_players", pFailedPlayers);

					SendMatchData();
					BeginShutdown();
				}
				else if (newState == DOTA_GAMERULES_STATE_HERO_SELECTION && m_szMatchId[0])
				{
					UTIL_LogToFile("Issuing command: tv_record \"replay/%s\"\n", m_szMatchId);
					engine->ServerCommand(CFmtStr("tv_record \"replay/%s\"\n", m_szMatchId));
				}
			}
			else if (m_LastState == DOTA_GAMERULES_STATE_INIT)
			{
				UTIL_LogToFile("Game starting, removing PreGameThink hook\n");
				ChangeMatchState(D2LState::MatchRunning);
			}

			m_LastState = newState;
		}
	}
#endif
}

#pragma region ConCommands

CON_COMMAND(d2lobby_debug, "")
{
	g_D2Lobby.PrintDebug();
}

CON_COMMAND(reset_all, "")
{
	g_D2Lobby.ResetAll();
}

CON_COMMAND(add_radiant_player, "add_radiant_player <steamid64> <name>")
{
	if (args.ArgC() != 3)
	{
		Msg("add_radiant_player <steamid64> <name>\n");
		return;
	}

	uint64 steamid64 = strtoull(args[1], nullptr, 10);
	if (steamid64 == 0)
	{
		Msg("Failed to parse steamid.\n");
		return;
	}

	CSteamID sid(steamid64);
	if (sid.GetEAccountType() != k_EAccountTypeIndividual || sid.GetEUniverse() != k_EUniversePublic)
	{
		Msg("Invalid steamid64 value.\n");
		return;
	}

	if (!g_D2Lobby.AddRadiantPlayer(sid.GetAccountID(), args[2]))
	{
		Msg("Failed to add another player to radiant!\n");
	}
}

CON_COMMAND(add_dire_player, "add_dire_player <steamid64> <name>")
{
	if (args.ArgC() != 3)
	{
		Msg("add_dire_player <steamid64> <name>\n");
		return;
	}

	uint64 steamid64 = strtoull(args[1], nullptr, 10);
	if (steamid64 == 0)
	{
		Msg("Failed to parse steamid.\n");
		return;
	}

	CSteamID sid(steamid64);
	if (sid.GetEAccountType() != k_EAccountTypeIndividual || sid.GetEUniverse() != k_EUniversePublic)
	{
		Msg("Invalid steamid64 value.\n");
		return;
	}

	if (!g_D2Lobby.AddDirePlayer(sid.GetAccountID(), args[2]))
	{
		Msg("Failed to add another player to dire!\n");
	}
}

CON_COMMAND(add_spectator_player, "add_spectator_player <steamid64> [name]")
{
	if (args.ArgC() < 2 || args.ArgC() > 3)
	{
		Msg("add_spectator_player <steamid64> [name]\n");
		return;
	}

	uint64 steamid64 = strtoull(args[1], nullptr, 10);
	if (steamid64 == 0)
	{
		Msg("Failed to parse steamid.\n");
		return;
	}

	CSteamID sid(steamid64);
	if (sid.GetEAccountType() != k_EAccountTypeIndividual || sid.GetEUniverse() != k_EUniversePublic)
	{
		Msg("Invalid steamid64 value.\n");
		return;
	}

	if (!g_D2Lobby.AddSpectatorPlayer(sid.GetAccountID(), args.ArgC() == 3 ? args[2] : "Spectator"))
	{
		Msg("Failed to add another spectator player!\n");
	}
}

CON_COMMAND(add_broadcast_channel, "add_broadcast_channel <countrycode> <description> <steamid64> <name> [steamid64] [name] [steamid64] [name] [steamid64] [name]")
{
	if (args.ArgC() != 5 && args.ArgC() != 7  && args.ArgC() != 9 && args.ArgC() != 11)
	{
		Msg("add_broadcast_channel <countrycode> <description> <steamid64> <name> [steamid64] [name] [steamid64] [name] [steamid64] [name]\n");
		return;
	}

	const char *pCountry = args[1];
	if (strlen(pCountry) != 2)
	{
		Msg("countrycode must be two characters.\n");
		return;
	}

	const char *pDesc = args[2];
	CSteamID caster1(strtoull(args[3], nullptr, 10));
	CSteamID caster2(args.ArgC() > 5 ? strtoull(args[5], nullptr, 10) : 0);
	CSteamID caster3(args.ArgC() > 7 ? strtoull(args[7], nullptr, 10) : 0);
	CSteamID caster4(args.ArgC() > 9 ? strtoull(args[9], nullptr, 10) : 0);
	if (!g_D2Lobby.AddBroadcastChannel(pCountry, pDesc,
		caster1.GetAccountID(), args[4],
		caster2.GetAccountID(), args.ArgC() > 6 ? args[6] : nullptr,
		caster3.GetAccountID(), args.ArgC() > 8 ? args[8] : nullptr,
		caster4.GetAccountID(), args.ArgC() > 10 ? args[10] : nullptr
		))
	{
		Msg("Unable to add more broadcast channels.\n");
	}
}

CON_COMMAND(set_series_data, "set_series_data <bo3|bo5> <radiantWins> <direWins>")
{
	if (args.ArgC() != 4)
	{
		Msg("set_series_data <bo3|bo5> <radiantWins> <direWins>\n");
		return;
	}

	SeriesType type;
	if (!Q_strcmp(args[1], "bo3"))
	{
		type = SeriesType::BO3;
	}
	else if (!Q_strcmp(args[1], "bo5"))
	{
		type = SeriesType::BO5;
	}
	else
	{
		Msg("set_series_data <bo3|bo5> <radiantWins> <direWins>\n");
		return;
	}

	g_D2Lobby.SetSeriesData(type, atoi(args[2]), atoi(args[3]));
}

CON_COMMAND(d2l_message_all, "d2l_message_all <message>")
{
	if (args.ArgC() < 2)
	{
		Msg("d2l_message_all <message>\n");
		return;
	}

	FilterEveryone filter;
	CUserMsg_TextMsg msg;
	msg.add_param(args[1]);
	msg.set_dest(HUD_PRINTTALK);

	SH_CALL(engine, &IVEngineServer::SendUserMessage)(filter, UM_TextMsg, msg);
}

CON_COMMAND(d2l_message_radiant, "d2l_message_radiant <message>")
{
	if (args.ArgC() < 2)
	{
		Msg("d2l_message_radiant <message>\n");
		return;
	}

	FilterTeam filter(kTeamRadiant);
	CUserMsg_TextMsg msg;
	msg.add_param(args[1]);
	msg.set_dest(HUD_PRINTTALK);

	SH_CALL(engine, &IVEngineServer::SendUserMessage)(filter, UM_TextMsg, msg);
}

CON_COMMAND(d2l_message_dire, "d2l_message_dire <message>")
{
	if (args.ArgC() < 2)
	{
		Msg("d2l_message_dire <message>\n");
		return;
	}

	FilterTeam filter(kTeamDire);
	CUserMsg_TextMsg msg;
	msg.add_param(args[1]);
	msg.set_dest(HUD_PRINTTALK);

	SH_CALL(engine, &IVEngineServer::SendUserMessage)(filter, UM_TextMsg, msg);
}

#ifdef D2L_MATCHDATA
CON_COMMAND(get_match_id, "get_match_id")
{
	json_t *pRes = json_object();
	json_object_set_new(pRes, "match_id", json_string(g_D2Lobby.GetMatchId()));
	
	char *pszOutput = json_dumps(pRes, JSON_COMPACT);
	Msg("%s", pszOutput);
	free(pszOutput);
	json_decref(pRes);
}

CON_COMMAND(set_match_id, "set_match_id <id>")
{
	if (args.ArgC() != 2)
	{
		Msg("set_match_id <id>\n");
		return;
	}

	g_D2Lobby.SetMatchId(args[1]);

#ifdef D2L_LOGGING
	if (g_pLogFile)
	{
		filesystem->Close(g_pLogFile);
	}

	if (!filesystem->IsDirectory("d2lobby_logs"))
		filesystem->CreateDirHierarchy("d2lobby_logs");

	g_pLogFile = filesystem->Open(CFmtStr("d2lobby_logs/%s.txt", args[1]), "a");
#endif
}
#endif

#pragma endregion

#pragma region Command Impls

bool D2Lobby::AddRadiantPlayer(uint32 accountId, const char *pszName)
{
	if (m_RadiantPlayers.size() >= kMaxTeamPlayers)
		return false;

	for (auto p : m_RadiantPlayers)
	{
		if (p->AccountId == accountId)
		{
			Msg("Player already added!.\n");
			return false;
		}
	}

	auto *plyr = new Player;
	plyr->AccountId = accountId;
	Q_strncpy(plyr->name, pszName, kMaxPlayerNameLength);

	m_RadiantPlayers.push_back(plyr);

	return true;
}

bool D2Lobby::AddDirePlayer(uint32 accountId, const char *pszName)
{
	if (m_DirePlayers.size() >= kMaxTeamPlayers)
		return false;

	for (auto p : m_DirePlayers)
	{
		if (p->AccountId == accountId)
		{
			Msg("Player already added!.\n");
			return false;
		}
	}

	auto *plyr = new Player;
	plyr->AccountId = accountId;
	Q_strncpy(plyr->name, pszName, kMaxPlayerNameLength);

	m_DirePlayers.push_back(plyr);

	return true;
}

bool D2Lobby::AddSpectatorPlayer(uint32 accountId, const char *pszName)
{
	auto *plyr = new Player;
	plyr->AccountId = accountId;
	Q_strncpy(plyr->name, pszName, kMaxPlayerNameLength);

	for (auto p : m_SpectatorPlayers)
	{
		if (p->AccountId == accountId)
		{
			Msg("Player already added!.\n");
			return false;
		}
	}

	m_SpectatorPlayers.push_back(plyr);

	return true;
}

bool D2Lobby::AddBroadcastChannel(const char *pszCountryCode, const char *pszDesc,
	uint32 caster1, const char *pszName1,
	uint32 caster2, const char *pszName2,
	uint32 caster3, const char *pszName3,
	uint32 caster4, const char *pszName4)
{
	if (m_iBroadcastChannelCnt >= kMaxBroadcastChannels)
		return false;

	auto pChannel = &m_BroadcastChannels[m_iBroadcastChannelCnt];

	Q_strncpy(pChannel->CountryCode, pszCountryCode, sizeof(pChannel->CountryCode));
	Q_strncpy(pChannel->Description, pszDesc, sizeof(pChannel->Description));

	pChannel->Casters[0].AccountId = caster1;
	Q_strncpy(pChannel->Casters[0].name, pszName1, sizeof(pChannel->Casters[0].name));

	pChannel->CasterCount = 1;

	if (caster2)
	{
		pChannel->Casters[1].AccountId = caster1;
		Q_strncpy(pChannel->Casters[1].name, pszName1, sizeof(pChannel->Casters[1].name));
		++pChannel->CasterCount;

		if (caster3)
		{
			pChannel->Casters[2].AccountId = caster1;
			Q_strncpy(pChannel->Casters[2].name, pszName1, sizeof(pChannel->Casters[2].name));
			++pChannel->CasterCount;

			if (caster4)
			{
				pChannel->Casters[3].AccountId = caster1;
				Q_strncpy(pChannel->Casters[3].name, pszName1, sizeof(pChannel->Casters[3].name));
				++pChannel->CasterCount;
			}
		}
	}

	++m_iBroadcastChannelCnt;

	return true;
}

void D2Lobby::SetSeriesData(SeriesType type, uint8 radiantWins, uint8 direWins)
{
	m_SeriesData.Type = type;
	m_SeriesData.RadiantWins = radiantWins;
	m_SeriesData.DireWins = direWins;
}

#ifdef D2L_MATCHDATA
void D2Lobby::SetMatchId(const char *pszId)
{
	Q_strncpy(m_szMatchId, pszId, sizeof(m_szMatchId));
}
#endif

void D2Lobby::ResetAll()
{
#ifdef D2L_MATCHDATA
	m_szMatchId[0] = '\0';
#endif
	for (auto *plyr : m_RadiantPlayers)
	{
		delete plyr;
	}

	m_RadiantPlayers.clear();

	for (auto *plyr : m_DirePlayers)
	{
		delete plyr;
	}

	m_DirePlayers.clear();

	for (auto *plyr : m_SpectatorPlayers)
	{
		delete plyr;
	}

	m_SpectatorPlayers.clear();

	m_iBroadcastChannelCnt = 0;
	m_SeriesData.Type = SeriesType::None;
}

void D2Lobby::PrintDebug()
{
	Msg("Radiant players:\n");
	for (auto *plyr : m_RadiantPlayers)
	{
		Msg("- %llu [U:1:%u]\t(%s)\n", CSteamID(plyr->AccountId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64(), plyr->AccountId, plyr->name);
	}

	Msg("Dire players:\n");
	for (auto *plyr : m_DirePlayers)
	{
		Msg("- %llu [U:1:%u]\t(%s)\n", CSteamID(plyr->AccountId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64(), plyr->AccountId, plyr->name);
	}

	Msg("Spectator players:\n");
	for (auto *plyr : m_SpectatorPlayers)
	{
		Msg("- %llu [U:1:%u]\t(%s)\n", CSteamID(plyr->AccountId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64(), plyr->AccountId, plyr->name);
	}

	for (int i = 0; i < m_iBroadcastChannelCnt; ++i)
	{
		Msg("Broadcast Channel %d (%s) - %s\n", i, m_BroadcastChannels[i].CountryCode, m_BroadcastChannels[i].Description);
		for (int j = 0; j < m_BroadcastChannels[i].CasterCount; ++j)
		{
			Msg("- %llu [U:1:%u]\t(%s)\n", CSteamID(m_BroadcastChannels[i].Casters[j].AccountId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64(), m_BroadcastChannels[i].Casters[j].AccountId, m_BroadcastChannels[i].Casters[j].name);
		}
	}

#ifdef D2L_MATCHDATA
	Msg("Match ID: \"%s\"\n", m_szMatchId);
	Msg("Match POST Url: \"%s\"\n", match_post_url.GetString());
#endif
}

#pragma endregion

void D2Lobby::Hook_ServerActivate()
{
	if (CommandLine()->HasParm("-d2lnorunes"))
	{
		// Remove the name on rune spawn markers so that GameRules cannot find them.
		for (auto *pEnt = servertools->FirstEntity(); pEnt; pEnt = servertools->NextEntity(pEnt))
		{
			static char szName[64];
			if (servertools->GetKeyValue(pEnt, "targetname", szName, sizeof(szName))
				&& !Q_strcmp(szName, "dota_item_rune_spawner"))
			{
				servertools->SetKeyValue(pEnt, "targetname", nullptr);
			}
		}
	}
}

void D2Lobby::Hook_ServerActivatePost()
{
	for (auto sys : PluginSystems())
	{
		sys->OnServerActivated();
	}

	PopulatePlayerResourceData();

	if (m_SeriesData.Type != SeriesType::None)
	{
		g_GameRules.SetSeriesData(m_SeriesData.Type, m_SeriesData.RadiantWins, m_SeriesData.DireWins);
	}
}

void D2Lobby::Hook_LevelShutdown()
{
	for (auto sys : PluginSystems())
	{
		sys->OnLevelShutdown();
	}
}

void D2Lobby::PopulatePlayerResourceData()
{
	int plyrId = 0;

	for (auto *plyr : m_RadiantPlayers)
	{
		g_PlayerResource.InitPlayer(plyrId, plyr->AccountId, plyr->name, kTeamRadiant);
		++plyrId;
	}

	for (auto *plyr : m_DirePlayers)
	{
		g_PlayerResource.InitPlayer(plyrId, plyr->AccountId, plyr->name, kTeamDire);
		++plyrId;
	}

	plyrId = kSpectatorIdStart;

	for (auto *plyr : m_SpectatorPlayers)
	{
		g_PlayerResource.InitPlayer(plyrId, plyr->AccountId, plyr->name, kTeamSpectators);
		++plyrId;
	}

	for (int i = 0; i < m_iBroadcastChannelCnt; ++i)
	{
		g_PlayerResource.InitBroadcastChannel(i, m_BroadcastChannels[i].CountryCode, m_BroadcastChannels[i].Description);

		for (int s = 0; s < m_BroadcastChannels[i].CasterCount; ++s, ++plyrId)
		{
			auto caster = &m_BroadcastChannels[i].Casters[s];
			g_PlayerResource.InitPlayer(plyrId, caster->AccountId, caster->name, kTeamSpectators, i, s);
		}
	}

	// We've changed a ton on here; just mark the whole ent as changed. (Even though no one has connected yet)
	g_PlayerResource.edict()->StateChanged();

	SetWaitForPlayersCount();
}

void D2Lobby::Hook_OnBotDisconnect(int reason)
{
	auto *pClient = META_IFACEPTR(IClient);
	// HLTV bot disconnects at hibernation time with a reason of "".
	// All other /normal/ disconnect reasons for it will be NULL or have text.
	if (pClient->IsHLTV())
	{
		if (reason == 0)
		{
			RETURN_META(MRES_SUPERCEDE);
		}
	}
	
	RETURN_META(MRES_IGNORED);
}

void D2Lobby::Event_PlayerConnect(IGameEvent *pEvent)
{
	int slot = pEvent->GetInt("index");
	auto *pClient = server->GetClient(slot);
	
	// This is too early for IsHLTV to return true, so hook all bots and check in callback.
	if (!strcmp(pEvent->GetString("networkid"), "BOT"))
	{
		SH_ADD_HOOK(IClient, Disconnect, pClient, SH_MEMBER(this, &D2Lobby::Hook_OnBotDisconnect), false);
	}
}

void D2Lobby::Event_PlayerDisconnect(IGameEvent *pEvent)
{
	int slot = pEvent->GetInt("index");
	auto *pClient = server->GetClient(slot);
	
	if (!strcmp(pEvent->GetString("networkid"), "BOT"))
	{
		SH_REMOVE_HOOK(IClient, Disconnect, pClient, SH_MEMBER(this, &D2Lobby::Hook_OnBotDisconnect), false);
	}
}

void D2Lobby::Event_PlayerActivate(IGameEvent *pEvent)
{
	int userid = pEvent->GetInt("userid");
	if (userid == 0)
		return;

	int entIdx = UserIdToEntIndex(userid);
	if (entIdx <= 0)
		return;

	const CSteamID *pSteamID = engine->GetClientSteamID(entIdx);
	if (pSteamID == nullptr)
		return;

	int team = GetPlayerTeam(pSteamID->GetAccountID());
	if (team == kTeamUnassigned)
		return;

	auto *pInfo = plyrmgr->GetPlayerInfo(PEntityOfEntIndex(entIdx));
	if (!pInfo)
		return;

	pInfo->ChangeTeam(team);
}

int D2Lobby::GetPlayerTeam(uint32 accountId) const
{
	for (auto *plyr : m_RadiantPlayers)
	{
		if (plyr->AccountId == accountId)
		{
			return kTeamRadiant;
		}
	}

	for (auto *plyr : m_DirePlayers)
	{
		if (plyr->AccountId == accountId)
		{
			return kTeamDire;
		}
	}

	for (auto *plyr : m_SpectatorPlayers)
	{
		if (plyr->AccountId == accountId)
		{
			return kTeamSpectators;
		}
	}

	for (int i = 0; i < m_iBroadcastChannelCnt; ++i)
	{
		for (int s = 0; s < m_BroadcastChannels[i].CasterCount; ++s)
		{
			if (m_BroadcastChannels[i].Casters[s].AccountId == accountId)
			{
				return kTeamSpectators;
			}
		}
	}

	return kTeamUnassigned;
}

bool D2Lobby::Hook_SteamIDAllowedToConnect(const CSteamID &steamId) const
{
	if (g_GameRules.GetState() >= DOTA_GAMERULES_STATE_POST_GAME || GetPlayerTeam(steamId.GetAccountID()) == kTeamUnassigned)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, false);
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, true);	
}

bool D2Lobby::Hook_GameInitPost()
{
#ifdef D2L_MATCHDATA
	eventmgr->AddListener(this, "game_rules_state_change", true);

	dota_wait_for_players_to_load_timeout = g_pCVar->FindVar("dota_wait_for_players_to_load_timeout");
	m_flStartupTime = gpGlobals->realtime;
	ChangeMatchState(D2LState::PreWaitingForPlayers);

	if (m_MatchData != nullptr)
	{
		json_decref(m_MatchData);
	}
	m_MatchData = nullptr;
#endif

	int h;
	h = SH_ADD_HOOK(ConCommand, Dispatch, g_pCVar->FindCommand("say"), SH_MEMBER(this, &D2Lobby::Hook_SayDispatch), false);
	m_GameHooks.push_back(h);

	h = SH_ADD_HOOK(ConCommand, Dispatch, g_pCVar->FindCommand("say_team"), SH_MEMBER(this, &D2Lobby::Hook_SayDispatch), false);
	m_GameHooks.push_back(h);

#ifdef D2L_EVENTS
	h = SH_ADD_HOOK(ConCommand, Dispatch, g_pCVar->FindCommand("dota_cancel_GG"), SH_MEMBER(this, &D2Lobby::Hook_CancelGGDispatch), false);
	m_GameHooks.push_back(h);
#endif

#ifdef D2L_MATCHDATA
	engine->ServerCommand("d2f_blockgc_server_command 0\n");
#endif

	engine->ServerCommand("d2f_allow_all 0\n");

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void D2Lobby::Hook_GameShutdown()
{
	for (int h : m_GameHooks)
	{
		SH_REMOVE_HOOK_ID(h);
	}

	m_GameHooks.clear();
}

#ifdef D2L_MATCHDATA
EGCResults D2Lobby::Hook_SendMessage(uint32 unMsgType, const void *pubData, uint32 cubData)
{
	int realMsg = unMsgType & ~0x80000000;

	switch (realMsg)
	{
	case k_EMsgGCGameMatchSignOut:
	{
		UTIL_LogToFile("Intercepted outgoing k_EMsgGCGameMatchSignOut\n");
		if (m_State == D2LState::MatchRunning)
		{
			UTIL_LogToFile("Building match end data\n");

			int headerSize = *(int32 *) ((intp) pubData + 4);
			int skip = 4 /*emsg*/ + 4 /*hsizesize*/ + headerSize;
			void *start = (void *) ((intp) pubData + skip);
			int size = cubData - skip;

			CMsgGameMatchSignOut msg;
			msg.ParseFromArray(start, size);

			assert(m_MatchData == nullptr);
			m_MatchData = parse_msg(&msg);
			json_object_set_new(m_MatchData, "match_id", json_string(m_szMatchId));
			json_object_set_new(m_MatchData, "status", json_string("completed"));

			// 5 minute timeout.
			m_flShutdownTime = gpGlobals->curtime;
			ChangeMatchState(D2LState::MatchOver);

			SendMatchData();
		}
		RETURN_META_VALUE(MRES_SUPERCEDE, k_EGCResultOK);
	}
	}

	RETURN_META_VALUE(MRES_IGNORED, k_EGCResultOK);
}

EGCResults D2Lobby::Hook_RetrieveMessage(uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize)
{
	EGCResults ret = SH_CALL(gamecoordinator, &ISteamGameCoordinator::RetrieveMessage)(punMsgType, pubDest, cubDest, pcubMsgSize);

	int realMsg = (*punMsgType) & ~0x80000000;

	switch (realMsg)
	{
	case k_EMsgGCGCToRelayConnect:
	case k_EMsgGCToServerConsoleCommand:
		RETURN_META_VALUE(MRES_SUPERCEDE, k_EGCResultNoMessage);
	case k_EMsgGCGameMatchSignOutPermissionResponse:
	{
		UTIL_LogToFile("Intercepted incoming k_EMsgGCGameMatchSignOutPermissionResponse\n");
		uint32 skip = 4 /*emsg*/ + 4 /*hsizesize*/ + *(int32 *) ((intp) pubDest + 4) /*hsize*/;
		void *start = (void *) ((intp) pubDest + skip);
		uint32 size = *pcubMsgSize - skip;

		CMsgGameMatchSignOutPermissionResponse msg;
		if (msg.ParseFromArray(start, size))
		{
			msg.set_permission_granted(true);
			msg.clear_retry_delay_seconds();
			uint32 newMsgSize = (uint32) msg.ByteSize();
			if (newMsgSize > (cubDest - skip))
			{
				RETURN_META_VALUE(MRES_SUPERCEDE, k_EGCResultBufferTooSmall);
			}

			msg.SerializeToArray(start, newMsgSize);
			*pcubMsgSize = newMsgSize + skip;
			RETURN_META_VALUE(MRES_SUPERCEDE, k_EGCResultOK);
		}
		else
		{
			UTIL_MsgAndLog("Failed to parse SignOutPermissionResponse\n");
		}
	}
	break;
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, ret);
}
#endif

#ifdef D2L_EVENTS

void D2Lobby::Hook_CancelGGDispatch(const CCommandContext &context, const CCommand &command)
{
	int playerId = g_PlayerResource.PlayerIdFromEntIndex(m_iCmdClientEntIdx);
	if (playerId < 0 || playerId >= kMaxGamePlayerIds)
	{
		if (g_GameRules.GetState() == DOTA_GAMERULES_STATE_GAME_IN_PROGRESS)
		{
			int ggTeam = g_GameRules.GetGGTeam();
			int plyrTeam = g_PlayerResource.GetPlayerTeam(playerId);
			if (ggTeam == plyrTeam)
			{
				g_EventLogger.LogGGCancel(g_PlayerResource.GetPlayerSteamId(playerId).ConvertToUint64(), plyrTeam);
			}
		}
	}

	RETURN_META(MRES_IGNORED);
}
#endif

void D2Lobby::Hook_SayDispatch(const CCommandContext &context, const CCommand &command)
{
	int playerId = g_PlayerResource.PlayerIdFromEntIndex(m_iCmdClientEntIdx);
	if (playerId < 0 || playerId >= kMaxGamePlayerIds)
	{
		RETURN_META(MRES_IGNORED);
	}

	if (!Q_stricmp(command.ArgS(), "\"gg\""))
	{
		if (g_GameRules.GetState() == DOTA_GAMERULES_STATE_GAME_IN_PROGRESS)
		{
			int team = g_PlayerResource.GetPlayerTeam(playerId);
			if (team == kTeamRadiant || team == kTeamDire)
			{
#ifdef D2L_EVENTS
				g_EventLogger.LogGGCall(g_PlayerResource.GetPlayerSteamId(playerId).ConvertToUint64(), team);
#endif
				g_GameRules.CallGG(team);
			}
		}
	}

	RETURN_META(MRES_IGNORED);
}

#ifdef D2L_MATCHDATA
ConVar d2lobby_max_team_disconnect_time("d2lobby_max_team_disconnect_time", "30", FCVAR_RELEASE,
	"Maximum amount of time in seconds that a whole team can be disconnected before automatic forfeit.");

void D2Lobby::Hook_Think(bool bFinalTick)
{
	switch (m_State)
	{
	case D2LState::PreWaitingForPlayers:
		if (gpGlobals->realtime - m_flStartupTime > dota_wait_for_players_to_load_timeout->GetFloat())
		{
			UTIL_LogToFile("PreGameThink: Timed out waiting for anyone to join, sending match data.\n");
			m_MatchData = json_object();
			json_object_set_new(m_MatchData, "match_id", json_string(m_szMatchId));
			json_object_set_new(m_MatchData, "status", json_string("load_failed"));
			json_object_set_new(m_MatchData, "connected_players", json_array());

			auto *pFailedPlayers = json_array();
			for (int i = 0; i < kMaxTotalPlayerIds; ++i)
			{
				if (!g_PlayerResource.IsValidPlayer(i))
					continue;

				json_array_append_new(pFailedPlayers, json_integer(g_PlayerResource.GetPlayerSteamId(i).ConvertToUint64()));
			}
			json_object_set_new(m_MatchData, "failed_players", pFailedPlayers);

			SendMatchData();
			BeginShutdown();
		}
		break;
	case D2LState::MatchRunning:
		{
			DOTA_GameState state = g_GameRules.GetState();
			if (state > DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS_TO_LOAD && state < DOTA_GAMERULES_STATE_POST_GAME && g_GameRules.GetGGTeam() == kTeamUnassigned)
			{
				int radiantCount = g_PlayerResource.GetTeamConnectedCount(kTeamRadiant);
				if (radiantCount == 0 && m_RadiantPlayers.size() > 0)
				{
					float flGameTime = g_GameRules.GetGameTime();
					if (m_flRadiantDisconnectTime == 0.f)
					{
						UTIL_LogToFile("No Radiant players detected. Radiant forfeit in %.2f seconds.\n", d2lobby_max_team_disconnect_time.GetFloat());
						m_flRadiantDisconnectTime = flGameTime + d2lobby_max_team_disconnect_time.GetFloat();
					}
					else if (flGameTime >= m_flRadiantDisconnectTime)
					{
						g_GameRules.ForceGameEnd(kTeamRadiant);
						m_flShutdownTime = gpGlobals->curtime;
					}
				}
				else if (m_flRadiantDisconnectTime != 0.f)
				{
					UTIL_LogToFile("Resetting Radiant forfeit time.\n");
					m_flRadiantDisconnectTime = 0.f;
				}

				int direCount = g_PlayerResource.GetTeamConnectedCount(kTeamDire);
				if (direCount == 0 && m_DirePlayers.size() > 0)
				{
					float flGameTime = g_GameRules.GetGameTime();
					if (m_flDireDisconnectTime == 0.f)
					{
						UTIL_LogToFile("No Dire players detected. Dire forfeit in %.2f seconds.\n", d2lobby_max_team_disconnect_time.GetFloat());
						m_flDireDisconnectTime = flGameTime + d2lobby_max_team_disconnect_time.GetFloat();
					}
					else if (flGameTime >= m_flDireDisconnectTime)
					{
						g_GameRules.ForceGameEnd(kTeamDire);
						m_flShutdownTime = gpGlobals->curtime;
					}
				}
				else if (m_flDireDisconnectTime != 0.f)
				{
					UTIL_LogToFile("Resetting Dire forfeit time.\n");
					m_flDireDisconnectTime = 0.f;
				}

				// Account for game being paused with everyone gone.
				int totalCount = radiantCount + direCount;
				if (totalCount == 0)
				{
					if (m_flAllDisconnectTime == 0.f)
					{
						UTIL_LogToFile("No players detected. Game ending in %.2f seconds.\n", d2lobby_max_team_disconnect_time.GetFloat() + 5.f);
						m_flAllDisconnectTime = gpGlobals->curtime + d2lobby_max_team_disconnect_time.GetFloat() + 5.f;
					}
					else if (gpGlobals->curtime >= m_flAllDisconnectTime)
					{
						g_GameRules.ForceGameEnd(kTeamDire);
						m_flShutdownTime = gpGlobals->curtime;
					}
				}
				else if (m_flAllDisconnectTime != 0.f)
				{
					UTIL_LogToFile("Resetting no players game end time.\n");
				}
			}
		}
		break;
	case D2LState::MatchOver:
		{
#ifdef D2L_EVENTS
		if (g_HTTPManager.HasAnyPendingRequests())
			return;
#endif
		static ConVarRef tv_delay("tv_delay");

		// Don't stop before delay + 3m, even if all wrapped up
		float flMinShutdownTime = m_flShutdownTime + (3.f * 60.f) + tv_delay.GetFloat();
		float flMaxShutdownTime = m_flShutdownTime + (5.f * 60.f) + tv_delay.GetFloat();
		if (gpGlobals->curtime > flMaxShutdownTime)
		{
			UTIL_LogToFile("PostGameThink: Reached shutdown time, sending match data.\n");
			BeginShutdown();
			return;
		}

		if (gpGlobals->curtime > flMinShutdownTime)
		{
			BeginShutdown();
			return;
		}

		int clientCount = 0;
		int maxClients = gpGlobals->maxClients;
		for (int i = 1; i <= maxClients; ++i)
		{
			edict_t *pEdict = PEntityOfEntIndex(i);
			if (!pEdict || pEdict->IsFree())
				continue;

			auto *pInfo = plyrmgr->GetPlayerInfo(pEdict);
			if (!pInfo || !pInfo->IsConnected() || pInfo->IsFakeClient() || pInfo->IsHLTV())
				continue;

			++clientCount;
		}

		if (clientCount == 0 && gpGlobals->curtime > (m_flShutdownTime + tv_delay.GetFloat()))
		{
			UTIL_LogToFile("PostGameThink: No players are still connected, shutting down.\n");
			BeginShutdown();
		}
		}
		break;
	case D2LState::ShuttingDown:
		if (g_HTTPManager.HasAnyPendingRequests())
			return;

		engine->ServerCommand("quit\n");
		break;
	}
}

void D2Lobby::SendMatchData()
{
	char *pszOutput = json_dumps(m_MatchData, JSON_COMPACT);

	if (match_post_url.GetString()[0])
	{
		g_HTTPManager.PostJSONToMatchUrl(pszOutput);
	}
	else
	{
		FILE *f = fopen(CFmtStr("match_%s.txt", m_szMatchId), "w");
		fprintf(f, "%s", pszOutput);
		fclose(f);
	}

	json_decref(m_MatchData);

	free(pszOutput);
}
#endif

void D2Lobby::BeginShutdown()
{
	ChangeMatchState(D2LState::ShuttingDown);

	json_t *pContainer = json_object();
	json_object_set_new(pContainer, "match_id", json_string(g_D2Lobby.GetMatchId()));
	json_object_set_new(pContainer, "status", json_string("shutdown"));

	char *pszOutput = json_dumps(pContainer, JSON_COMPACT);
	json_decref(pContainer);

	if (match_post_url.GetString()[0])
	{
		g_HTTPManager.PostJSONToMatchUrl(pszOutput);
	}

	free(pszOutput);
}

void D2Lobby::Hook_GameServerSteamAPIActivatedPost()
{
#ifdef D2L_MATCHDATA
	HSteamUser hSteamUser = SteamGameServer_GetHSteamUser();
	HSteamPipe hSteamPipe = SteamGameServer_GetHSteamPipe();

	http = (ISteamHTTP *) g_pSteamClientGameServer->GetISteamGenericInterface(hSteamUser, hSteamPipe, STEAMHTTP_INTERFACE_VERSION);
	UTIL_MsgAndLog("Found ISteamHTTP at %p.\n", http);

	gamecoordinator = (ISteamGameCoordinator *) g_pSteamClientGameServer->GetISteamGenericInterface(hSteamUser, hSteamPipe, STEAMGAMECOORDINATOR_INTERFACE_VERSION);
	UTIL_MsgAndLog("Found ISteamGameCoordinator at %p.\n", gamecoordinator);

	m_iRetrieveMessageHook = SH_ADD_HOOK(ISteamGameCoordinator, RetrieveMessage, gamecoordinator, SH_MEMBER(this, &D2Lobby::Hook_RetrieveMessage), false);
	m_iSendMessageHook = SH_ADD_HOOK(ISteamGameCoordinator, SendMessage, gamecoordinator, SH_MEMBER(this, &D2Lobby::Hook_SendMessage), false);
#endif

	RETURN_META(MRES_IGNORED);
}

void D2Lobby::SetWaitForPlayersCount()
{
	int playerCount = m_RadiantPlayers.size() + m_DirePlayers.size() + m_SpectatorPlayers.size();
	for (int i = 0; i < m_iBroadcastChannelCnt; ++i)
	{
		playerCount += m_BroadcastChannels[i].CasterCount;
	}

	// If this reaches 0 at any time during INIT, the game will state immediately when first person connects
	if (playerCount == 0)
		++playerCount;

	engine->ServerCommand(CFmtStrN<64>("dota_wait_for_players_to_load_count %d\n", playerCount));
}

void D2Lobby::Hook_OnClientCommand(CEntityIndex ent, const CCommand &args)
{
	META_RES res = MRES_IGNORED;
	for (auto p : PluginSystems())
	{
		META_RES tmp = p->OnClientCommand(ent, args);
		if (tmp > res)
			res = tmp;
	}

	RETURN_META(res);
}

void D2Lobby::Hook_SetServerHibernationPost(bool bHibernating)
{
	if (bHibernating)
	{
		if (match_post_url.GetString()[0])
		{
			auto *pMsg = json_object();
			json_object_set_new(pMsg, "match_id", json_string(m_szMatchId));
			json_object_set_new(pMsg, "status", json_string("hibernating"));

			char *pszOutput = json_dumps(pMsg, JSON_COMPACT);
			g_HTTPManager.PostJSONToMatchUrl(pszOutput);

			free(pszOutput);
			json_decref(pMsg);
		}

		SetWaitForPlayersCount();
	}
	else
	{
		// No longer hibernating
		PopulatePlayerResourceData();

	}
}

#if defined( D2L_EVENTS )
void D2Lobby::Hook_SendUserMessagePost(IRecipientFilter &filter, int message, const google::protobuf::Message &msg)
{
	for (auto sys : PluginSystems())
	{
		sys->OnUserMessage(filter, message, msg);
	}
}
#endif

void D2Lobby::ChangeMatchState(D2LState newState)
{
	UTIL_LogToFile("Changing Match state from %d to %d\n", m_State, newState);
	m_State = newState;
}



const char *D2Lobby::GetLicense()
{
	return "Private";
}

const char *D2Lobby::GetVersion()
{
	return "2.0.3";
}

const char *D2Lobby::GetDate()
{
	return __DATE__;
}

const char *D2Lobby::GetLogTag()
{
	return "D2LOBBY";
}

const char *D2Lobby::GetAuthor()
{
	return "Nicholas Hastings";
}

const char *D2Lobby::GetDescription()
{
	return "";
}

const char *D2Lobby::GetName()
{
	return "Dota 2 Lobby";
}

const char *D2Lobby::GetURL()
{
	return "";
}
