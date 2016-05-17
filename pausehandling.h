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

#include <igameevents.h>
#include "pluginsystem.h"
#include "constants.h"

class PauseHandling : public IGameEventListener2, public IPluginSystem
{
public: // IGameEventListener2
	void FireGameEvent(IGameEvent *pEvent) override;
	int GetEventDebugID() override { return EVENT_DEBUG_ID_INIT; }
public: // IPluginSystem
	const char* name() override { return "ph"; }
	bool OnLoad() override;
	void OnUnload() override;
	void OnServerActivated() override;
	void OnLevelShutdown() override;
public:
	void Hook_ClientDisconnect(CEntityIndex idx, int reason);
	void Hook_ThinkPost(bool bFinalTick);
	bool Hook_DispatchClientMessage(CEntityIndex index, int msg_type, int size, const uint8 *pData);

private:
	void SetForcePaused(int team);
	void SetForceUnpaused(int team);
	inline bool IsForcePaused() const { return m_flForcePauseEndTime > 0.f; }

private:
	int m_iRadiantPausesUsed = 0;
	int m_iDirePausesUsed = 0;
	float m_flForcePauseEndTime = 0.f;
	int m_iForcePauseTeam = kTeamUnassigned;
	int m_iThinkHook = 0;
};
