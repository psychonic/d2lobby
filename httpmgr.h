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
