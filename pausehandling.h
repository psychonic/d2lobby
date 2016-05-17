#pragma once

#include <igameevents.h>
#include "pluginsystem.h"
#include "constants.h"

class PauseHandling : public IGameEventListener2, public IPluginSystem
{
public: // IGameEventListener2
	void FireGameEvent(IGameEvent *pEvent) override;
	int GetEventDebugID() override { return EVENT_DEBUG_ID_INIT; }
public: // IPluginSystem
	const char* name() override { return "ph"; }
	bool OnLoad() override;
	void OnUnload() override;
	void OnServerActivated() override;
	void OnLevelShutdown() override;
public:
	void Hook_ClientDisconnect(CEntityIndex idx, int reason);
	void Hook_ThinkPost(bool bFinalTick);
	bool Hook_DispatchClientMessage(CEntityIndex index, int msg_type, int size, const uint8 *pData);

private:
	void SetForcePaused(int team);
	void SetForceUnpaused(int team);
	inline bool IsForcePaused() const { return m_flForcePauseEndTime > 0.f; }

private:
	int m_iRadiantPausesUsed = 0;
	int m_iDirePausesUsed = 0;
	float m_flForcePauseEndTime = 0.f;
	int m_iForcePauseTeam = kTeamUnassigned;
	int m_iThinkHook = 0;
};
