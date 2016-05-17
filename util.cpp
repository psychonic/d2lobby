#include "util.h"

#include <Windows.h>

#include <server_class.h>
#include <toolframework/itoolentity.h>

#include <steamclientpublic.h>

IHandleEntity *CBaseHandle::Get() const
{
	if (!IsValid())
	{
		return nullptr;
	}

	auto *pEdict = PEntityOfEntIndex(GetEntryIndex());
	if (!pEdict || pEdict->IsFree())
	{
		return nullptr;
	}

	auto *pUnknown = pEdict->GetIServerEntity();
	if (!pUnknown)
	{
		return nullptr;
	}

	if (*this != pUnknown->GetRefEHandle())
	{
		return nullptr;
	}

	return pUnknown;
}

int UTIL_FindInSendTable(SendTable *pTable, const char *name, unsigned int offset, SendProp **pOutProp)
{
	const char *pname;
	int props = pTable->GetNumProps();
	SendProp *prop;

	for (int i = 0; i<props; i++)
	{
		prop = pTable->GetPropA(i);
		pname = prop->GetName();
		if (pname && strcmp(name, pname) == 0)
		{
			if (pOutProp)
				*pOutProp = prop;
			return offset + prop->GetOffset();
		}
		if (prop->GetDataTable())
		{
			SendProp *pOut;
			int realOffset;
			if ((realOffset = UTIL_FindInSendTable(prop->GetDataTable(),
				name,
				offset + prop->GetOffset(), &pOut)) != -1
				)
			{
				if (pOutProp)
					*pOutProp = pOut;
				return realOffset;
			}
		}
	}

	return -1;
}

CBaseEntity *UTIL_FindEntityByClassname(CBaseEntity *pStartEnt, const char *pszClassname)
{
	IHandleEntity *pCurrEnt = (IHandleEntity *) pStartEnt;
	if (!pCurrEnt)
	{
		pCurrEnt = (IHandleEntity *) servertools->FirstEntity();
	}

	if (!pCurrEnt)
	{
		return nullptr;
	}

	char classname[64];
	do
	{
		servertools->GetKeyValue(pCurrEnt, "classname", classname, sizeof(classname));
		if (!strcmp(classname, pszClassname))
		{
			return ((IServerUnknown *) pCurrEnt)->GetBaseEntity();
		}
	} while ((pCurrEnt = (IHandleEntity *) servertools->NextEntity(pCurrEnt)));

	return nullptr;
}

void *UTIL_FindAddress(void *startAddr, const char *sig, size_t len)
{
#ifdef _WIN32
	bool found;
	char *ptr, *end;

	MEMORY_BASIC_INFORMATION mem;

	if (!startAddr)
		return NULL;

	if (!VirtualQuery(startAddr, &mem, sizeof(mem)))
		return NULL;

	IMAGE_DOS_HEADER *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(mem.AllocationBase);
	IMAGE_NT_HEADERS *pe = reinterpret_cast<IMAGE_NT_HEADERS *>((intp) dos + dos->e_lfanew);

	if (pe->Signature != IMAGE_NT_SIGNATURE)
	{
		// GetDllMemInfo failedpe points to a bad location
		return NULL;
	}

	ptr = reinterpret_cast<char *>(mem.AllocationBase);
	end = ptr + pe->OptionalHeader.SizeOfImage - len;

	while (ptr < end)
	{
		found = true;
		for (size_t i = 0; i < len; i++)
		{
			if (sig[i] != '\x2A' && sig[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
			return ptr;

		ptr++;
	}
#endif

	return NULL;
}


string_t UTIL_AllocPooledString(const char *pszString)
{
	CBaseEntity *pEntity = ((IServerUnknown *)servertools->FirstEntity())->GetBaseEntity();
	static int offset = UTIL_FindInSendTable(((IServerUnknown *)pEntity)->GetNetworkable()->GetServerClass()->m_pTable, "m_iName");

	string_t *pProp = (string_t *) ((intp)pEntity + offset);
	string_t backup = *pProp;
	servertools->SetKeyValue(pEntity, "targetname", pszString);
	string_t newString = *pProp;
	*pProp = backup;

	return newString;
}

CSteamID StringToCSteamID(const char *pszRendered)
{
	CSteamID ret(0, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual);

	if (strlen(pszRendered) < 11)
		return ret;

	if (!!strncmp(pszRendered, "STEAM_1", 7))
		return ret;

	if (pszRendered[7] != ':' || pszRendered[9] != ':')
		return ret;

	if (pszRendered[8] != '0' && pszRendered[8] != '1')
		return ret;

	AccountID_t acctId = atoi(&pszRendered[10]) << 1;
	if (pszRendered[8] == '1')
		acctId += 1;

	ret.SetAccountID(acctId);

	return ret;
}

void *FindPatchAddress(const char *sig, size_t len, void *pLib)
{
	bool found;
	char *ptr, *end;

	LPCVOID startAddr = pLib;

	MEMORY_BASIC_INFORMATION mem;

	if (!startAddr)
		return NULL;

	if (!VirtualQuery(startAddr, &mem, sizeof(mem)))
		return NULL;

	IMAGE_DOS_HEADER *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(mem.AllocationBase);
	IMAGE_NT_HEADERS *pe = reinterpret_cast<IMAGE_NT_HEADERS *>((intp) dos + dos->e_lfanew);

	if (pe->Signature != IMAGE_NT_SIGNATURE)
	{
		// GetDllMemInfo failedpe points to a bad location
		return NULL;
	}

	ptr = reinterpret_cast<char *>(mem.AllocationBase);
	end = ptr + pe->OptionalHeader.SizeOfImage - len;

	while (ptr < end)
	{
		found = true;
		for (size_t i = 0; i < len; i++)
		{
			if (sig[i] != '\x2A' && sig[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
			return ptr;

		ptr++;
	}

	return NULL;
}
