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

#include "d2lobby.h"
#include <steam_gameserver.h>

class HTTPManager;
extern HTTPManager g_HTTPManager;

class HTTPManager
{
public:
	void PostJSONToMatchUrl(const char *pszText);
	bool HasAnyPendingRequests() const { return m_PendingRequests.size() > 0; }

private:
	class TrackedRequest
	{
	public:
		TrackedRequest(const TrackedRequest &req) = delete;
		TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, const char *pszText);
		~TrackedRequest();
	private:
		void OnHTTPRequestCompleted(HTTPRequestCompleted_t *arg, bool bFailed);

		HTTPRequestHandle m_hHTTPReq;
		CCallResult<TrackedRequest, HTTPRequestCompleted_t> m_CallResult;
		char *m_pszText;
	};
private:
	SourceHook::CVector<HTTPManager::TrackedRequest *> m_PendingRequests;
};
