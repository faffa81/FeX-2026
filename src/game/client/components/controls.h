/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CONTROLS_H
#define GAME_CLIENT_COMPONENTS_CONTROLS_H

#include <base/vmath.h>

#include <engine/client.h>

#include <game/client/component.h>
#include <game/generated/protocol.h>

// Fast input modes. The mode decides which parts of the player input trigger
// an immediate reprediction instead of waiting for the next tick.
enum EFastInputMode
{
	FASTINPUT_OFF = 0,
	FASTINPUT_ALL,           // every input field, the original behaviour
	FASTINPUT_MOVEMENT,      // direction + jump
	FASTINPUT_COMBAT,        // hook + fire + weapon switch
	FASTINPUT_MOVEMENT_HOOK, // direction + jump + hook ("run and hook")
	FASTINPUT_HOOK,
	FASTINPUT_FIRE,
	FASTINPUT_WEAPONS,
	FASTINPUT_JUMP,
	FASTINPUT_PRESSES,       // every field, but releases do not trigger
	FASTINPUT_AIM,           // crosshair moved more than the configured angle
	FASTINPUT_AIM_COMBAT,    // crosshair + hook + fire + weapon switch
	NUM_FASTINPUT_MODES,
};

enum
{
	FASTINPUT_FIELD_DIRECTION = 1 << 0,
	FASTINPUT_FIELD_JUMP = 1 << 1,
	FASTINPUT_FIELD_HOOK = 1 << 2,
	FASTINPUT_FIELD_FIRE = 1 << 3,
	FASTINPUT_FIELD_WEAPON = 1 << 4,
	FASTINPUT_FIELD_AIM = 1 << 5,
};

const char *FastInputModeName(int Mode);
const char *FastInputModeDescription(int Mode);
// Which input fields a mode watches, 0 for off/unknown modes.
int FastInputModeMask(int Mode);

class CControls : public CComponent
{
public:
	float GetMinMouseDistance() const;
	float GetMaxMouseDistance() const;

	bool CheckNewInput();

	vec2 m_aMousePos[NUM_DUMMIES];
	vec2 m_aMousePosOnAction[NUM_DUMMIES];
	vec2 m_aTargetPos[NUM_DUMMIES];

	int m_aAmmoCount[NUM_WEAPONS];

	int64_t m_LastSendTime;
	CNetObj_PlayerInput m_aInputData[NUM_DUMMIES];
	CNetObj_PlayerInput m_aLastData[NUM_DUMMIES];
	int m_aInputDirectionLeft[NUM_DUMMIES];
	int m_aInputDirectionRight[NUM_DUMMIES];
	int m_aShowHookColl[NUM_DUMMIES];

	CControls();
	virtual int Sizeof() const override { return sizeof(*this); }

	virtual void OnReset() override;
	virtual void OnRender() override;
	virtual void OnMessage(int MsgType, void *pRawMsg) override;
	virtual bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
	virtual void OnConsoleInit() override;
	virtual void OnPlayerDeath();

	int SnapInput(int *pData);
	void ClampMousePos();
	void ResetInput(int Dummy);
	CNetObj_PlayerInput m_FastInput{};
};
#endif
