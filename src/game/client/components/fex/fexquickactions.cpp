#include "fexquickactions.h"

#include <algorithm>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <game/client/components/hud.h>
#include <game/client/gameclient.h>
#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

static const int s_aActionKeys[MAX_ACTIONS_PER_PAGE] = {KEY_F, KEY_D, KEY_A};
static const char *s_apActionKeyNames[MAX_ACTIONS_PER_PAGE] = {"F", "D", "A"};

static void EscapeParam(char *pDst, const char *pSrc, int Size)
{
	str_escape(&pDst, pSrc, pDst + Size);
}

// Replaces {specid}, {specname} and {specclan} in a configured command.
static void ExpandCommand(const char *pCommand, int ClientId, CGameClient *pClient, char *pOut, int OutSize)
{
	char aId[16];
	str_format(aId, sizeof(aId), "%d", ClientId);

	const char *pName = "";
	const char *pClan = "";
	if(ClientId >= 0 && ClientId < MAX_CLIENTS)
	{
		pName = pClient->m_aClients[ClientId].m_aName;
		pClan = pClient->m_aClients[ClientId].m_aClan;
	}

	char aResult[ACTION_MAX_CMD * 2];
	str_copy(aResult, pCommand, sizeof(aResult));

	struct SPlaceholder
	{
		const char *m_pNeedle;
		const char *m_pValue;
	};
	const SPlaceholder aPlaceholders[] = {
		{"{specid}", aId},
		{"{specname}", pName},
		{"{specclan}", pClan},
	};

	for(const SPlaceholder &Placeholder : aPlaceholders)
	{
		const int NeedleLen = str_length(Placeholder.m_pNeedle);
		if(NeedleLen <= 0)
			continue;

		char aBuffer[sizeof(aResult)];
		int OutLen = 0;
		for(const char *pRead = aResult; *pRead && OutLen < (int)sizeof(aBuffer) - 1;)
		{
			if(str_comp_num(pRead, Placeholder.m_pNeedle, NeedleLen) == 0)
			{
				for(const char *pValue = Placeholder.m_pValue; *pValue && OutLen < (int)sizeof(aBuffer) - 1; pValue++)
					aBuffer[OutLen++] = *pValue;
				pRead += NeedleLen;
			}
			else
				aBuffer[OutLen++] = *pRead++;
		}
		aBuffer[OutLen] = '\0';
		str_copy(aResult, aBuffer, sizeof(aResult));
	}

	str_copy(pOut, aResult, OutSize);
}

CActions::CActions()
{
	InitDefaultPages();
}

void CActions::InitDefaultPages()
{
	ClearPages();

	struct SDefaultPage
	{
		const char *m_pName;
		const char *m_apActionNames[MAX_ACTIONS_PER_PAGE];
		const char *m_apCommands[MAX_ACTIONS_PER_PAGE];
	};

	const SDefaultPage aDefaults[] = {
		{"Player", {"War", "Team", "Helper"}, {"spec_war {specid}", "spec_team {specid}", "spec_helper {specid}"}},
		{"Clan", {"Clan War", "Clan Team", "Clan Helper"}, {"spec_clanwar {specid}", "spec_clanteam {specid}", "spec_clanhelper {specid}"}},
		{"General", {"Vote", "Friend", "Mute"}, {"spec_vote {specid}", "spec_friend {specid}", "spec_mute {specid}"}},
	};

	for(const SDefaultPage &Default : aDefaults)
	{
		if(m_NumPages >= MAX_PAGES)
			break;

		SPage &Page = m_aPages[m_NumPages];
		str_copy(Page.m_aName, Default.m_pName);
		Page.m_NumActions = MAX_ACTIONS_PER_PAGE;
		for(int i = 0; i < MAX_ACTIONS_PER_PAGE; i++)
		{
			str_copy(Page.m_aActions[i].m_aName, Default.m_apActionNames[i]);
			str_copy(Page.m_aActions[i].m_aCommand, Default.m_apCommands[i]);
		}
		m_NumPages++;
	}
}

void CActions::ClearPages()
{
	for(SPage &Page : m_aPages)
		Page = SPage();
	m_NumPages = 0;
}

SPage *CActions::Page(int Index)
{
	if(Index < 0 || Index >= m_NumPages)
		return nullptr;
	return &m_aPages[Index];
}

const SPage *CActions::Page(int Index) const
{
	if(Index < 0 || Index >= m_NumPages)
		return nullptr;
	return &m_aPages[Index];
}

int CActions::FindPage(const char *pName) const
{
	if(!pName || pName[0] == '\0')
		return -1;
	for(int i = 0; i < m_NumPages; i++)
	{
		if(str_comp(m_aPages[i].m_aName, pName) == 0)
			return i;
	}
	return -1;
}

bool CActions::AddPage(const char *pPageName)
{
	if(!pPageName || pPageName[0] == '\0' || m_NumPages >= MAX_PAGES)
		return false;
	if(FindPage(pPageName) >= 0)
		return false;

	SPage &Page = m_aPages[m_NumPages];
	str_copy(Page.m_aName, pPageName);
	Page.m_NumActions = 0;
	m_NumPages++;
	return true;
}

bool CActions::RemovePage(int PageIndex)
{
	// Keep at least one page: an empty menu has nothing to show.
	if(PageIndex < 0 || PageIndex >= m_NumPages || m_NumPages <= 1)
		return false;

	for(int i = PageIndex; i < m_NumPages - 1; i++)
		m_aPages[i] = m_aPages[i + 1];
	m_aPages[m_NumPages - 1] = SPage();
	m_NumPages--;

	if(m_Page >= m_NumPages)
		m_Page = m_NumPages - 1;
	return true;
}

bool CActions::AddAction(int PageIndex, const char *pName, const char *pCommand)
{
	SPage *pPage = Page(PageIndex);
	if(!pPage || pPage->m_NumActions >= MAX_ACTIONS_PER_PAGE)
		return false;

	SAction &Action = pPage->m_aActions[pPage->m_NumActions];
	// The settings page edits the slot before adding it, so only fill in
	// defaults when nothing was typed.
	if(pName && pName[0] != '\0')
		str_copy(Action.m_aName, pName);
	else if(Action.m_aName[0] == '\0')
	{
		char aBuf[ACTION_MAX_NAME];
		str_format(aBuf, sizeof(aBuf), "Action %d", pPage->m_NumActions + 1);
		str_copy(Action.m_aName, aBuf);
	}
	if(pCommand && pCommand[0] != '\0')
		str_copy(Action.m_aCommand, pCommand);

	pPage->m_NumActions++;
	return true;
}

bool CActions::RemoveAction(int PageIndex, int ActionIndex)
{
	SPage *pPage = Page(PageIndex);
	if(!pPage || ActionIndex < 0 || ActionIndex >= pPage->m_NumActions)
		return false;

	for(int i = ActionIndex; i < pPage->m_NumActions - 1; i++)
		pPage->m_aActions[i] = pPage->m_aActions[i + 1];
	pPage->m_aActions[pPage->m_NumActions - 1] = SAction();
	pPage->m_NumActions--;
	return true;
}

int CActions::SpectatorId() const
{
	if(!m_pClient->m_Snap.m_SpecInfo.m_Active || m_pClient->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW)
		return -1;
	const int Id = m_pClient->m_Snap.m_SpecInfo.m_SpectatorId;
	return Id >= 0 && Id < MAX_CLIENTS ? Id : -1;
}

void CActions::Notify(const char *pText, ColorRGBA Color)
{
	Color.a = 0.4f;
	m_pClient->m_Hud.ShowNotification(pText, 3.0f, 1.0f, Color, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
}

void CActions::ToggleWar(int ClientId, int WarType, bool Clan)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;
	if(WarType < WAR_TYPE_ENEMY || WarType > WAR_TYPE_HELPER)
		return;

	const char *pName = m_pClient->m_aClients[ClientId].m_aName;
	const char *pClan = m_pClient->m_aClients[ClientId].m_aClan;
	const char *pTypeName = WarType == WAR_TYPE_ENEMY ? "enemy" : WarType == WAR_TYPE_TEAM ? "team" : "helper";

	if(Clan && pClan[0] == '\0')
	{
		Notify("This player has no clan", ColorRGBA(0.9f, 0.3f, 0.3f, 1.0f));
		return;
	}

	const char *pTargetName = Clan ? "" : pName;
	const char *pTargetClan = Clan ? pClan : "";
	const char *pTarget = Clan ? pClan : pName;

	CWarList &WarList = m_pClient->m_WarList;
	char aBuf[256];
	if(WarList.IsInList(pTargetName, pTargetClan, pTypeName))
	{
		WarList.RemoveWarEntry(pTargetName, pTargetClan);
		str_format(aBuf, sizeof(aBuf), "Removed %s%s from the %s list", Clan ? "clan " : "", pTarget, pTypeName);
	}
	else
	{
		WarList.AddWarEntry(pTargetName, pTargetClan, "", pTypeName);
		str_format(aBuf, sizeof(aBuf), "Added %s%s to the %s list", Clan ? "clan " : "", pTarget, pTypeName);
	}
	Notify(aBuf, WarList.m_WarTypes[WarType]->m_Color);
}

void CActions::ToggleFriend(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;

	const char *pName = m_pClient->m_aClients[ClientId].m_aName;
	const char *pClan = m_pClient->m_aClients[ClientId].m_aClan;

	char aBuf[256];
	if(m_pClient->Friends()->IsFriend(pName, pClan, true))
	{
		m_pClient->Friends()->RemoveFriend(pName, pClan);
		str_format(aBuf, sizeof(aBuf), "Removed friend %s", pName);
		Notify(aBuf, ColorRGBA(0.9f, 0.2f, 0.2f, 1.0f));
	}
	else
	{
		m_pClient->Friends()->AddFriend(pName, pClan);
		str_format(aBuf, sizeof(aBuf), "Added friend %s", pName);
		Notify(aBuf, ColorRGBA(0.2f, 0.9f, 0.2f, 1.0f));
	}
}

void CActions::ToggleMute(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;

	const char *pName = m_pClient->m_aClients[ClientId].m_aName;
	char aBuf[256];
	if(m_pClient->m_aClients[ClientId].m_IsMute)
	{
		m_pClient->m_WarList.DelMute(pName);
		str_format(aBuf, sizeof(aBuf), "Removed mute from %s", pName);
		Notify(aBuf, ColorRGBA(0.9f, 0.2f, 0.2f, 1.0f));
	}
	else
	{
		m_pClient->m_WarList.AddMute(pName);
		str_format(aBuf, sizeof(aBuf), "Muted %s", pName);
		Notify(aBuf, ColorRGBA(0.7f, 0.7f, 0.2f, 1.0f));
	}
}

void CActions::VoteKick(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;

	m_pClient->m_Voting.CallvoteKick(ClientId, g_Config.m_ClSpectatorActionVoteReason);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Vote started against %s", m_pClient->m_aClients[ClientId].m_aName);
	Notify(aBuf, ColorRGBA(0.9f, 0.2f, 0.2f, 1.0f));
}

bool CActions::ExecutePageAction(int ActionIndex)
{
	const SPage *pPage = Page(m_Page);
	if(!pPage || ActionIndex < 0 || ActionIndex >= pPage->m_NumActions)
		return false;

	const SAction &Action = pPage->m_aActions[ActionIndex];
	if(Action.m_aCommand[0] == '\0')
		return false;

	const int Id = SpectatorId();
	if(Id < 0)
		return false;

	char aCommand[ACTION_MAX_CMD * 2];
	ExpandCommand(Action.m_aCommand, Id, m_pClient, aCommand, sizeof(aCommand));
	if(aCommand[0] == '\0')
		return false;

	Console()->ExecuteLine(aCommand);
	return true;
}

void CActions::OpenMenu()
{
	if(m_NumPages <= 0)
		return;
	if(m_Page < 0 || m_Page >= m_NumPages)
		m_Page = 0;
	m_MenuActive = true;
	m_MenuClosing = false;
	m_OpenTime = time_get();
}

void CActions::CloseMenu()
{
	m_MenuActive = false;
	if(g_Config.m_ClSpectatorActionAnimation == 0)
	{
		m_MenuClosing = false;
		return;
	}
	m_MenuClosing = true;
	m_CloseTime = time_get();
}

void CActions::NextPage()
{
	if(m_NumPages <= 0)
		return;
	m_Page = (m_Page + 1) % m_NumPages;
}

bool CActions::OnInput(const IInput::CEvent &Event)
{
	if(!g_Config.m_ClSpectatorActionHud || SpectatorId() < 0)
		return false;
	if(!(Event.m_Flags & IInput::FLAG_PRESS))
		return false;

	if(Event.m_Key == KEY_W)
	{
		if(m_MenuActive && !m_MenuClosing)
			CloseMenu();
		else
			OpenMenu();
		return true;
	}

	if(!m_MenuActive || m_MenuClosing)
		return false;

	if(Event.m_Key == KEY_S)
	{
		NextPage();
		return true;
	}

	for(int i = 0; i < MAX_ACTIONS_PER_PAGE; i++)
	{
		if(Event.m_Key != s_aActionKeys[i])
			continue;

		// Only these three keys belong to the menu; everything else stays
		// available to binds and chat while it is open.
		if(ExecutePageAction(i))
			CloseMenu();
		return true;
	}

	return false;
}

void CActions::OnReset()
{
	m_MenuActive = false;
	m_MenuClosing = false;
	m_OpenTime = 0;
	m_CloseTime = 0;
	m_Page = 0;
}

void CActions::OnConsoleInit()
{
	IConfigManager *pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	if(pConfigManager)
		pConfigManager->RegisterFXCallback(ConfigSaveCallback, this);

	Console()->Register("add_action_page", "s[page_name]", CFGFLAG_CLIENT, ConAddPage, this, "Adds a quick actions page");
	Console()->Register("remove_action_page", "s[page_name]", CFGFLAG_CLIENT, ConRemovePage, this, "Removes a quick actions page");
	Console()->Register("add_action", "s[page_name] s[action_name] s[action_command]", CFGFLAG_CLIENT, ConAddAction, this, "Adds an action to a quick actions page");
	Console()->Register("remove_action", "s[page_name] s[action_name]", CFGFLAG_CLIENT, ConRemoveAction, this, "Removes an action from a quick actions page");

	// Actions usable from the quick actions menu. They take a client id, which
	// the {specid} placeholder fills in with the spectated player.
	Console()->Register("spec_war", "i[client_id]", CFGFLAG_CLIENT, ConSpecWar, this, "Toggle enemy war list entry for a client");
	Console()->Register("spec_team", "i[client_id]", CFGFLAG_CLIENT, ConSpecTeam, this, "Toggle team list entry for a client");
	Console()->Register("spec_helper", "i[client_id]", CFGFLAG_CLIENT, ConSpecHelper, this, "Toggle helper list entry for a client");
	Console()->Register("spec_clanwar", "i[client_id]", CFGFLAG_CLIENT, ConSpecClanWar, this, "Toggle enemy war list entry for a client's clan");
	Console()->Register("spec_clanteam", "i[client_id]", CFGFLAG_CLIENT, ConSpecClanTeam, this, "Toggle team list entry for a client's clan");
	Console()->Register("spec_clanhelper", "i[client_id]", CFGFLAG_CLIENT, ConSpecClanHelper, this, "Toggle helper list entry for a client's clan");
	Console()->Register("spec_vote", "i[client_id]", CFGFLAG_CLIENT, ConSpecVote, this, "Start a kick vote against a client");
	Console()->Register("spec_friend", "i[client_id]", CFGFLAG_CLIENT, ConSpecFriend, this, "Toggle friend status for a client");
	Console()->Register("spec_mute", "i[client_id]", CFGFLAG_CLIENT, ConSpecMute, this, "Toggle mute for a client");

	LoadFromFile();
	OnReset();
}

void CActions::LoadFromFile()
{
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	if(!m_pStorage)
		return;

	IOHANDLE File = m_pStorage->OpenFile(ACTIONS_FILE, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return; // no config yet, keep the defaults from the constructor

	io_close(File);

	// The file rebuilds the page list from scratch.
	ClearPages();
	Console()->ExecuteFile(ACTIONS_FILE);
	if(m_NumPages <= 0)
		InitDefaultPages();
}

void CActions::WriteLine(const char *pLine)
{
	if(!m_ActionsFile)
		return;
	if(io_write(m_ActionsFile, pLine, str_length(pLine)) != static_cast<unsigned>(str_length(pLine)))
		return;
	io_write_newline(m_ActionsFile);
}

void CActions::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CActions *pThis = static_cast<CActions *>(pUserData);
	if(!pThis->m_pStorage)
		pThis->m_pStorage = pThis->Kernel()->RequestInterface<IStorage>();
	if(!pThis->m_pStorage)
		return;

	pThis->m_ActionsFile = pThis->m_pStorage->OpenFile(ACTIONS_FILE, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!pThis->m_ActionsFile)
	{
		dbg_msg("config", "ERROR: opening %s failed", ACTIONS_FILE);
		return;
	}

	bool Failed = false;
	for(int i = 0; i < pThis->m_NumPages; i++)
	{
		const SPage &Page = pThis->m_aPages[i];

		char aName[ACTION_PAGE_MAX_NAME * 2];
		EscapeParam(aName, Page.m_aName, sizeof(aName));

		char aBuf[ACTION_PAGE_MAX_NAME * 2 + 32];
		str_format(aBuf, sizeof(aBuf), "add_action_page \"%s\"", aName);
		pThis->WriteLine(aBuf);

		for(int j = 0; j < Page.m_NumActions; j++)
		{
			const SAction &Action = Page.m_aActions[j];

			char aActionName[ACTION_MAX_NAME * 2];
			char aActionCommand[ACTION_MAX_CMD * 2];
			EscapeParam(aActionName, Action.m_aName, sizeof(aActionName));
			EscapeParam(aActionCommand, Action.m_aCommand, sizeof(aActionCommand));

			char aLine[ACTION_PAGE_MAX_NAME * 2 + ACTION_MAX_NAME * 2 + ACTION_MAX_CMD * 2 + 64];
			str_format(aLine, sizeof(aLine), "add_action \"%s\" \"%s\" \"%s\"", aName, aActionName, aActionCommand);
			pThis->WriteLine(aLine);
		}
	}

	if(io_sync(pThis->m_ActionsFile) != 0)
		Failed = true;
	if(io_close(pThis->m_ActionsFile) != 0)
		Failed = true;
	pThis->m_ActionsFile = {};

	if(Failed)
		dbg_msg("config", "ERROR: writing to %s failed", ACTIONS_FILE);
}

void CActions::ConAddPage(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->AddPage(pResult->GetString(0));
}

void CActions::ConRemovePage(IConsole::IResult *pResult, void *pUserData)
{
	CActions *pThis = static_cast<CActions *>(pUserData);
	pThis->RemovePage(pThis->FindPage(pResult->GetString(0)));
}

void CActions::ConAddAction(IConsole::IResult *pResult, void *pUserData)
{
	CActions *pThis = static_cast<CActions *>(pUserData);
	const int PageIndex = pThis->FindPage(pResult->GetString(0));
	if(PageIndex >= 0)
		pThis->AddAction(PageIndex, pResult->GetString(1), pResult->GetString(2));
}

void CActions::ConRemoveAction(IConsole::IResult *pResult, void *pUserData)
{
	CActions *pThis = static_cast<CActions *>(pUserData);
	const int PageIndex = pThis->FindPage(pResult->GetString(0));
	if(PageIndex < 0)
		return;
	const SPage *pPage = pThis->Page(PageIndex);
	const char *pActionName = pResult->GetString(1);
	for(int i = 0; pPage && i < pPage->m_NumActions; i++)
	{
		if(str_comp(pPage->m_aActions[i].m_aName, pActionName) == 0)
		{
			pThis->RemoveAction(PageIndex, i);
			return;
		}
	}
}

void CActions::ConSpecWar(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleWar(pResult->GetInteger(0), WAR_TYPE_ENEMY, false);
}

void CActions::ConSpecTeam(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleWar(pResult->GetInteger(0), WAR_TYPE_TEAM, false);
}

void CActions::ConSpecHelper(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleWar(pResult->GetInteger(0), WAR_TYPE_HELPER, false);
}

void CActions::ConSpecClanWar(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleWar(pResult->GetInteger(0), WAR_TYPE_ENEMY, true);
}

void CActions::ConSpecClanTeam(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleWar(pResult->GetInteger(0), WAR_TYPE_TEAM, true);
}

void CActions::ConSpecClanHelper(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleWar(pResult->GetInteger(0), WAR_TYPE_HELPER, true);
}

void CActions::ConSpecVote(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->VoteKick(pResult->GetInteger(0));
}

void CActions::ConSpecFriend(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleFriend(pResult->GetInteger(0));
}

void CActions::ConSpecMute(IConsole::IResult *pResult, void *pUserData)
{
	static_cast<CActions *>(pUserData)->ToggleMute(pResult->GetInteger(0));
}

float CActions::CachedTextWidth(int Slot, const char *pText, float Size)
{
	if(Slot < 0 || Slot >= NUM_TEXT_METRICS)
		return TextRender()->TextWidth(Size, pText, -1, -1.0f);

	STextMetric &Metric = m_aTextMetrics[Slot];
	if(Metric.m_Size != Size || str_comp(Metric.m_aText, pText) != 0)
	{
		str_copy(Metric.m_aText, pText);
		Metric.m_Size = Size;
		Metric.m_Width = TextRender()->TextWidth(Size, pText, -1, -1.0f);
	}
	return Metric.m_Width;
}

void CActions::RenderPage(vec2 Center, float Radius, int PageIndex, float Scale, float Alpha)
{
	const SPage *pPage = Page(PageIndex);
	if(!pPage || Alpha <= 0.0f || Scale <= 0.0f)
		return;

	const float OuterRadius = Radius * Scale;
	const float InnerRadius = 30.0f * Scale;
	const int NumActions = pPage->m_NumActions;

	// One draw batch for the backdrop and every sector: the old code began and
	// ended a quad batch per sector.
	Graphics()->TextureClear();
	Graphics()->BlendNormal();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.3f * Alpha);
	Graphics()->DrawCircle(Center.x, Center.y, OuterRadius, 48);

	for(int i = 0; i < NumActions; i++)
	{
		const float Angle = (i * 2.0f * pi / NumActions) - pi / 2.0f;
		const float NextAngle = ((i + 1) * 2.0f * pi / NumActions) - pi / 2.0f;

		const vec2 Pos1 = Center + vec2(cos(Angle) * InnerRadius, sin(Angle) * InnerRadius);
		const vec2 Pos2 = Center + vec2(cos(Angle) * OuterRadius, sin(Angle) * OuterRadius);
		const vec2 Pos3 = Center + vec2(cos(NextAngle) * OuterRadius, sin(NextAngle) * OuterRadius);
		const vec2 Pos4 = Center + vec2(cos(NextAngle) * InnerRadius, sin(NextAngle) * InnerRadius);

		ColorRGBA Color = GameClient()->m_WarList.m_WarTypes[i + 1]->m_Color;
		Color.a = 0.5f * Alpha;
		Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
		IGraphics::CFreeformItem Freeform(Pos1.x, Pos1.y, Pos2.x, Pos2.y, Pos3.x, Pos3.y, Pos4.x, Pos4.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	}
	Graphics()->QuadsEnd();

	const char *pCenterText = pPage->m_aName[0] != '\0' ? pPage->m_aName : "Actions";
	const float CenterFontSize = 10.0f * Scale;
	const float CenterWidth = CachedTextWidth(NUM_TEXT_METRICS - 1, pCenterText, CenterFontSize);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, Alpha);
	TextRender()->Text(Center.x - CenterWidth / 2.0f, Center.y - CenterFontSize / 2.0f, CenterFontSize, pCenterText, -1.0f);

	if(NumActions == 0)
	{
		const char *pEmpty = "no actions";
		const float EmptySize = 7.0f * Scale;
		const float EmptyWidth = CachedTextWidth(NUM_TEXT_METRICS - 2, pEmpty, EmptySize);
		TextRender()->Text(Center.x - EmptyWidth / 2.0f, Center.y + CenterFontSize, EmptySize, pEmpty, -1.0f);
	}

	for(int i = 0; i < NumActions; i++)
	{
		const float Angle = (i * 2.0f * pi / NumActions) - pi / 2.0f;
		const float MidAngle = Angle + pi / NumActions;
		const float LabelRadius = (OuterRadius + InnerRadius) / 2.0f;
		const vec2 LabelPos = Center + vec2(cos(MidAngle) * LabelRadius, sin(MidAngle) * LabelRadius);

		const char *pText = pPage->m_aActions[i].m_aName[0] != '\0' ? pPage->m_aActions[i].m_aName : "Empty";
		const float FontSize = 8.0f * Scale;

		// fx_spectator_action_icons: single-letter action names on the first
		// page are drawn as the war / team icons instead of text.
		if(g_Config.m_ClSpectatorActionIcons && PageIndex == 0 && str_length(pText) == 1)
		{
			const int ImageId = i == 1 ? IMAGE_TEAM_ICON : IMAGE_SWORD_ICON;
			const float IconSize = 16.0f * Scale;
			Graphics()->TextureSet(g_pData->m_aImages[ImageId].m_Id);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
			IGraphics::CQuadItem IconQuad(LabelPos.x - IconSize / 2.0f, LabelPos.y - IconSize / 2.0f, IconSize, IconSize);
			Graphics()->QuadsDrawTL(&IconQuad, 1);
			Graphics()->QuadsEnd();
		}
		else
		{
			const float TextWidth = CachedTextWidth(i, pText, FontSize);
			TextRender()->Text(LabelPos.x - TextWidth / 2.0f, LabelPos.y - FontSize / 2.0f, FontSize, pText, -1.0f);
		}

		const vec2 KeyPos = Center + vec2(cos(Angle) * (OuterRadius + 10.0f * Scale), sin(Angle) * (OuterRadius + 10.0f * Scale));
		Graphics()->DrawRect(KeyPos.x - 8.0f, KeyPos.y - 8.0f, 16.0f, 16.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.2f * Alpha), IGraphics::CORNER_ALL, 5.0f);

		const char *pKeyName = s_apActionKeyNames[i];
		const float KeyFontSize = 8.0f;
		const float KeyWidth = CachedTextWidth(MAX_ACTIONS_PER_PAGE + i, pKeyName, KeyFontSize);
		TextRender()->Text(KeyPos.x - KeyWidth / 2.0f, KeyPos.y - KeyFontSize / 2.0f, KeyFontSize, pKeyName, -1.0f);
	}

	TextRender()->TextColor(TextRender()->DefaultTextColor());
}

void CActions::RenderSpectatorActionMenu()
{
	if(!g_Config.m_ClSpectatorActionHud || SpectatorId() < 0)
		return;
	if(!m_MenuActive && !m_MenuClosing)
		return;
	if(m_NumPages <= 0)
		return;
	if(m_Page < 0 || m_Page >= m_NumPages)
		m_Page = 0;

	float Progress = 1.0f;
	if(g_Config.m_ClSpectatorActionAnimation != 0)
	{
		const int64_t Start = m_MenuClosing ? m_CloseTime : m_OpenTime;
		Progress = (time_get() - Start) / (float)time_freq() / 0.5f;
		if(m_MenuClosing)
		{
			Progress = 1.0f - Progress;
			if(Progress <= 0.0f)
			{
				// Finished fading out: stop drawing instead of leaving the last
				// frame on screen.
				m_MenuClosing = false;
				return;
			}
		}
		Progress = std::clamp(Progress, 0.0f, 1.0f);
	}

	const float Scale = g_Config.m_ClSpectatorActionAnimation == 2 ? 0.6f + 0.4f * Progress : 1.0f;
	const float Alpha = g_Config.m_ClSpectatorActionAnimation == 1 ? Progress : 1.0f;

	RenderPage(vec2(m_pClient->m_Hud.m_Width / 2.0f, m_pClient->m_Hud.m_Height / 2.0f), 80.0f, m_Page, Scale, Alpha);
}
