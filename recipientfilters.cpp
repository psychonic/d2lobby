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

#include "recipientfilters.h"
#include "playerresourcehelper.h"

FilterEveryone::FilterEveryone()
{
	for (int i = 0; i < kMaxTotalPlayerIds; ++i)
	{
		if (!g_PlayerResource.IsValidPlayer(i) || g_PlayerResource.IsFakeClient(i) || g_PlayerResource.GetPlayerConnectionState(i) != DOTA_CONNECTION_STATE_CONNECTED)
			continue;

		int entidx = g_PlayerResource.GetPlayerEntIndex(i);
		if (entidx == -1)
			continue;

		m_Users.push_back(entidx);
	}
}


FilterTeam::FilterTeam(int team)
{
	int maxId = kMaxTotalPlayerIds;
	if (team == kTeamRadiant || team == kTeamDire)
		maxId = kMaxGamePlayerIds;

	for (int i = 0; i < maxId; ++i)
	{
		if (!g_PlayerResource.IsValidPlayer(i) || g_PlayerResource.IsFakeClient(i) || g_PlayerResource.GetPlayerConnectionState(i) != DOTA_CONNECTION_STATE_CONNECTED)
			continue;

		if (g_PlayerResource.GetPlayerTeam(i) != team)
			continue;

		int entidx = g_PlayerResource.GetPlayerEntIndex(i);
		if (entidx == -1)
			continue;

		m_Users.push_back(entidx);
	}
}