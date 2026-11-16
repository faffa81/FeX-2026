#ifndef GAME_CLIENT_COMPONENTS_FEX_FEXQUICKACTIONS_H
#define GAME_CLIENT_COMPONENTS_FEX_FEXQUICKACTIONS_H

#include <base/color.h>
#include <base/system.h>
#include <engine/console.h>
#include <game/client/component.h>

#define ACTIONS_FILE "fex_quickactions.cfg"

class IConfigManager;
class IStorage;

enum
{
	ACTION_PAGE_MAX_NAME = 16,
	ACTION_MAX_NAME = 32,
	ACTION_MAX_CMD = 512,
	MAX_PAGES = 32,
	MAX_ACTIONS_PER_PAGE = 3,
	NUM_TEXT_METRICS = 8,
};

struct SAction
{
	char m_aName[ACTION_MAX_NAME] = "";
	char m_aCommand[ACTION_MAX_CMD] = "";
};

struct SPage
{
	char m_aName[ACTION_PAGE_MAX_NAME] = "";
	SAction m_aActions[MAX_ACTIONS_PER_PAGE];
	int m_NumActions = 0;
};

// The spectator quick actions menu: a radial menu of user-defined pages, each
// with up to three commands (F / D / A, S cycles pages, W opens and closes).
class CActions : public CComponent
{
public:
	CActions();

	int Sizeof() const override { return sizeof(*this); }
	bool OnInput(const IInput::CEvent &Event) override;
	void OnConsoleInit() override;
	void OnReset() override;

	int NumPages() const { return m_NumPages; }
	SPage *Page(int Index);
	const SPage *Page(int Index) const;
	int FindPage(const char *pName) const;

	bool AddPage(const char *pPageName);
	bool RemovePage(int PageIndex);
	bool AddAction(int PageIndex, const char *pName, const char *pCommand);
	bool RemoveAction(int PageIndex, int ActionIndex);

	void RenderSpectatorActionMenu();

	// Draws one page of the radial menu. Shared by the in-game HUD and the
	// settings preview, so the preview cannot drift from what is rendered.
	void RenderPage(vec2 Center, float Radius, int PageIndex, float Scale, float Alpha);

private:
	enum
	{
		WAR_TYPE_ENEMY = 1,
		WAR_TYPE_TEAM = 2,
		WAR_TYPE_HELPER = 3,
	};

	struct STextMetric
	{
		char m_aText[ACTION_MAX_NAME] = "";
		float m_Size = 0.0f;
		float m_Width = 0.0f;
	};

	SPage m_aPages[MAX_PAGES];
	int m_NumPages = 0;

	bool m_MenuActive = false;
	bool m_MenuClosing = false;
	int m_Page = 0;
	int64_t m_OpenTime = 0;
	int64_t m_CloseTime = 0;

	// TextWidth allocates a text container per call; the menu redraws every
	// frame, so remember the last measurement per slot.
	STextMetric m_aTextMetrics[NUM_TEXT_METRICS];

	IStorage *m_pStorage = nullptr;
	IOHANDLE m_ActionsFile = nullptr;

	void InitDefaultPages();
	void ClearPages();
	void LoadFromFile();

	void OpenMenu();
	void CloseMenu();
	void NextPage();
	bool ExecutePageAction(int ActionIndex);

	int SpectatorId() const;
	void ToggleWar(int ClientId, int WarType, bool Clan);
	void ToggleFriend(int ClientId);
	void ToggleMute(int ClientId);
	void VoteKick(int ClientId);
	void Notify(const char *pText, ColorRGBA Color);
	float CachedTextWidth(int Slot, const char *pText, float Size);

	void WriteLine(const char *pLine);
	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

	static void ConAddPage(IConsole::IResult *pResult, void *pUserData);
	static void ConRemovePage(IConsole::IResult *pResult, void *pUserData);
	static void ConAddAction(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveAction(IConsole::IResult *pResult, void *pUserData);

	static void ConSpecWar(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecHelper(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecClanWar(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecClanTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecClanHelper(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecVote(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecFriend(IConsole::IResult *pResult, void *pUserData);
	static void ConSpecMute(IConsole::IResult *pResult, void *pUserData);
};

#endif // GAME_CLIENT_COMPONENTS_FEX_FEXQUICKACTIONS_H
