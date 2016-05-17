#include "pausehandling.h"

#include <convar.h>

ConVar d2l_block_pause("d2l_block_pause", "0");

#include "d2lobby.h"
#include "playerresourcehelper.h"
#include "util.h"

#include "protobuf/dota_clientmessages.pb.h"

#include <eiface.h>

PauseHandling g_PauseHandling;

ConVar d2l_team_disconnect_pause_time("d2l_team_disconnect_pause_time", "180", 0, "Seconds to auto-pause for disconnected team.");
ConVar d2l_team_disconnect_pause_count("d2l_team_disconnect_pause_count", "1", 0, "Number of automatic pauses allowed for a disconnected team.");

SH_DECL_HOOK2_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CEntityIndex, int);
SH_DECL_HOOK4(IServerGameClients, DispatchClientMessage, SH_NOATTRIB, 0, bool, CEntityIndex, int, int, const uint8 *);
SH_DECL_EXTERN1_void(IServerGameDLL, Think, SH_NOATTRIB, 0, bool);

bool PauseHandling::OnLoad()
{
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, serverclients, SH_MEMBER(this, &PauseHandling::Hook_ClientDisconnect), false);
	SH_ADD_HOOK(IServerGameClients, DispatchClientMessage, serverclients, SH_MEMBER(this, &PauseHandling::Hook_DispatchClientMessage), false);
	return true;
}

void PauseHandling::OnUnload()
{
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, serverclients, SH_MEMBER(this, &PauseHandling::Hook_ClientDisconnect), false);
	SH_REMOVE_HOOK(IServerGameClients, DispatchClientMessage, serverclients, SH_MEMBER(this, &PauseHandling::Hook_DispatchClientMessage), false);
}

void PauseHandling::OnServerActivated()
{
	eventmgr->AddListener(this, "player_connect_full", true);
}

void PauseHandling::OnLevelShutdown()
{
	eventmgr->RemoveListener(this);
}

void PauseHandling::FireGameEvent(IGameEvent *pEvent)
{
	if (g_GameRules.GetState() >= DOTA_GAMERULES_STATE_POST_GAME)
		return;

	const char *pszName = pEvent->GetName();
	if (!Q_strcmp(pszName, "player_connect_full"))
	{
		int userid = pEvent->GetInt("userid");
		if (userid == 0)
			return;

		int entIdx = UserIdToEntIndex(userid);
		if (entIdx <= 0)
			return;

		int team = g_PlayerResource.GetPlayerTeam(g_PlayerResource.PlayerIdFromEntIndex(entIdx));
		DevMsg("TEAM: %d\n", team);
		DevMsg("FPT: %d\n", m_iForcePauseTeam);
		DevMsg("TCC: %d\n", g_PlayerResource.GetTeamConnectedCount(m_iForcePauseTeam));
		DevMsg("TPC: %d\n", g_PlayerResource.GetTeamPlayerCount(m_iForcePauseTeam));
		DevMsg("IFP: %d\n", IsForcePaused());
		if (IsForcePaused() && m_iForcePauseTeam == team
			&& g_PlayerResource.GetTeamConnectedCount(m_iForcePauseTeam) == g_PlayerResource.GetTeamPlayerCount(m_iForcePauseTeam))
		{
			DevMsg("Conditonal true!\n");
			SetForceUnpaused(m_iForcePauseTeam);
		}
	}
}

void PauseHandling::Hook_ClientDisconnect(CEntityIndex idx, int reason)
{
	if (IsForcePaused() || g_GameRules.GetState() <= DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS_TO_LOAD)
		return;

	int team = g_PlayerResource.GetPlayerTeam(g_PlayerResource.PlayerIdFromEntIndex(idx.Get()));
	if ((team == kTeamRadiant && m_iRadiantPausesUsed < d2l_team_disconnect_pause_count.GetInt())
		|| (team == kTeamDire && m_iDirePausesUsed < d2l_team_disconnect_pause_count.GetInt())
		)
	{
		SetForcePaused(team);
	}

	RETURN_META(MRES_IGNORED);
}

bool PauseHandling::Hook_DispatchClientMessage(CEntityIndex index, int msg_type, int size, const uint8 *pData)
{
	if (msg_type == DOTA_CM_Pause && (IsForcePaused() || d2l_block_pause.GetBool()))
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, true);
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void PauseHandling::Hook_ThinkPost(bool bFinalTick)
{
	if (IsForcePaused() && (m_flForcePauseEndTime + d2l_team_disconnect_pause_time.GetFloat()) < gpGlobals->curtime)
	{
		SetForceUnpaused(m_iForcePauseTeam);
	}
}

void PauseHandling::SetForcePaused(int team)
{
	if (IsForcePaused())
		return;

	if (m_iThinkHook != 0)
	{
		UTIL_MsgAndLog(MSG_TAG "Warning: Think hook already exists in SetPaused.");
		SH_REMOVE_HOOK_ID(m_iThinkHook);
	}

	m_iThinkHook = SH_ADD_HOOK(IServerGameDLL, Think, gamedll, SH_MEMBER(this, &PauseHandling::Hook_ThinkPost), true);

	if (g_GameRules.IsPaused())
	{
		// already paused, update pause team to none
		DevMsg("Already paused!\n");
		g_GameRules.SetPauseTeam(kTeamUnassigned);
	}
	else
	{
		g_GameRules.SetPaused(true);
	}

	if (team == kTeamRadiant)
		++m_iRadiantPausesUsed;
	else
		++m_iDirePausesUsed;

	m_flForcePauseEndTime = gpGlobals->curtime;
	m_iForcePauseTeam = team;
}

void PauseHandling::SetForceUnpaused(int team)
{
	DevMsg("Force unpaused..\n");
	g_GameRules.SetPaused(false);
	g_GameRules.SetPauseTeam(kTeamUnassigned);
	m_flForcePauseEndTime = 0.f;
	m_iForcePauseTeam = kTeamUnassigned;

	if (m_iThinkHook == 0)
	{
		UTIL_MsgAndLog(MSG_TAG "Warning: Think hook doesn't exist in SetUnpaused.");
	}
	else
	{
		SH_REMOVE_HOOK_ID(m_iThinkHook);
	}

	int other = OtherTeam(team);
	DevMsg("Other: %d\n", other);
	DevMsg("TCC: %d\n", (int)g_PlayerResource.GetTeamConnectedCount(other));
	DevMsg("TPC: %d\n", (int)g_PlayerResource.GetTeamPlayerCount(other));

	if (g_PlayerResource.GetTeamConnectedCount(other) != g_PlayerResource.GetTeamPlayerCount(other))
	{
		if ((other == kTeamDire && m_iDirePausesUsed < d2l_team_disconnect_pause_count.GetInt()) ||( other == kTeamRadiant && m_iRadiantPausesUsed < d2l_team_disconnect_pause_count.GetInt()))
			SetForcePaused(other);
	}
}
