#ifndef GAME_CLIENT_COMPONENTS_FEX_PLAYERRELATIONS_H
#define GAME_CLIENT_COMPONENTS_FEX_PLAYERRELATIONS_H

#include <base/color.h>
#include <engine/console.h>
#include <engine/shared/protocol.h>
#include <game/client/component.h>
#include <vector>

class CWarList;

// How one client currently relates to the local player.
//
// This merges the two sources that used to be combined by hand at every call
// site: the persistent war list (names, clans, mutes) and the session-only
// temporary list (temp war, temp helper, temp mute). Consumers ask for the
// merged value and never touch either cache directly.
class SPlayerRelation
{
public:
	// from the persistent war list
	bool m_PersistentWar = false;
	bool m_PersistentTeam = false;
	bool m_PersistentHelper = false;
	bool m_PersistentMute = false;

	// from the session-only temporary list
	bool m_TempWar = false;
	bool m_TempHelper = false;
	bool m_TempMute = false;

	// persistent list membership, used for colouring and list bookkeeping
	bool m_IsWarName = false;
	bool m_IsWarClan = false;

	ColorRGBA m_NameColor = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	ColorRGBA m_ClanColor = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);

	// merged reason, the temporary entry wins when it has one
	const char *m_pReason = "";
	const char *m_pTempReason = "";

	bool AtWar() const { return m_PersistentWar || m_TempWar; }
	bool IsTeam() const { return m_PersistentTeam; }
	bool IsHelper() const { return m_PersistentHelper || m_TempHelper; }
	bool IsMuted() const { return m_PersistentMute || m_TempMute; }
};

// Owns the session-only temporary list and answers relation queries.
class CPlayerRelations : public CComponent
{
public:
	// merged view of one client
	SPlayerRelation Get(int ClientId) const;

	bool IsWar(int ClientId) const { return Get(ClientId).AtWar(); }
	bool IsHelper(int ClientId) const { return Get(ClientId).IsHelper(); }
	bool IsTeam(int ClientId) const { return Get(ClientId).IsTeam(); }
	bool IsMuted(int ClientId) const { return Get(ClientId).IsMuted(); }
	bool IsAnyList(int ClientId) const;
	const char *Reason(int ClientId) const { return Get(ClientId).m_pReason; }

	// Colour override from the temporary list. false when the client has no
	// temporary war/helper entry, in which case the caller keeps its colour.
	bool TempColor(int ClientId, ColorRGBA *pColor) const;

	// Whether the persistent list has a name or clan entry for this client.
	bool HasWarEntry(int ClientId) const;
	// Colour the persistent list would use (clan colour when the entry is a
	// clan-only match, name colour otherwise).
	ColorRGBA PriorityColor(int ClientId) const;

	// Temporary list, session only.
	void TempWar(const char *pName, const char *pReason, bool Silent = false);
	void TempHelper(const char *pName, const char *pReason, bool Silent = false);
	void TempMute(const char *pName, bool Silent = false);

	bool UnTempWar(const char *pName, bool Silent = false);
	bool UnTempHelper(const char *pName, bool Silent = false);
	bool UnTempMute(const char *pName, bool Silent = false);

	// Rebuilds the per-client temporary flags from the temporary entries.
	void Refresh();

	int Sizeof() const override { return sizeof(*this); }

private:
	// A temporary list entry names a player in exactly one of the lists, so the
	// type is explicit instead of being encoded in which string is filled.
	struct STempEntry
	{
		enum class EType
		{
			WAR,
			HELPER,
			MUTE,
		};

		EType m_Type = EType::WAR;
		char m_aName[MAX_NAME_LENGTH] = "";
		char m_aReason[128] = "";

		bool Matches(EType Type, const char *pName) const;
	};

	struct STempData
	{
		bool m_War = false;
		bool m_Helper = false;
		bool m_Mute = false;
		char m_aReason[128] = "";
	};

	std::vector<STempEntry> m_vTempEntries;
	STempData m_aTempPlayers[MAX_CLIENTS];

	void AddTempEntry(STempEntry::EType Type, const char *pName, const char *pReason);
	bool RemoveTempEntries(STempEntry::EType Type, const char *pName);

	static void ConTempWar(IConsole::IResult *pResult, void *pUserData);
	static void ConUnTempWar(IConsole::IResult *pResult, void *pUserData);
	static void ConTempHelper(IConsole::IResult *pResult, void *pUserData);
	static void ConUnTempHelper(IConsole::IResult *pResult, void *pUserData);
	static void ConTempMute(IConsole::IResult *pResult, void *pUserData);
	static void ConUnTempMute(IConsole::IResult *pResult, void *pUserData);

	void OnConsoleInit() override;
	void OnNewSnapshot() override;
};

#endif
