#pragma once

#include <eiface.h>
#include <filesystem.h>
#include "constants.h"

#include <string>

class SendProp;
class IServerTools;

extern CGlobalVars *gpGlobals;
extern IServerTools *servertools;
extern IVEngineServer *engine;

inline int UserIdToEntIndex(int userid)
{
	for (int i = 0; i < gpGlobals->maxClients; ++i)
	{
		if (engine->GetPlayerUserId(i) == userid)
			return i + 1;
	}

	return -1;
}

inline const CSteamID *UserIdToCSteamID(int userid)
{
	for (int i = 0; i < gpGlobals->maxClients; ++i)
	{
		if (engine->GetPlayerUserId(i) == userid)
			return engine->GetClientSteamID(i + 1);
	}

	return nullptr;
}

inline edict_t *PEntityOfEntIndex(int iEntIndex)
{
	if (iEntIndex >= 0 && iEntIndex < gpGlobals->maxEntities)
	{
		return (edict_t *) (gpGlobals->pEdicts + iEntIndex);
	}
	return nullptr;
}

inline int IndexOfEdict(const edict_t *pEdict)
{
	return (int) (pEdict - gpGlobals->pEdicts);
}

inline CBaseEntity *UTIL_EdictToBaseEntity(edict_t *pEdict)
{
	if (pEdict)
	{
		auto *pUnk = pEdict->GetUnknown();
		if (pUnk)
		{
			return pUnk->GetBaseEntity();
		}
	}

	return nullptr;
}

int UTIL_FindInSendTable(SendTable *pTable, const char *name, unsigned int offset = 0, SendProp **pOutProp = nullptr);

CBaseEntity *UTIL_FindEntityByClassname(CBaseEntity *pStartEnt, const char *pszClassname);

void *UTIL_FindAddress(void *startAddr, const char *sig, size_t len);

string_t UTIL_AllocPooledString(const char *pszString);

CSteamID StringToCSteamID(const char *pszRendered);

inline CBaseEntity *EntityOfIndex(int idx)
{
	edict_t *pEdict = PEntityOfEntIndex(idx);
	if (!pEdict || pEdict->IsFree())
		return nullptr;

	return pEdict->GetUnknown()->GetBaseEntity();
}

inline int OtherTeam(int team)
{
	return team == kTeamRadiant ? kTeamDire : kTeamRadiant;
}

template <typename ... Ts>
void UTIL_LogToFile(const char *pMsg, Ts ... ts)
{
#ifdef D2L_LOGGING
	extern FileHandle_t g_pLogFile;
	if (g_pLogFile)
	{
		filesystem->FPrintf(g_pLogFile, pMsg, ts...);
	}
#endif
}

template <typename ... Ts>
void UTIL_MsgAndLog(const char *pMsg, Ts ... ts)
{
#ifdef D2L_LOGGING
	Msg(pMsg, ts...);
	extern FileHandle_t g_pLogFile;
	if (g_pLogFile)
	{
		filesystem->FPrintf(g_pLogFile, pMsg, ts...);
	}
#endif
}

void *FindPatchAddress(const char *sig, size_t len, void *pLib);

