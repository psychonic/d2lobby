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