#include "pluginsystem.h"

IPluginSystem::IPluginSystem()
{
	PluginSystems().push_back(this);
}

SourceHook::List<IPluginSystem *> &PluginSystems()
{
	static SourceHook::List<IPluginSystem *> *l = new SourceHook::List<IPluginSystem *>();
	return *l;
}
