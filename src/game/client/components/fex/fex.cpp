#include <engine/client.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <engine/textrender.h>
#include <game/gamecore.h>
#include <game/client/components/chat.h>
#include <game/client/components/controls.h>
#include <game/client/gameclient.h>
#include <game/generated/protocol.h>
#include "fex.h"
#include <base/system.h>
#include <base/math.h>
#include <cmath>
#include <engine/serverbrowser.h>


int CFex::IdWithName(const char *pName)
{
	int ClientId;

	for(ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		if(str_comp(pName, GameClient()->m_aClients[ClientId].m_aName) == 0)
		{
			return ClientId;
		}
	}
	return -1;
}

void CFex::ConOnlineInfo(IConsole::IResult *pResult, void *pUserData)
{
	CFex *pSelf = (CFex *)pUserData;
	pSelf->OnlineInfo();
}

void CFex::ConPlayerInfo(IConsole::IResult *pResult, void *pUserData)
{
	CFex *pSelf = (CFex *)pUserData;
	pSelf->PlayerInfo(pResult->GetString(0));
}

void CFex::OnlineInfo(bool Integrate)
{
	char aBuf[512];
	char active[512];

	int NumberWars = 0;
	int NumberWarsAfk = 0;
	for(auto &Client : GameClient()->m_aClients)
	{
		const SPlayerRelation Relation = GameClient()->m_Relations.Get(Client.m_Id);
		bool War = Relation.m_PersistentWar;
		bool TempWar = Relation.m_TempWar;

		if(!Client.m_Active && GameClient()->m_Teams.Team(Client.m_Id) == 0)
			continue;

		if(Client.m_Id == m_pClient->m_Snap.m_LocalClientId)
			continue;

		if((War && !TempWar) || (!War && TempWar))
		{
			NumberWars++;
			if(Client.m_Afk)
				NumberWarsAfk++;
		}
	}

	int NumberTeams = 0;
	int NumberTeamsAfk = 0;
	for(auto &Client : GameClient()->m_aClients)
	{
		const SPlayerRelation Relation = GameClient()->m_Relations.Get(Client.m_Id);
		bool Team = Relation.m_PersistentTeam;

		if(!Client.m_Active && GameClient()->m_Teams.Team(Client.m_Id) == 0)
			continue;

		if(Client.m_Id == m_pClient->m_Snap.m_LocalClientId)
			continue;

		if(Team)
		{
			NumberTeams++;
			if(Client.m_Afk)
				NumberTeamsAfk++;
		}
	}

	int NumberHelpers = 0;
	int NumberHelpersAfk = 0;
	for(auto &Client : GameClient()->m_aClients)
	{
		const SPlayerRelation Relation = GameClient()->m_Relations.Get(Client.m_Id);
		bool Helper = Relation.m_PersistentHelper;
		bool TempHelper = Relation.m_TempHelper;

		if(!Client.m_Active && GameClient()->m_Teams.Team(Client.m_Id) == 0)
			continue;

		if(Client.m_Id == m_pClient->m_Snap.m_LocalClientId)
			continue;

		if((Helper && !TempHelper) || (!Helper && TempHelper))
		{
			NumberHelpers++;
			if(Client.m_Afk)
				NumberHelpersAfk++;
		}
	}

	int NumberMutes = 0;
	int NumberMutesAfk = 0;
	for(auto &Client : GameClient()->m_aClients)
	{
		const SPlayerRelation Relation = GameClient()->m_Relations.Get(Client.m_Id);
		bool Mute = Relation.m_PersistentMute;
		bool TempMute = Relation.m_TempMute;

		if(!Client.m_Active)
			continue;

		if(Client.m_Id == m_pClient->m_Snap.m_LocalClientId)
			continue;

		if((Mute && !TempMute) || (!Mute && TempMute))
		{
			NumberMutes++;
			if(Client.m_Afk)
				NumberMutesAfk++;
		}
	}

	str_format(aBuf, sizeof(aBuf), "│ [online] %d Teams | %d Wars | %d Helpers | %d Mutes", NumberTeams, NumberWars, NumberHelpers, NumberMutes);
	str_format(active, sizeof(active), "│ [active] %d Teams | %d Wars | %d Helpers | %d Mutes", NumberTeams - NumberTeamsAfk, NumberWars - NumberWarsAfk, NumberHelpers - NumberHelpersAfk, NumberMutes - NumberMutesAfk);
	if(!Integrate)
	{
		GameClient()->aMessage("╭──                  FeX Info");
		GameClient()->aMessage("│");
	}
	GameClient()->aMessage(aBuf);
	GameClient()->aMessage(active);
	if(!Integrate)
	{
		GameClient()->aMessage("│");
		GameClient()->aMessage("╰───────────────────────");
	}
}

void CFex::PlayerInfo(const char *pName)
{
	char aBuf[1024];
	str_format(aBuf, sizeof(aBuf), "Couldnt Find a Player With The Name \"%s\"", pName);
	if(IdWithName(pName) != -1)
	{
		int Id = IdWithName(pName);
		GameClient()->aMessage("╭──                  Player Info");
		GameClient()->aMessage("│");

		str_format(aBuf, sizeof(aBuf), "│ Name: %s", pName);
		GameClient()->aMessage(aBuf);
		str_format(aBuf, sizeof(aBuf), "│ Clan: %s", GameClient()->m_aClients[Id].m_aClan);
		GameClient()->aMessage(aBuf);
		GameClient()->aMessage("│");
		if(!GameClient()->m_aClients[Id].m_UseCustomColor)
		{
			str_format(aBuf, sizeof(aBuf), "│ Custom Color: %s", "No");
			GameClient()->aMessage(aBuf);
			GameClient()->aMessage("│");
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "│ Custom Color: %s", "Yes");
			GameClient()->aMessage(aBuf);
			str_format(aBuf, sizeof(aBuf), "│ Body Color: %d", GameClient()->m_aClients[Id].m_ColorBody);
			GameClient()->aMessage(aBuf);
			str_format(aBuf, sizeof(aBuf), "│ Feet Color: %d", GameClient()->m_aClients[Id].m_ColorFeet);
			GameClient()->aMessage(aBuf);
			GameClient()->aMessage("│");
		}
		str_format(aBuf, sizeof(aBuf), "│ Skin Name: %s", GameClient()->m_aClients[Id].m_aSkinName);
		GameClient()->aMessage(aBuf);
		GameClient()->aMessage("│");

		if(GameClient()->m_aClients[Id].m_AuthLevel > 0)
			str_format(aBuf, sizeof(aBuf), "│ Authed: Yes, Auth Level %d", GameClient()->m_aClients[Id].m_AuthLevel);
		else
			str_format(aBuf, sizeof(aBuf), "│ Authed: No", GameClient()->m_aClients[Id].m_AuthLevel);
		GameClient()->aMessage(aBuf);


		GameClient()->aMessage("│");
		GameClient()->aMessage("╰───────────────────────");
	}
	else
		GameClient()->aMessage(aBuf);
}

const char* CFex::NameFromId(int ID)
{
    return m_pClient->m_aClients[ID].m_aName;
}

void CFex::OnRender()
{
	// Handle auto join team
	if(g_Config.m_ClAutoJoinTeam > 0 && !m_AutoJoinTeamAttempted && Client()->State() == IClient::STATE_ONLINE)
	{
		if (g_Config.m_ClAutoJoinTeam == 0)
			return;
		bool AlreadyInTeam = false; // Initialize to false
		if(m_pClient->m_Snap.m_LocalClientId >= 0)
		{
			int LocalTeam = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientId].m_Team;
			if(LocalTeam != 0) // If not in spectator/freemode
			{
				AlreadyInTeam = true;
				m_AutoJoinTeamAttempted = true; // Prevent further attempts
				return;
			}
		}

		if(!AlreadyInTeam)
		{
			if(m_AutoJoinTeamTick == 0)
			{
				m_AutoJoinTeamTick = time_get() + time_freq() * g_Config.m_ClAutoJoinTeamDelay;
			}
			else if(time_get() > m_AutoJoinTeamTick)
			{
				int TargetTeam = g_Config.m_ClAutoJoinTeam;
				bool TeamFull = false;
				
				if(TargetTeam > 0)
				{
					int PlayersInTeam = 0;
					for(int i = 0; i < MAX_CLIENTS; i++)
					{
						if(m_pClient->m_aClients[i].m_Active && !m_pClient->m_aClients[i].m_Spec && m_pClient->m_Teams.Team(i) == TargetTeam)
							PlayersInTeam++;
					}
					
					if(PlayersInTeam > 0)
					{
						TeamFull = true;
						TargetTeam = (TargetTeam % 63) + 1;
					}
				}
				
				char teamswitch[128];
				str_format(teamswitch, sizeof(teamswitch), "/team %d", TargetTeam);
				m_pClient->m_Chat.SendChat(0, teamswitch);
				
				m_AutoJoinTeamAttempted = true;
				
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Auto joined team %d%s", 
					TargetTeam, 
					TeamFull ? " (original team was full)" : "");
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FeX(Autojoin)->", aBuf);

				if(g_Config.m_ClAutoJoinTeamLock)
				{
					char teamlock[128];
					str_copy(teamlock, "/lock", sizeof(teamlock));
					m_pClient->m_Chat.SendChat(0, teamlock);
					str_format(aBuf, sizeof(aBuf), "Locked team %d", TargetTeam); 
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FeX(Autojoin)->", aBuf);
				}

				m_AutoJoinTeamTick = 0;
			}
		}
	}

	// Handle auto join dummy
	if(g_Config.m_ClAutoJoinDummy && Client()->State() == IClient::STATE_ONLINE)
	{
		if(Client()->DummyConnected())
		{
			if(g_Config.m_ClAutoJoinDummySwitch && g_Config.m_ClDummy == 1 && !m_HasSwitchedBack)
			{
				g_Config.m_ClDummy = 0;
				m_HasSwitchedBack = true;
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "autojoin", "Switched back to main player");
			}
			return;
		}

		if(m_FirstDummyAttempted)
		{
			CheckPlayerLeave();
			m_FirstDummyAttempted = false;  
		}

		if(m_OnAttemptingDummyJoin)
		{
			if(m_AutoJoinDummyTick == 0)
			{
				m_AutoJoinDummyTick = time_get() + time_freq() * g_Config.m_ClAutoJoinDummyDelay;
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "autojoin", "Waiting %d seconds to attempt dummy join...", g_Config.m_ClAutoJoinDummyDelay);
			}
			else if(time_get() > m_AutoJoinDummyTick)
			{
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "autojoin", "Attempting to join with dummy");
				Console()->ExecuteLine("dummy_connect");

				m_FirstDummyAttempted = true;  

				if(g_Config.m_ClAutoJoinDummySwitch)
					g_Config.m_ClDummy = 0;

				m_AutoJoinDummyTick = time_get() + time_freq() * g_Config.m_ClAutoJoinDummyDelay;
			}
		}
	}


    if(Client()->State() != IClient::STATE_ONLINE)
    {
        m_AutoJoinTeamAttempted = false;
        m_AutoJoinDummyAttempted = false;
        m_AutoJoinTeamTick = 0;
        m_AutoJoinDummyTick = 0;
        m_ServerJustJoined = false;
		m_OnAttemptingDummyJoin = false;
		m_HasSwitchedBack = false;
    }
}

void CFex::CheckPlayerLeave()
{
    int CurrentPlayers = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(m_pClient->m_aClients[i].m_Active)
            CurrentPlayers++;
    }

    if(CurrentPlayers < m_LastPlayerCount)
    {
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Player left, attempting to join (Players: %d -> %d)", m_LastPlayerCount, CurrentPlayers);
        Console()->ExecuteLine("dummy_connect");
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "autojoin", aBuf);
    }

    m_LastPlayerCount = CurrentPlayers;
}

void CFex::OnConsoleInit()
{
	Console()->Register("onlineinfo", "", CFGFLAG_CLIENT, ConOnlineInfo, this, "Shows you how many people of default lists are on the current server");
	Console()->Register("playerinfo", "s[name]", CFGFLAG_CLIENT, ConPlayerInfo, this, "Get Info of a Player");


}
void CFex::OnInit()
{
	// On client load
	TextRender()->SetCustomFace(g_Config.m_ClCustomFont);

	// Auto	join variables
	m_AutoJoinTeamTick = 0;
	m_AutoJoinDummyTick = 0;
	m_LastPlayerCount = 0;
	m_AutoJoinTeamAttempted = false;
	m_AutoJoinDummyAttempted = false;
	m_ServerJustJoined = false;
	m_OnAttemptingDummyJoin = false;
	m_LastPlayerCount = 0;
	m_FirstTimerExpired = false;
	m_HasSwitchedBack = false;
	// m_LastNameChangeTick = 0;
    // m_NameJustChanged = false;
	// m_IsRandomName = false;
}

void CFex::OnStateChange(int NewState, int OldState)
{
	if(NewState == IClient::STATE_ONLINE && OldState != IClient::STATE_ONLINE)
    {
        // m_AutoJoinTeamAttempted = false;
        m_AutoJoinDummyAttempted = false;
		if(g_Config.m_ClAutoJoinDummy)
			m_OnAttemptingDummyJoin = true;
        m_ServerJustJoined = true;
    }
}


