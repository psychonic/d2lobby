#pragma once

#include "pluginsystem.h"

#include <igameevents.h>

class MidOnlyCustom : public IPluginSystem,
	public IGameEventListener2
{
	bool OnLoad() override;
	void OnUnload() override;
	void OnServerActivated() override;
	void OnLevelShutdown() override;
	META_RES OnClientCommand(CEntityIndex ent, const CCommand &args) override;

	void FireGameEvent(IGameEvent *pEvent) override;

public:
	void SetHero(const char *pszHero);

private:
	char m_szMidHero[64];

	KeyValues *m_pkvHeroes = nullptr;
};