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

#include <igameevents.h>

class ForcedHero : public IPluginSystem,
	public IGameEventListener2
{
	const char* name() override { return "ForcedHero"; }
	bool OnLoad() override;
	void OnServerActivated() override;
	void OnLevelShutdown() override;
	META_RES OnClientCommand(CEntityIndex ent, const CCommand &args) override;

	void FireGameEvent(IGameEvent *pEvent) override;
	int GetEventDebugID() override { return EVENT_DEBUG_ID_INIT; }

public:
	void SetHero(const char *pszHero);

private:
	char m_szForcedHero[64];
};