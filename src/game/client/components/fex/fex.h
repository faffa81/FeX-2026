#ifndef GAME_CLIENT_COMPONENTS_FEX_FEX_H
#define GAME_CLIENT_COMPONENTS_FEX_FEX_H

#include <game/client/component.h>
#include <engine/console.h>
#include <base/system.h>

// Client-wide helpers: server info commands, player info and the auto-join
// logic. Player relations (war / helper / mute lists) live in
// CPlayerRelations, see playerrelations.h.
class CFex : public CComponent
{
public:
	bool IsSpecialClan(const char *pClanName);

	virtual void OnConsoleInit() override;
	virtual void OnRender() override;

	static void ConOnlineInfo(IConsole::IResult *pResult, void *pUserData);
	static void ConPlayerInfo(IConsole::IResult *pResult, void *pUserData);
	static void ConAutoMessage(IConsole::IResult *pResult, void *pUserData);
	void OnlineInfo(bool Integrate = false);
	int IdWithName(const char *pName);
	void PlayerInfo(const char *pName);

	const char *NameFromId(int ID);

	// FeX Autojoins
	bool m_FirstDummyAttempted;
	int64_t m_AutoJoinTeamTick;
	int64_t m_AutoJoinDummyTick;
	bool m_OnAttemptingDummyJoin;
	int m_LastPlayerCount;
	bool m_AutoJoinTeamAttempted;
	bool m_AutoJoinDummyAttempted;
	bool m_ServerJustJoined;
	int64_t m_AutoJoinDummySwitchTick;
	bool m_FirstTimerExpired = false;
	bool m_HasSwitchedBack = false;
	void CheckPlayerLeave();

private:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnStateChange(int NewState, int OldState) override;
};

#endif // FEX_H
