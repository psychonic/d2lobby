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