#include "fexlanguage.h"

#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/json.h>
#include <engine/storage.h>

#include <game/localization.h>

#include <algorithm>
#include <cstdlib>
#include <set>

CFeXLanguage g_FeXLanguage;

static const char *s_pLanguageDir = "fex/languages";
static const char *s_pLanguageKey = "_language";
static const char *s_pFallbackCode = "english";

std::string CFeXLanguage::ReadLanguageFile(const std::string &Name) const
{
	if(!m_pStorage)
		return std::string();

	char aPath[IO_MAX_PATH_LENGTH];
	str_format(aPath, sizeof(aPath), "%s/%s.json", s_pLanguageDir, Name.c_str());
	void *pData = nullptr;
	unsigned DataSize = 0;
	if(!m_pStorage->ReadFile(aPath, IStorage::TYPE_ALL, &pData, &DataSize))
		return std::string();
	std::string Result((const char *)pData, DataSize);
	free(pData);
	return Result;
}

std::string CFeXLanguage::LanguageName(const std::string &Code) const
{
	const std::string Json = ReadLanguageFile(Code);
	if(Json.empty())
		return Code;

	json_value *pRoot = json_parse(Json.c_str(), Json.size());
	if(!pRoot)
		return Code;
	if(pRoot->type != json_object)
	{
		json_value_free(pRoot);
		return Code;
	}

	std::string Name = Code;
	if(const json_value *pName = json_object_get(pRoot, s_pLanguageKey))
	{
		if(pName->type == json_string && pName->u.string.length > 0)
			Name = json_string_get(pName);
	}
	json_value_free(pRoot);
	return Name;
}

void CFeXLanguage::Init(IStorage *pStorage)
{
	m_pStorage = pStorage;
	m_vLanguages.clear();
	m_vNames.clear();
	m_Map.clear();

	if(m_pStorage)
	{
		std::set<std::string> Entries;
		m_pStorage->FindFiles("*.json", s_pLanguageDir, IStorage::TYPE_ALL, &Entries);
		for(const std::string &Entry : Entries)
		{
			// FindFiles may hand back a path, only the file name matters here.
			std::string Code = Entry;
			size_t Sep = Code.find_last_of('/');
			if(Sep == std::string::npos)
				Sep = Code.find_last_of('\\');
			if(Sep != std::string::npos)
				Code = Code.substr(Sep + 1);
			const size_t Dot = Code.find_last_of('.');
			if(Dot != std::string::npos)
				Code = Code.substr(0, Dot);
			if(Code.empty())
				continue;

			SLanguage Language;
			Language.m_Code = Code;
			Language.m_Name = LanguageName(Code);
			m_vLanguages.push_back(Language);
		}
	}

	if(m_vLanguages.empty())
	{
		SLanguage Language;
		Language.m_Code = s_pFallbackCode;
		Language.m_Name = "English";
		m_vLanguages.push_back(Language);
	}

	// English first, the rest alphabetically by their display name.
	std::stable_sort(m_vLanguages.begin(), m_vLanguages.end(), [](const SLanguage &A, const SLanguage &B) {
		if((A.m_Code == s_pFallbackCode) != (B.m_Code == s_pFallbackCode))
			return A.m_Code == s_pFallbackCode;
		return str_comp_nocase(A.m_Name.c_str(), B.m_Name.c_str()) < 0;
	});

	for(const SLanguage &Language : m_vLanguages)
		m_vNames.push_back(Language.m_Name);

	LoadActive();
}

void CFeXLanguage::Reload()
{
	LoadActive();
}

void CFeXLanguage::LoadActive()
{
	m_Map.clear();
	if(!m_pStorage)
		return;

	const char *pCode = g_Config.m_ClFeXLanguage;
	if(!pCode[0] || !str_comp_nocase(pCode, s_pFallbackCode))
		return; // English needs no table, the source text is English

	const std::string Json = ReadLanguageFile(pCode);
	if(Json.empty())
		return;

	json_value *pRoot = json_parse(Json.c_str(), Json.size());
	if(!pRoot)
		return;
	if(pRoot->type != json_object)
	{
		json_value_free(pRoot);
		return;
	}

	for(unsigned i = 0; i < pRoot->u.object.length; i++)
	{
		const json_value *pValue = pRoot->u.object.values[i].value;
		if(!pValue || pValue->type != json_string)
			continue;
		const char *pKey = pRoot->u.object.values[i].name;
		if(!pKey || pKey[0] == '_')
			continue;
		m_Map[pKey] = json_string_get(pValue);
	}
	json_value_free(pRoot);
}

const char *CFeXLanguage::Translate(const char *pText) const
{
	if(!pText || !pText[0])
		return pText ? pText : "";

	if(!m_Map.empty())
	{
		const auto It = m_Map.find(pText);
		if(It != m_Map.end() && !It->second.empty())
			return It->second.c_str();
	}

	// Vanilla strings keep the translations that ship with the client.
	return Localize(pText);
}

const char *CFeXLanguage::NameAt(int Index) const
{
	if(Index < 0 || Index >= (int)m_vLanguages.size())
		return "English";
	return m_vLanguages[Index].m_Name.c_str();
}

const char *CFeXLanguage::CodeAt(int Index) const
{
	if(Index < 0 || Index >= (int)m_vLanguages.size())
		return s_pFallbackCode;
	return m_vLanguages[Index].m_Code.c_str();
}

int CFeXLanguage::IndexOfCode(const char *pCode) const
{
	if(!pCode || !pCode[0])
		return 0;
	for(int i = 0; i < (int)m_vLanguages.size(); i++)
		if(!str_comp_nocase(m_vLanguages[i].m_Code.c_str(), pCode))
			return i;
	return 0;
}

const char *FeXLocalize(const char *pText)
{
	return g_FeXLanguage.Translate(pText);
}
