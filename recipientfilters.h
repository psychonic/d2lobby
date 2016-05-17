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
