#ifndef GAME_CLIENT_COMPONENTS_FEX_FEXCONFIGMANAGER_H
#define GAME_CLIENT_COMPONENTS_FEX_FEXCONFIGMANAGER_H

#include <engine/storage.h>

#include <vector>

// A small config library: named save slots for the client configuration and
// for every FeX data file that has its own save routine.
//
// Everything lives under fex/configs/<domain>/ and can be organised in one
// level of user folders. A new save target only has to call RegisterDomain()
// with a display name, a folder name and the file it snapshots, so adding a
// config does not touch the library code.
#define FEX_CONFIG_LIBRARY "fex/configs"

class CFeXConfigManager
{
public:
	struct SDomain
	{
		const char *m_pName;
		const char *m_pFolder;
		const char *m_pSource;
		const char *m_pExtension;
	};

	struct SEntry
	{
		char m_aName[128];
		bool m_IsFolder;
	};

private:
	IStorage *m_pStorage = nullptr;
	std::vector<SDomain> m_vDomains;

	void DomainFolder(int Domain, const char *pFolder, char *pBuf, int Size) const;
	void EntryPath(int Domain, const char *pFolder, const char *pName, char *pBuf, int Size) const;
	bool CopyFile(const char *pFrom, const char *pTo, char *pErr, int ErrSize) const;
	bool DeleteFolderRecursive(const char *pPath, int Depth) const;
	bool CopyFolderRecursive(const char *pFrom, const char *pTo, int Depth) const;
	static void SetError(char *pErr, int ErrSize, const char *pMsg);

public:
	void Init(IStorage *pStorage);
	void RegisterDomain(const char *pName, const char *pFolder, const char *pSource, const char *pExtension = ".cfg");

	int NumDomains() const { return (int)m_vDomains.size(); }
	const char *DomainName(int Domain) const;
	const char *DomainSource(int Domain) const;
	void GetFolderPath(int Domain, const char *pFolder, char *pBuf, int Size) const;

	void ListFolders(int Domain, const char *pFolder, std::vector<SEntry> &vOut) const;
	void ListConfigs(int Domain, const char *pFolder, std::vector<SEntry> &vOut) const;

	bool CreateFolder(int Domain, const char *pFolder, const char *pName, char *pErr = nullptr, int ErrSize = 0) const;
	bool Save(int Domain, const char *pFolder, const char *pName, char *pErr = nullptr, int ErrSize = 0) const;
	bool Load(int Domain, const char *pFolder, const char *pName, char *pErr = nullptr, int ErrSize = 0) const;
	bool Rename(int Domain, const char *pFolder, const char *pOldName, const char *pNewName, bool IsFolder, char *pErr = nullptr, int ErrSize = 0) const;
	bool Delete(int Domain, const char *pFolder, const char *pName, bool IsFolder, char *pErr = nullptr, int ErrSize = 0) const;
};

extern CFeXConfigManager g_FeXConfigManager;

#endif
