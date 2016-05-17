#include "httpmgr.h"

#include "util.h"

HTTPManager g_HTTPManager;

#undef strdup

HTTPManager::TrackedRequest::TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, const char *pszText)
{
	m_hHTTPReq = hndl;
	m_CallResult.SetGameserverFlag();
	m_CallResult.Set(hCall, this, &TrackedRequest::OnHTTPRequestCompleted);

	m_pszText = strdup(pszText);

	g_HTTPManager.m_PendingRequests.push_back(this);
}

HTTPManager::TrackedRequest::~TrackedRequest()
{
	for (auto e = g_HTTPManager.m_PendingRequests.begin(); e != g_HTTPManager.m_PendingRequests.end(); ++e)
	{
		if (*e == this)
		{
			g_HTTPManager.m_PendingRequests.erase(e);
			break;
		}
	}

	free(m_pszText);
}

void HTTPManager::TrackedRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t *arg, bool bFailed)
{
	if (bFailed || arg->m_eStatusCode < 200 || arg->m_eStatusCode > 299)
	{
		g_HTTPManager.PostJSONToMatchUrl(m_pszText);
	}
	else
	{
		uint32 size;
		http->GetHTTPResponseBodySize(arg->m_hRequest, &size);

		uint8 *response = new uint8[size];
		http->GetHTTPResponseBodyData(arg->m_hRequest, response, size);

		if (size < 2 || response[0] != 'o' || response[1] != 'k')
		{
			g_HTTPManager.PostJSONToMatchUrl(m_pszText);
		}
	}
	
	if (http)
	{
		http->ReleaseHTTPRequest(arg->m_hRequest);
	}
	delete this;
}

void HTTPManager::PostJSONToMatchUrl(const char *pszText)
{
//	UTIL_MsgAndLog("Sending HTTP:\n%s\n", pszText);

	if (!match_post_url.GetString()[0])
	{
		return;
	}

	auto hReq = http->CreateHTTPRequest(k_EHTTPMethodPOST, match_post_url.GetString());

//	UTIL_MsgAndLog("HTTP request: %p\n", hReq);

	int size = strlen(pszText);
	if (http->SetHTTPRequestRawPostBody(hReq, "application/json", (uint8 *) pszText, size))
	{
		SteamAPICall_t hCall;
		http->SendHTTPRequest(hReq, &hCall);

		new TrackedRequest(hReq, hCall, pszText);
	}
	else
	{
//		UTIL_MsgAndLog("Failed to SetHTTPRequestRawPostBody\n");
	}
}