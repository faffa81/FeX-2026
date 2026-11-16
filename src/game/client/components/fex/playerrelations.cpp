#include "playerrelations.h"

#include <base/system.h>
#include <engine/shared/protocol.h>
#include <game/client/gameclient.h>

#include <game/client/components/fex/fexwarlist.h>

bool CPlayerRelations::STempEntry::Matches(EType Type, const char *pName) const
{
	return m_Type == Type && !str_comp(m_aName, pName);
}

SPlayerRelation CPlayerRelations::Get(int ClientId) const
{
	SPlayerRelation Rel;
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return Rel;

	const CWarDataCache &War = GameClient()->m_WarList.m_WarPlayers[ClientId];
	const STempData &Temp = m_aTempPlayers[ClientId];

	Rel.m_PersistentWar = War.m_WarGroupMatches.size() > 1 && War.m_WarGroupMatches[1];
	Rel.m_PersistentTeam = War.m_WarGroupMatches.size() > 2 && War.m_WarGroupMatches[2];
	Rel.m_PersistentHelper = War.m_WarGroupMatches.size() > 3 && War.m_WarGroupMatches[3];
	Rel.m_PersistentMute = War.IsMuted;

	Rel.m_TempWar = Temp.m_War;
	Rel.m_TempHelper = Temp.m_Helper;
	Rel.m_TempMute = Temp.m_Mute;

	Rel.m_IsWarName = War.IsWarName;
	Rel.m_IsWarClan = War.IsWarClan;
	Rel.m_NameColor = War.m_NameColor;
	Rel.m_ClanColor = War.m_ClanColor;

	Rel.m_pReason = War.m_aReason;
	if(str_comp(Temp.m_aReason, "") != 0)
	{
		Rel.m_pTempReason = Temp.m_aReason;
		Rel.m_pReason = Temp.m_aReason;
	}

	return Rel;
}

bool CPlayerRelations::IsAnyList(int ClientId) const
{
	const SPlayerRelation Rel = Get(ClientId);
	return Rel.m_IsWarName || Rel.AtWar() || Rel.IsTeam() || Rel.IsHelper() || Rel.IsMuted();
}

bool CPlayerRelations::TempColor(int ClientId, ColorRGBA *pColor) const
{
	const SPlayerRelation Rel = Get(ClientId);
	CWarList &WarList = GameClient()->m_WarList;

	if(Rel.m_TempWar)
	{
		*pColor = WarList.m_WarTypes[1]->m_Color;
		return true;
	}
	if(Rel.m_TempHelper)
	{
		*pColor = WarList.m_WarTypes[3]->m_Color;
		return true;
	}
	return false;
}

bool CPlayerRelations::HasWarEntry(int ClientId) const
{
	const SPlayerRelation Rel = Get(ClientId);
	return Rel.m_IsWarName || Rel.m_IsWarClan;
}

ColorRGBA CPlayerRelations::PriorityColor(int ClientId) const
{
	const SPlayerRelation Rel = Get(ClientId);
	return Rel.m_IsWarClan && !Rel.m_IsWarName ? Rel.m_ClanColor : Rel.m_NameColor;
}

void CPlayerRelations::AddTempEntry(STempEntry::EType Type, const char *pName, const char *pReason)
{
	STempEntry Entry;
	Entry.m_Type = Type;
	str_copy(Entry.m_aName, pName);
	str_copy(Entry.m_aReason, pReason);
	m_vTempEntries.push_back(Entry);
	Refresh();
}

bool CPlayerRelations::RemoveTempEntries(STempEntry::EType Type, const char *pName)
{
	bool Removed = false;
	for(auto it = m_vTempEntries.begin(); it != m_vTempEntries.end();)
	{
		if(it->Matches(Type, pName))
		{
			it = m_vTempEntries.erase(it);
			Removed = true;
		}
		else
			++it;
	}
	if(Removed)
		Refresh();
	return Removed;
}

void CPlayerRelations::TempWar(const char *pName, const char *pReason, bool Silent)
{
	if(str_comp(pName, "") == 0)
		return;

	RemoveTempEntries(STempEntry::EType::WAR, pName);
	AddTempEntry(STempEntry::EType::WAR, pName, pReason);
	RemoveTempEntries(STempEntry::EType::HELPER, pName);

	if(!Silent)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Added \"%s\" to the Temp War List", pName);
		GameClient()->aMessage(aBuf);
	}
}

void CPlayerRelations::TempHelper(const char *pName, const char *pReason, bool Silent)
{
	if(str_comp(pName, "") == 0)
		return;

	RemoveTempEntries(STempEntry::EType::HELPER, pName);
	AddTempEntry(STempEntry::EType::HELPER, pName, pReason);
	RemoveTempEntries(STempEntry::EType::WAR, pName);

	if(!Silent)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Added \"%s\" to the Temp Helper List", pName);
		GameClient()->aMessage(aBuf);
	}
}

void CPlayerRelations::TempMute(const char *pName, bool Silent)
{
	if(str_comp(pName, "") == 0)
		return;

	AddTempEntry(STempEntry::EType::MUTE, pName, "");

	if(!Silent)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Added \"%s\" to the Temp Mute List", pName);
		GameClient()->aMessage(aBuf);
	}
}

bool CPlayerRelations::UnTempWar(const char *pName, bool Silent)
{
	if(str_comp(pName, "") == 0)
		return false;

	const bool Removed = RemoveTempEntries(STempEntry::EType::WAR, pName);

	char aBuf[128];
	if(Removed)
		str_format(aBuf, sizeof(aBuf), "Removed \"%s\" from the Temp War List", pName);
	else
		str_format(aBuf, sizeof(aBuf), "couldn't find \"%s\" on the Temp War List", pName);

	if(!Silent)
		GameClient()->aMessage(aBuf);
	return Removed;
}

bool CPlayerRelations::UnTempHelper(const char *pName, bool Silent)
{
	if(str_comp(pName, "") == 0)
		return false;

	const bool Removed = RemoveTempEntries(STempEntry::EType::HELPER, pName);

	char aBuf[128];
	if(Removed)
		str_format(aBuf, sizeof(aBuf), "Removed \"%s\" from the Temp Helper List", pName);
	else
		str_format(aBuf, sizeof(aBuf), "couldn't find \"%s\" on the Temp Helper List", pName);

	if(!Silent)
		GameClient()->aMessage(aBuf);
	return Removed;
}

bool CPlayerRelations::UnTempMute(const char *pName, bool Silent)
{
	if(str_comp(pName, "") == 0)
		return false;

	const bool Removed = RemoveTempEntries(STempEntry::EType::MUTE, pName);

	char aBuf[128];
	if(Removed)
		str_format(aBuf, sizeof(aBuf), "Removed \"%s\" from the Temp Mute List", pName);
	else
		str_format(aBuf, sizeof(aBuf), "couldn't find \"%s\" on the Temp Mute List", pName);

	if(!Silent)
		GameClient()->aMessage(aBuf);
	return Removed;
}

void CPlayerRelations::Refresh()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!GameClient()->m_aClients[i].m_Active)
			continue;

		m_aTempPlayers[i].m_War = false;
		m_aTempPlayers[i].m_Helper = false;
		m_aTempPlayers[i].m_Mute = false;
		m_aTempPlayers[i].m_aReason[0] = '\0';

		for(const STempEntry &Entry : m_vTempEntries)
		{
			if(str_comp(GameClient()->m_aClients[i].m_aName, Entry.m_aName) != 0)
				continue;

			switch(Entry.m_Type)
			{
			case STempEntry::EType::WAR:
				m_aTempPlayers[i].m_War = true;
				break;
			case STempEntry::EType::HELPER:
				m_aTempPlayers[i].m_Helper = true;
				break;
			case STempEntry::EType::MUTE:
				m_aTempPlayers[i].m_Mute = true;
				break;
			}
			if(str_comp(Entry.m_aReason, "") != 0)
				str_copy(m_aTempPlayers[i].m_aReason, Entry.m_aReason);
		}
	}
}

void CPlayerRelations::OnNewSnapshot()
{
	Refresh();
}

void CPlayerRelations::OnConsoleInit()
{
	Console()->Register("addtempwar", "s[name] ?r[reason]", CFGFLAG_CLIENT, ConTempWar, this, "temporary War");
	Console()->Register("deltempwar", "s[name]", CFGFLAG_CLIENT, ConUnTempWar, this, "remove temporary War");

	Console()->Register("addtemphelper", "s[name] ?r[reason]", CFGFLAG_CLIENT, ConTempHelper, this, "temporary Helper");
	Console()->Register("deltemphelper", "s[name]", CFGFLAG_CLIENT, ConUnTempHelper, this, "remove temporary Helper");

	Console()->Register("addtempmute", "s[name] ?r[reason]", CFGFLAG_CLIENT, ConTempMute, this, "temporary Mute");
	Console()->Register("deltempmute", "s[name]", CFGFLAG_CLIENT, ConUnTempMute, this, "remove temporary Mute");
}

void CPlayerRelations::ConTempWar(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CPlayerRelations *>(pUserData)->TempWar(pResult->GetString(0), pResult->GetString(1));
}

void CPlayerRelations::ConUnTempWar(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CPlayerRelations *>(pUserData)->UnTempWar(pResult->GetString(0));
}

void CPlayerRelations::ConTempHelper(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CPlayerRelations *>(pUserData)->TempHelper(pResult->GetString(0), pResult->GetString(1));
}

void CPlayerRelations::ConUnTempHelper(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CPlayerRelations *>(pUserData)->UnTempHelper(pResult->GetString(0));
}

void CPlayerRelations::ConTempMute(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CPlayerRelations *>(pUserData)->TempMute(pResult->GetString(0));
}

void CPlayerRelations::ConUnTempMute(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CPlayerRelations *>(pUserData)->UnTempMute(pResult->GetString(0));
}
