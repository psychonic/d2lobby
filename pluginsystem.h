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

#include <sourcehook.h>
#include <eiface.h>
#include <sh_list.h>

class CCommand;
class IRecipientFilter;
namespace google { namespace protobuf {
	class Message;
}; };

class IPluginSystem
{
public:
	IPluginSystem();
public:
	virtual ~IPluginSystem() {}
	virtual bool OnLoad() { return true; }
	virtual void OnUnload() {}
	virtual void OnServerActivated() {}
	virtual void OnLevelShutdown() {}
	virtual void OnUserMessage(IRecipientFilter &filter, int type, const google::protobuf::Message &msg) {}
	virtual META_RES OnClientCommand(CEntityIndex ent, const CCommand &args) { return MRES_IGNORED; }
	virtual void OnClientMessage(CEntityIndex ent, int msg_type, int size, const uint8 *pData) {}
	virtual const char* name() { return "none"; }
	
};

SourceHook::List<IPluginSystem *> &PluginSystems();