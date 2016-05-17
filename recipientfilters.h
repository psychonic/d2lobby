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

#include <sh_vector.h>
#include <irecipientfilter.h>
#include <vector>

class SingleUserRecipientFilter : public IRecipientFilter
{
public:
	SingleUserRecipientFilter(int entidx) { m_iUser = entidx; }
	bool IsReliable() const { return true; }
	bool IsInitMessage() const { return false; }
	int GetRecipientCount() const { return 1; }
	CEntityIndex GetRecipientIndex(int slot) const { return m_iUser; }
private:
	int m_iUser;
};

class MultiRecipientFilter : public IRecipientFilter
{
public:
	bool IsReliable() const { return true; }
	bool IsInitMessage() const { return false; }
	int GetRecipientCount() const { return m_Users.size(); }
	CEntityIndex GetRecipientIndex(int slot) const { return m_Users[slot]; }
public:
	bool AddUser(int entidx)
	{
		for (auto i : m_Users)
		{
			if (i == entidx)
				return false;
		}

		m_Users.push_back(entidx);
		return true;
	}
private:
	SourceHook::CVector<int> m_Users;
};

class FilterEveryone : public IRecipientFilter
{
public:
	FilterEveryone();
public:
	bool IsReliable() const { return true; }
	bool IsInitMessage() const { return false; }
	int GetRecipientCount() const { return m_Users.size(); }
	CEntityIndex GetRecipientIndex(int slot) const { return m_Users[slot]; }
private:
	std::vector<int> m_Users;
};

class FilterTeam : public IRecipientFilter
{
public:
	FilterTeam(int team);
public:
	bool IsReliable() const { return true; }
	bool IsInitMessage() const { return false; }
	int GetRecipientCount() const { return m_Users.size(); }
	CEntityIndex GetRecipientIndex(int slot) const { return m_Users[slot]; }
private:
	std::vector<int> m_Users;
};
