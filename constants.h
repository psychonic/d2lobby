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
