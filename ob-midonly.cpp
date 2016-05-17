#include "d2lobby.h"
#include "ob-midonly.h"
#include "gameruleshelper.h"
#include "playerresourcehelper.h"

#include <filesystem.h>
#include <fmtstr.h>
#include <utlvector.h>
#include <vstdlib/random.h>

static MidOnlyCustom s_MidOnlyCustom;

CON_COMMAND(set_forced_hero, "set_forced_hero <hero entity name>")
{
	if (args.ArgC() < 2)
	{
		Msg("set_forced_hero <hero entity name>\n");
	}
	s_MidOnlyCustom.SetHero(args[1]);
}

bool MidOnlyCustom::OnLoad()
{
	m_szMidHero[0] = '\0';
	m_pkvHeroes = new KeyValues("DOTAHeroes");
	return m_pkvHeroes->LoadFromFile(filesystem, "scripts/npc/npc_heroes.txt");
}

void MidOnlyCustom::OnUnload()
{
	m_pkvHeroes->deleteThis();
}

void MidOnlyCustom::OnServerActivated()
{
	eventmgr->AddListener(this, "game_rules_state_change", true);
}

void MidOnlyCustom::OnLevelShutdown()
{
	eventmgr->RemoveListener(this);
}

void MidOnlyCustom::SetHero(const char *pszHero)
{
	if (!Q_stricmp(pszHero, "random"))
	{
		CUtlVector<const char *> validHeroes;
		FOR_EACH_SUBKEY(m_pkvHeroes, h)
		{
			if (!h->GetBool("Enabled"))
				continue;

			validHeroes.AddToTail(h->GetName());
		}

		const char *pszRandomHero = validHeroes[RandomInt(0, validHeroes.Count() - 1)];
		Q_strncpy(m_szMidHero, pszRandomHero, sizeof(m_szMidHero));
	}
	else if (!m_pkvHeroes->FindKey(pszHero))
	{
		Msg("Cannot find hero named \"%s\"\n", pszHero);
	}
	else
	{
		Q_strncpy(m_szMidHero, pszHero, sizeof(m_szMidHero));
	}
}

void MidOnlyCustom::FireGameEvent(IGameEvent *pEvent)
{
	if (!Q_strcmp(pEvent->GetName(), "game_rules_state_change"))
	{
		if (g_GameRules.GetState() == DOTA_GAMERULES_STATE_HERO_SELECTION && m_szMidHero[0])
		{
			CCommand args;
			args.Tokenize(CFmtStr("dota_select_hero %s reserve", m_szMidHero));

			for (int i = 0; i < kMaxGamePlayerIds; ++i)
			{
				if (!g_PlayerResource.IsValidPlayer(i))
					continue;

				int idx = g_PlayerResource.GetPlayerEntIndex(i);
				serverclients->ClientCommand(CEntityIndex(idx), args);
			}
		}
	}
}

META_RES MidOnlyCustom::OnClientCommand(CEntityIndex ent, const CCommand &args)
{
	if (m_szMidHero[0])
	{
		if (args.ArgC() >= 2 && !Q_stricmp(args[0], "dota_select_hero") && !Q_stricmp(args[1], "repick"))
		{
			return MRES_SUPERCEDE;
		}
	}

	return MRES_IGNORED;
}
