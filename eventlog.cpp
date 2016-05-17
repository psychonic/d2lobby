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

#include "eventlog.h"

#include "d2lobby.h"
#include "httpmgr.h"
#include "playerresourcehelper.h"
#include "util.h"

#include <filesystem.h>
#include <fmtstr.h>
#include <toolframework/itoolentity.h>

#include <irecipientfilter.h>

#include <steam_gameserver.h>

#include "protobuf/dota_usermessages.pb.h"

EventLogger g_EventLogger;

void EventLogger::SendAndFreeEvent(json_t *pData)
{
#pragma message ("Remember to queue events or something if ISteamHTTP isn't available yet")
	if (!http)
	{
		UTIL_LogToFile("Can't send event, ISteamHTTP not available!!!!!!\n");
		return;
	}

	json_t *pContainer = json_object();
	json_t *pEvents = json_array();
	json_array_append_new(pEvents, pData);
	json_object_set_new(pContainer, "match_id", json_string(g_D2Lobby.GetMatchId()));
	json_object_set_new(pContainer, "events", pEvents);
	json_object_set_new(pContainer, "status", json_string("events"));
	json_object_set_new(pContainer, "has_events", json_boolean(true));

	char *pszOutput = json_dumps(pContainer, JSON_COMPACT);
	json_decref(pContainer);

	if (match_post_url.GetString()[0])
	{
		g_HTTPManager.PostJSONToMatchUrl(pszOutput);
	}

	free(pszOutput);
}

json_t *EventLogger::CreateTimedEvent(EventType type)
{
	json_t *pEvent = json_object();
	json_object_set_new(pEvent, "time", json_real(g_GameRules.GetGameTime()));
	json_object_set_new(pEvent, "event_type", json_integer((int)type));

	return pEvent;
}

void EventLogger::OnServerActivated()
{
	eventmgr->AddListener(this, "player_connect", true);
	eventmgr->AddListener(this, "player_disconnect", true);
	eventmgr->AddListener(this, "player_team", true);
	eventmgr->AddListener(this, "game_rules_state_change", true);
	eventmgr->AddListener(this, "entity_killed", true);
}

void EventLogger::OnLevelShutdown()
{
	eventmgr->RemoveListener(this);
}

void EventLogger::FireGameEvent(IGameEvent *pEvent)
{
	const char *pszEventName = pEvent->GetName();
	if (!strcmp(pszEventName, "game_rules_state_change"))
	{
		LogGameStateChange(g_GameRules.GetState());
	}
	else if (!strcmp(pszEventName, "player_connect"))
	{
		LogPlayerConnect(pEvent->GetString("name"), pEvent->GetInt("userid"));
	}
	else if (!strcmp(pszEventName, "player_disconnect"))
	{
		LogPlayerDisconnect(pEvent->GetString("name"), pEvent->GetInt("userid"), pEvent->GetInt("reason"));
	}
	else if (!strcmp(pszEventName, "player_team"))
	{
		LogPlayerTeam(pEvent->GetString("name"), pEvent->GetInt("userid"), pEvent->GetInt("team"));
	}
	else if (!strcmp(pszEventName, "entity_killed"))
	{
		int ent = pEvent->GetInt("entindex_killed");
		if (ent >= 0 && ent < gpGlobals->maxEntities)
		{
			static char clsname[64];
			auto *pEnt = EntityOfIndex(ent);
			if (pEnt && servertools->GetKeyValue(pEnt, "classname", clsname, sizeof(clsname)))
			{
				if (!strcmp(clsname, "npc_dota_tower"))
				{
					LogTowerKill(pEnt, pEvent->GetInt("entindex_attacker"));
				}
				else if (!strcmp(clsname, "npc_dota_barracks"))
				{
					LogRaxKill(pEnt, pEvent->GetInt("entindex_attacker"));
				}
			}
		}
	}
}

void ChatDebug(CDOTAUserMsg_ChatEvent &msg)
{
	Msg("Chat event %s\n", DOTA_CHAT_MESSAGE_Name(msg.type()).c_str());
	if (msg.has_value())
		Msg("- Value: %u\n", msg.value());
	if (msg.has_playerid_1())
		Msg("- Player1: %d\n", msg.playerid_1());
	if (msg.has_playerid_2())
		Msg("- Player2: %d\n", msg.playerid_2());
	if (msg.has_playerid_3())
		Msg("- Player3: %d\n", msg.playerid_3());
	if (msg.has_playerid_4())
		Msg("- Player4: %d\n", msg.playerid_4());
	if (msg.has_playerid_5())
		Msg("- Player5: %d\n", msg.playerid_5());
	if (msg.has_playerid_6())
		Msg("- Player6: %d\n", msg.playerid_6());
}

void EventLogger::OnUserMessage(IRecipientFilter &filter, int type, const google::protobuf::Message &msg)
{
	switch ((EDotaUserMessages) type)
	{
		case DOTA_UM_ChatEvent:
		{
			auto chatEvent = (CDOTAUserMsg_ChatEvent &) msg;
			switch (chatEvent.type())
			{
			case CHAT_MESSAGE_COURIER_LOST:
				ChatDebug(chatEvent);
				LogCourierKill(chatEvent.playerid_1() == kTeamDire ? kTeamRadiant : kTeamDire);
				break;
			case CHAT_MESSAGE_HERO_KILL:
				{
					SourceHook::CVector<int> vecKillers;
					if (chatEvent.has_playerid_2())
					{
						vecKillers.push_back(chatEvent.playerid_2());
						if (chatEvent.has_playerid_3())
						{
							vecKillers.push_back(chatEvent.playerid_3());
							if (chatEvent.has_playerid_4())
							{
								vecKillers.push_back(chatEvent.playerid_4());
								if (chatEvent.has_playerid_5())
								{
									vecKillers.push_back(chatEvent.playerid_5());
									if (chatEvent.has_playerid_6())
									{
										vecKillers.push_back(chatEvent.playerid_6());
									}
								}
							}
						}
					}
					LogHeroKill(chatEvent.playerid_1(), vecKillers, chatEvent.value());
				}
				break;
			case CHAT_MESSAGE_RUNE_BOTTLE:
				LogRuneBottle(chatEvent.playerid_1(), (DotaRune) chatEvent.value());
				break;
			case CHAT_MESSAGE_RUNE_PICKUP:
				LogRuneUse(chatEvent.playerid_1(), (DotaRune) chatEvent.value());
				break;
			case CHAT_MESSAGE_ROSHAN_KILL:
				ChatDebug(chatEvent);
				LogRoshanKill(chatEvent.playerid_1(), chatEvent.value());
				break;
			case CHAT_MESSAGE_AEGIS:
				LogSimplePlayerEvent(EventType::AegisPickup, chatEvent.playerid_1());
				break;
			case CHAT_MESSAGE_AEGIS_STOLEN:
				LogSimplePlayerEvent(EventType::AegisSteal, chatEvent.playerid_1());
				break;
			//case CHAT_MESSAGE_HERO_DENY:
			case CHAT_MESSAGE_BUYBACK:
				LogSimplePlayerEvent(EventType::Buyback, chatEvent.playerid_1());
				break;
				// player id
			case CHAT_MESSAGE_DENIED_AEGIS:
				LogSimplePlayerEvent(EventType::AegisDeny, chatEvent.playerid_1());
				break;
			case CHAT_MESSAGE_FIRSTBLOOD:
				LogSimplePlayerEvent(EventType::FirstBlood, chatEvent.playerid_1());
				break;
			case CHAT_MESSAGE_ITEM_PURCHASE:
				{
					bool skip = false;
					int cnt = filter.GetRecipientCount();
					int ourTeam = g_PlayerResource.GetPlayerTeam(chatEvent.playerid_1());
					for (int i = 0; i < cnt; ++i)
					{
						int playerId = g_PlayerResource.PlayerIdFromEntIndex(i);
						if (playerId == -1 || !g_PlayerResource.IsValidPlayer(playerId))
						{
							skip = true;
							break;
						}

						int team = g_PlayerResource.GetPlayerTeam(playerId);
						if (team != ourTeam)
						{
							skip = true;
							break;
						}
					}

					if (!skip)
					{
						LogItemPurchase(chatEvent.playerid_1(), chatEvent.value());
					}
				}
				break;
			}
		}
		break;
	}
}

void EventLogger::LogGameStateChange(DOTA_GameState newState)
{
	json_t *pEvent = CreateTimedEvent(EventType::GameStateChange);
	json_object_set_new(pEvent, "new_state", json_integer(newState));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogPlayerConnect(const char *pszName, int userid)
{
	auto *sid = UserIdToCSteamID(userid);

	if (sid)
	{
		json_t *pEvent = CreateTimedEvent(EventType::PlayerConnect);
		json_object_set_new(pEvent, "player", json_integer(sid ? sid->ConvertToUint64() : 0));

		SendAndFreeEvent(pEvent);
	}
}

void EventLogger::LogPlayerDisconnect(const char *pszName, int userid, int reason)
{
	auto *sid = UserIdToCSteamID(userid);

	if (sid)
	{
		json_t *pEvent = CreateTimedEvent(EventType::PlayerDisconnect);
		json_object_set_new(pEvent, "player", json_integer(sid ? sid->ConvertToUint64() : 0));
		json_object_set_new(pEvent, "reason", json_integer(reason));

		SendAndFreeEvent(pEvent);
	}
}

void EventLogger::LogPlayerTeam(const char *pszName, int userid, int team)
{
	json_t *pEvent = CreateTimedEvent(EventType::PlayerTeam);
	auto *sid = UserIdToCSteamID(userid);
	json_object_set_new(pEvent, "player", json_integer(sid ? sid->ConvertToUint64() : 0));
	json_object_set_new(pEvent, "team", json_integer(team));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogSelectedHero(int playerId, const char *pszHeroName)
{
	json_t *pEvent = CreateTimedEvent(EventType::HeroSelection);
	json_object_set_new(pEvent, "player", json_integer(g_PlayerResource.GetPlayerSteamId(playerId).ConvertToUint64()));
	json_object_set_new(pEvent, "hero", json_string(pszHeroName));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogHeroKill(int victimId, SourceHook::CVector<int> &vecKillers, uint gold)
{
	json_t *pEvent = CreateTimedEvent(EventType::HeroDeath);
	json_object_set_new(pEvent, "player", json_integer(g_PlayerResource.GetPlayerSteamId(victimId).ConvertToUint64()));
	json_object_set_new(pEvent, "gold", json_integer(gold));
	json_t *pKillers = json_array();
	for (int k : vecKillers)
	{
		if (k != -1)
			json_array_append_new(pKillers, json_integer(g_PlayerResource.GetPlayerSteamId(k).ConvertToUint64()));
	}	
	json_object_set_new(pEvent, "killers", pKillers);

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogTowerKill(CBaseEntity *pTower, int attackerEntIdx)
{
	int plyr = g_PlayerResource.PlayerIdFromEntIndex(attackerEntIdx);

	static char buffer[64];
	servertools->GetKeyValue(pTower, "teamnumber", buffer, sizeof(buffer));

	json_t *pEvent;
	int towerTeam = atoi(buffer);
	if (g_PlayerResource.GetPlayerTeam(plyr) == towerTeam)
	{
		pEvent = CreateTimedEvent(EventType::TowerDeny);
	}
	else
	{
		pEvent = CreateTimedEvent(EventType::TowerKill);
	}

	int tier = 0;
	DotaLane lane = DotaLane::Unknown;
	servertools->GetKeyValue(pTower, "targetname", buffer, sizeof(buffer));
	if (strstr(buffer, "1")) {
		tier = 1;
	}
	else if (strstr(buffer, "2")) {
		tier = 2;
	}
	else if (strstr(buffer, "3"))	{
		tier = 3;
	}
	else if (strstr(buffer, "4"))	{
		tier = 4;
	}

	if (tier == 4) {
		lane = DotaLane::Mid;
	}
	else if (strstr(buffer, "top")) {
		lane = DotaLane::Top;
	}
	else if (strstr(buffer, "mid")) {
		lane = DotaLane::Mid;
	}
	else if (strstr(buffer, "bot")) {
		lane = DotaLane::Bot;
	}

	json_object_set_new(pEvent, "player", json_integer(plyr == -1 ? 0 : g_PlayerResource.GetPlayerSteamId(plyr).ConvertToUint64()));
	json_object_set_new(pEvent, "tier", json_integer(tier));
	json_object_set_new(pEvent, "lane", json_integer((int) lane));
	json_object_set_new(pEvent, "team", json_integer(towerTeam));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogRaxKill(CBaseEntity *pRax, int attackerEntIdx)
{
	int plyr = g_PlayerResource.PlayerIdFromEntIndex(attackerEntIdx);

	static char buffer[64];
	servertools->GetKeyValue(pRax, "teamnumber", buffer, sizeof(buffer));

	int raxTeam = atoi(buffer);
	servertools->GetKeyValue(pRax, "targetname", buffer, sizeof(buffer));

	DotaRaxType raxtype = DotaRaxType::Unknown;
	DotaLane lane = DotaLane::Unknown;

	if (strstr(buffer, "melee")) {
		raxtype = DotaRaxType::Melee;
	}
	else if (strstr(buffer, "range")) {
		raxtype = DotaRaxType::Ranged;
	}

	if (strstr(buffer, "top")) {
		lane = DotaLane::Top;
	}
	else if (strstr(buffer, "mid")) {
		lane = DotaLane::Mid;
	}
	else if (strstr(buffer, "bot")) {
		lane = DotaLane::Bot;
	}

	json_t *pEvent = CreateTimedEvent(EventType::RaxKill);
	json_object_set_new(pEvent, "player", json_integer(plyr == -1 ? 0 : g_PlayerResource.GetPlayerSteamId(plyr).ConvertToUint64()));
	json_object_set_new(pEvent, "raxtype", json_integer((int) raxtype));
	json_object_set_new(pEvent, "lane", json_integer((int) lane));
	json_object_set_new(pEvent, "team", json_integer(raxTeam));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogCourierKill(int team)
{
	json_t *pEvent = CreateTimedEvent(EventType::CourierKill);
	json_object_set_new(pEvent, "team", json_integer(team));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogRoshanKill(int team, uint gold)
{
	json_t *pEvent = CreateTimedEvent(EventType::RoshanKill);
	json_object_set_new(pEvent, "team", json_integer(team));
	json_object_set_new(pEvent, "gold", json_integer(gold));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogSimplePlayerEvent(EventType type, int playerId)
{
	json_t *pEvent = CreateTimedEvent(type);
	json_object_set_new(pEvent, "player", json_integer(g_PlayerResource.GetPlayerSteamId(playerId).ConvertToUint64()));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogRuneBottle(int playerId, DotaRune rune)
{
	json_t *pEvent = CreateTimedEvent(EventType::RuneBottled);
	json_object_set_new(pEvent, "player", json_integer(g_PlayerResource.GetPlayerSteamId(playerId).ConvertToUint64()));
	json_object_set_new(pEvent, "rune_type", json_integer((int) rune));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogRuneUse(int playerId, DotaRune rune)
{
	json_t *pEvent = CreateTimedEvent(EventType::RuneUsed);
	json_object_set_new(pEvent, "player", json_integer(g_PlayerResource.GetPlayerSteamId(playerId).ConvertToUint64()));
	json_object_set_new(pEvent, "rune_type", json_integer((int) rune));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogItemPurchase(int playerId, int itemId)
{
	json_t *pEvent = CreateTimedEvent(EventType::ItemPurchase);
	json_object_set_new(pEvent, "player", json_integer(g_PlayerResource.GetPlayerSteamId(playerId).ConvertToUint64()));
	json_object_set_new(pEvent, "item_id", json_integer(itemId));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogGGCall(uint64 steamId64, int team)
{
	json_t *pEvent = CreateTimedEvent(EventType::CallGG);
	json_object_set_new(pEvent, "player", json_integer(steamId64));
	json_object_set_new(pEvent, "team", json_integer(team));

	SendAndFreeEvent(pEvent);
}

void EventLogger::LogGGCancel(uint64 steamId64, int team)
{
	json_t *pEvent = CreateTimedEvent(EventType::CancelGG);
	json_object_set_new(pEvent, "player", json_integer(steamId64));
	json_object_set_new(pEvent, "team", json_integer(team));

	SendAndFreeEvent(pEvent);
}
