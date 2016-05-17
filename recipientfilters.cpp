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