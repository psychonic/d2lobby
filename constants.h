#pragma once

#define MSG_TAG "[D2Lobby] "

const int kMaxTeamPlayers = 5;
const int kMaxGamePlayerIds = 10;
const int kSpectatorIdStart = kMaxGamePlayerIds;
const int kMaxTotalPlayerIds = 32;
const int kMaxBroadcastChannels = 6;
const int kMaxBroadcastChannelSlots = 4;
const int kMaxPlayerNameLength = 128;

// Classic Source colors
extern const char *g_ColorGreen;
extern const char *g_ColorDarkGreen;
extern const char *g_ColorBlue;
extern const char *g_ColorRed;
extern const char *g_ColorYellow;
extern const char *g_ColorGrey;

enum DotaTeam
{
	kTeamUnassigned = 0,
	kTeamSpectators = 1,
	kTeamRadiant = 2,
	kTeamDire = 3,
	kTeamNeutrals = 4,
};

enum DotaRune : int
{
	DoubleDamage,
	Haste,
	Illusion,
	Invisibility,
	Regeneration,
};

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4
