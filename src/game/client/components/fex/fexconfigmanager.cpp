#include "fexconfigmanager.h"

#include <base/system.h>

#include <engine/serverbrowser.h>
#include <engine/shared/config.h>

#include <algorithm>

#include "fexskinprofiles.h"
#include "fexwarlist.h"

CFeXConfigManager g_FeXConfigManager;

namespace {
struct SListData
{
	std::vector<CFeXConfigManager::SEntry> *m_pOut;
	bool m_WantFolders;
	const char *m_pExtension;
};

bool EndsWithNoCase(const char *pStr, const char *pSuffix)
{
	const int StrLen = str_length(pStr);
	const int SuffixLen = str_length(pSuffix);
	if(SuffixLen <= 0 || StrLen <= SuffixLen)
		return false;
	return str_comp_nocase(pStr + StrLen - SuffixLen, pSuffix) == 0;
}

int ListCallback(const CFsFileInfo *pInfo, int IsDir, int StorageType, void *pUser)
{
	SListData *pData = static_cast<SListData *>(pUser);
	if(!pInfo || !pInfo->m_pName[0] || pInfo->m_pName[0] == '.')
		return 0;
	if((IsDir != 0) != pData->m_WantFolders)
		return 0;

	CFeXConfigManager::SEntry Entry;
	if(!IsDir && pData->m_pExtension && EndsWithNoCase(pInfo->m_pName, pData->m_pExtension))
	{
		char aBuf[sizeof(Entry.m_aName)];
		str_copy(aBuf, pInfo->m_pName, sizeof(aBuf));
		aBuf[str_length(aBuf) - str_length(pData->m_pExtension)] = '\0';
		str_copy(Entry.m_aName, aBuf, sizeof(Entry.m_aName));
	}
	else
		str_copy(Entry.m_aName, pInfo->m_pName, sizeof(Entry.m_aName));

	if(!Entry.m_aName[0])
		return 0;

	Entry.m_IsFolder = IsDir != 0;
	pData->m_pOut->push_back(Entry);
	return 0;
}

bool CompareEntry(const CFeXConfigManager::SEntry &Left, const CFeXConfigManager::SEntry &Right)
{
	return str_comp_nocase(Left.m_aName, Right.m_aName) < 0;
}
} // namespace

void CFeXConfigManager::SetError(char *pErr, int ErrSize, const char *pMsg)
{
	if(pErr && ErrSize > 0)
		str_copy(pErr, pMsg, ErrSize);
}

void CFeXConfigManager::Init(IStorage *pStorage)
{
	m_pStorage = pStorage;
	m_vDomains.clear();

	// Domains that already exist in the client. Registering a new one here (or
	// from anywhere else after init) is all it takes to expose its save slots.
	RegisterDomain("DDNet config", "ddnet", CONFIG_FILE);
	RegisterDomain("FeX config", "fex", FEX_CONFIG_FILE);
	RegisterDomain("War presets", "wars", WARLIST_FILE);
	RegisterDomain("Skin presets", "skins", PROFILES_FILE);
}

void CFeXConfigManager::RegisterDomain(const char *pName, const char *pFolder, const char *pSource, const char *pExtension)
{
	SDomain Domain;
	Domain.m_pName = pName;
	Domain.m_pFolder = pFolder;
	Domain.m_pSource = pSource;
	Domain.m_pExtension = pExtension;
	m_vDomains.push_back(Domain);

	if(m_pStorage)
	{
		char aPath[IO_MAX_PATH_LENGTH];
		str_format(aPath, sizeof(aPath), "%s/%s", FEX_CONFIG_LIBRARY, pFolder);
		m_pStorage->CreateFolder(aPath, IStorage::TYPE_SAVE);
	}
}

const char *CFeXConfigManager::DomainName(int Domain) const
{
	if(Domain < 0 || Domain >= (int)m_vDomains.size())
		return "";
	return m_vDomains[Domain].m_pName;
}

const char *CFeXConfigManager::DomainSource(int Domain) const
{
	if(Domain < 0 || Domain >= (int)m_vDomains.size())
		return "";
	return m_vDomains[Domain].m_pSource;
}

void CFeXConfigManager::DomainFolder(int Domain, const char *pFolder, char *pBuf, int Size) const
{
	const char *pDomainFolder = (Domain >= 0 && Domain < (int)m_vDomains.size()) ? m_vDomains[Domain].m_pFolder : "";
	if(pFolder && pFolder[0])
		str_format(pBuf, Size, "%s/%s/%s", FEX_CONFIG_LIBRARY, pDomainFolder, pFolder);
	else
		str_format(pBuf, Size, "%s/%s", FEX_CONFIG_LIBRARY, pDomainFolder);
}

void CFeXConfigManager::GetFolderPath(int Domain, const char *pFolder, char *pBuf, int Size) const
{
	DomainFolder(Domain, pFolder, pBuf, Size);
}

void CFeXConfigManager::EntryPath(int Domain, const char *pFolder, const char *pName, char *pBuf, int Size) const
{
	char aFolder[IO_MAX_PATH_LENGTH];
	DomainFolder(Domain, pFolder, aFolder, sizeof(aFolder));
	const char *pExtension = (Domain >= 0 && Domain < (int)m_vDomains.size()) ? m_vDomains[Domain].m_pExtension : "";
	if(pExtension && pExtension[0] && EndsWithNoCase(pName, pExtension))
		str_format(pBuf, Size, "%s/%s", aFolder, pName);
	else
		str_format(pBuf, Size, "%s/%s%s", aFolder, pName, pExtension ? pExtension : "");
}

void CFeXConfigManager::ListFolders(int Domain, const char *pFolder, std::vector<SEntry> &vOut) const
{
	vOut.clear();
	if(!m_pStorage)
		return;

	char aPath[IO_MAX_PATH_LENGTH];
	DomainFolder(Domain, pFolder, aPath, sizeof(aPath));
	SListData Data = {&vOut, true, nullptr};
	m_pStorage->ListDirectoryInfo(IStorage::TYPE_SAVE, aPath, ListCallback, &Data);
	std::sort(vOut.begin(), vOut.end(), CompareEntry);
}

void CFeXConfigManager::ListConfigs(int Domain, const char *pFolder, std::vector<SEntry> &vOut) const
{
	vOut.clear();
	if(!m_pStorage)
		return;

	char aPath[IO_MAX_PATH_LENGTH];
	DomainFolder(Domain, pFolder, aPath, sizeof(aPath));
	SListData Data = {&vOut, false, (Domain >= 0 && Domain < (int)m_vDomains.size()) ? m_vDomains[Domain].m_pExtension : ".cfg"};
	m_pStorage->ListDirectoryInfo(IStorage::TYPE_SAVE, aPath, ListCallback, &Data);
	std::sort(vOut.begin(), vOut.end(), CompareEntry);
}

bool CFeXConfigManager::CopyFile(const char *pFrom, const char *pTo, char *pErr, int ErrSize) const
{
	void *pData = nullptr;
	unsigned DataSize = 0;
	if(!m_pStorage->ReadFile(pFrom, IStorage::TYPE_SAVE, &pData, &DataSize) || !pData)
	{
		free(pData);
		SetError(pErr, ErrSize, "Failed to read the source file.");
		return false;
	}

	IOHANDLE File = m_pStorage->OpenFile(pTo, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
	{
		free(pData);
		SetError(pErr, ErrSize, "Failed to open the target file for writing.");
		return false;
	}

	const bool Ok = io_write(File, pData, DataSize) == DataSize;
	io_close(File);
	free(pData);
	if(!Ok)
	{
		SetError(pErr, ErrSize, "Failed to write the target file.");
		return false;
	}
	return true;
}

bool CFeXConfigManager::CreateFolder(int Domain, const char *pFolder, const char *pName, char *pErr, int ErrSize) const
{
	if(!m_pStorage)
		return false;
	if(!pName || !pName[0])
	{
		SetError(pErr, ErrSize, "The folder needs a name.");
		return false;
	}

	char aPath[IO_MAX_PATH_LENGTH];
	DomainFolder(Domain, pFolder, aPath, sizeof(aPath));
	char aNewPath[IO_MAX_PATH_LENGTH];
	str_format(aNewPath, sizeof(aNewPath), "%s/%s", aPath, pName);

	if(m_pStorage->FolderExists(aNewPath, IStorage::TYPE_SAVE))
	{
		SetError(pErr, ErrSize, "That folder already exists.");
		return false;
	}
	if(!m_pStorage->CreateFolder(aNewPath, IStorage::TYPE_SAVE))
	{
		SetError(pErr, ErrSize, "Could not create the folder.");
		return false;
	}
	return true;
}

bool CFeXConfigManager::Save(int Domain, const char *pFolder, const char *pName, char *pErr, int ErrSize) const
{
	if(!m_pStorage || Domain < 0 || Domain >= (int)m_vDomains.size())
		return false;
	if(!pName || !pName[0])
	{
		SetError(pErr, ErrSize, "The config needs a name.");
		return false;
	}

	const char *pSource = m_vDomains[Domain].m_pSource;
	if(!pSource || !pSource[0])
	{
		SetError(pErr, ErrSize, "This config domain has no source file.");
		return false;
	}

	char aFolder[IO_MAX_PATH_LENGTH];
	DomainFolder(Domain, pFolder, aFolder, sizeof(aFolder));
	m_pStorage->CreateFolder(aFolder, IStorage::TYPE_SAVE);

	char aTarget[IO_MAX_PATH_LENGTH];
	EntryPath(Domain, pFolder, pName, aTarget, sizeof(aTarget));
	return CopyFile(pSource, aTarget, pErr, ErrSize);
}

bool CFeXConfigManager::Load(int Domain, const char *pFolder, const char *pName, char *pErr, int ErrSize) const
{
	if(!m_pStorage || Domain < 0 || Domain >= (int)m_vDomains.size())
		return false;

	const char *pSource = m_vDomains[Domain].m_pSource;
	if(!pSource || !pSource[0])
	{
		SetError(pErr, ErrSize, "This config domain has no target file.");
		return false;
	}

	char aEntry[IO_MAX_PATH_LENGTH];
	EntryPath(Domain, pFolder, pName, aEntry, sizeof(aEntry));
	if(!m_pStorage->FileExists(aEntry, IStorage::TYPE_SAVE))
	{
		SetError(pErr, ErrSize, "That config does not exist.");
		return false;
	}
	return CopyFile(aEntry, pSource, pErr, ErrSize);
}

bool CFeXConfigManager::Rename(int Domain, const char *pFolder, const char *pOldName, const char *pNewName, bool IsFolder, char *pErr, int ErrSize) const
{
	if(!m_pStorage)
		return false;
	if(!pOldName || !pOldName[0] || !pNewName || !pNewName[0])
	{
		SetError(pErr, ErrSize, "Both names are required.");
		return false;
	}

	char aFolder[IO_MAX_PATH_LENGTH];
	DomainFolder(Domain, pFolder, aFolder, sizeof(aFolder));

	char aOld[IO_MAX_PATH_LENGTH];
	char aNew[IO_MAX_PATH_LENGTH];
	if(IsFolder)
	{
		str_format(aOld, sizeof(aOld), "%s/%s", aFolder, pOldName);
		str_format(aNew, sizeof(aNew), "%s/%s", aFolder, pNewName);
	}
	else
	{
		EntryPath(Domain, pFolder, pOldName, aOld, sizeof(aOld));
		EntryPath(Domain, pFolder, pNewName, aNew, sizeof(aNew));
	}

	if(!m_pStorage->FileExists(aOld, IStorage::TYPE_SAVE) && !m_pStorage->FolderExists(aOld, IStorage::TYPE_SAVE))
	{
		SetError(pErr, ErrSize, "The entry does not exist.");
		return false;
	}
	if(m_pStorage->FileExists(aNew, IStorage::TYPE_SAVE) || m_pStorage->FolderExists(aNew, IStorage::TYPE_SAVE))
	{
		SetError(pErr, ErrSize, "An entry with that name already exists.");
		return false;
	}

	if(IsFolder)
	{
		if(!m_pStorage->CreateFolder(aNew, IStorage::TYPE_SAVE) || !CopyFolderRecursive(aOld, aNew, 0) || !DeleteFolderRecursive(aOld, 0))
		{
			SetError(pErr, ErrSize, "Could not rename the folder.");
			return false;
		}
		return true;
	}

	if(!m_pStorage->RenameFile(aOld, aNew, IStorage::TYPE_SAVE))
	{
		SetError(pErr, ErrSize, "Could not rename the config.");
		return false;
	}
	return true;
}

bool CFeXConfigManager::DeleteFolderRecursive(const char *pPath, int Depth) const
{
	if(Depth > 4)
		return false;

	std::vector<SEntry> vEntries;
	SListData Data = {&vEntries, true, nullptr};
	m_pStorage->ListDirectoryInfo(IStorage::TYPE_SAVE, pPath, ListCallback, &Data);
	for(const SEntry &Entry : vEntries)
	{
		char aSub[IO_MAX_PATH_LENGTH];
		str_format(aSub, sizeof(aSub), "%s/%s", pPath, Entry.m_aName);
		DeleteFolderRecursive(aSub, Depth + 1);
	}

	vEntries.clear();
	Data = {&vEntries, false, nullptr};
	m_pStorage->ListDirectoryInfo(IStorage::TYPE_SAVE, pPath, ListCallback, &Data);
	bool Ok = true;
	for(const SEntry &Entry : vEntries)
	{
		char aFile[IO_MAX_PATH_LENGTH];
		str_format(aFile, sizeof(aFile), "%s/%s", pPath, Entry.m_aName);
		if(!m_pStorage->RemoveFile(aFile, IStorage::TYPE_SAVE))
			Ok = false;
	}
	return Ok;
}

bool CFeXConfigManager::CopyFolderRecursive(const char *pFrom, const char *pTo, int Depth) const
{
	if(Depth > 4)
		return false;

	std::vector<SEntry> vEntries;
	SListData Data = {&vEntries, false, nullptr};
	m_pStorage->ListDirectoryInfo(IStorage::TYPE_SAVE, pFrom, ListCallback, &Data);
	for(const SEntry &Entry : vEntries)
	{
		char aFrom[IO_MAX_PATH_LENGTH];
		char aTo[IO_MAX_PATH_LENGTH];
		str_format(aFrom, sizeof(aFrom), "%s/%s", pFrom, Entry.m_aName);
		str_format(aTo, sizeof(aTo), "%s/%s", pTo, Entry.m_aName);
		if(!CopyFile(aFrom, aTo, nullptr, 0))
			return false;
	}

	vEntries.clear();
	Data = {&vEntries, true, nullptr};
	m_pStorage->ListDirectoryInfo(IStorage::TYPE_SAVE, pFrom, ListCallback, &Data);
	for(const SEntry &Entry : vEntries)
	{
		char aFrom[IO_MAX_PATH_LENGTH];
		char aTo[IO_MAX_PATH_LENGTH];
		str_format(aFrom, sizeof(aFrom), "%s/%s", pFrom, Entry.m_aName);
		str_format(aTo, sizeof(aTo), "%s/%s", pTo, Entry.m_aName);
		m_pStorage->CreateFolder(aTo, IStorage::TYPE_SAVE);
		if(!CopyFolderRecursive(aFrom, aTo, Depth + 1))
			return false;
	}
	return true;
}

bool CFeXConfigManager::Delete(int Domain, const char *pFolder, const char *pName, bool IsFolder, char *pErr, int ErrSize) const
{
	if(!m_pStorage)
		return false;
	if(!pName || !pName[0])
	{
		SetError(pErr, ErrSize, "Nothing is selected.");
		return false;
	}

	char aFolder[IO_MAX_PATH_LENGTH];
	DomainFolder(Domain, pFolder, aFolder, sizeof(aFolder));

	if(IsFolder)
	{
		char aPath[IO_MAX_PATH_LENGTH];
		str_format(aPath, sizeof(aPath), "%s/%s", aFolder, pName);
		if(!m_pStorage->FolderExists(aPath, IStorage::TYPE_SAVE))
		{
			SetError(pErr, ErrSize, "The folder does not exist.");
			return false;
		}
		if(!DeleteFolderRecursive(aPath, 0))
		{
			SetError(pErr, ErrSize, "Could not delete every file in the folder.");
			return false;
		}
		return true;
	}

	char aPath[IO_MAX_PATH_LENGTH];
	EntryPath(Domain, pFolder, pName, aPath, sizeof(aPath));
	if(!m_pStorage->RemoveFile(aPath, IStorage::TYPE_SAVE))
	{
		SetError(pErr, ErrSize, "Could not delete the config.");
		return false;
	}
	return true;
}
