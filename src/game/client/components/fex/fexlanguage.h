#ifndef GAME_CLIENT_COMPONENTS_FEX_FEXLANGUAGE_H
#define GAME_CLIENT_COMPONENTS_FEX_FEXLANGUAGE_H

#include <map>
#include <string>
#include <vector>

class IStorage;

/**
 * Loads the FeX interface translations from data/fex/languages/<code>.json.
 *
 * Every file is a flat JSON object that maps the English text used in the
 * source to its translation. Keys that a file does not contain simply stay
 * English, so a translation can be partial while it is being written.
 *
 * Files are discovered at startup and the display name comes from the optional
 * "_language" key, so dropping a new json file into the directory adds a new
 * entry to the language selector without touching the code.
 */
class CFeXLanguage
{
public:
	void Init(IStorage *pStorage);
	void Reload();

	// Returns the translation for pText, or the English text (localized by the
	// vanilla table where possible) when no translation exists.
	const char *Translate(const char *pText) const;

	int NumLanguages() const { return (int)m_vLanguages.size(); }
	const char *NameAt(int Index) const;
	const char *CodeAt(int Index) const;
	int IndexOfCode(const char *pCode) const;

private:
	struct SLanguage
	{
		std::string m_Code;
		std::string m_Name;
	};

	void LoadActive();
	std::string ReadLanguageFile(const std::string &Name) const;
	std::string LanguageName(const std::string &Code) const;

	IStorage *m_pStorage = nullptr;
	std::vector<SLanguage> m_vLanguages;
	std::vector<std::string> m_vNames;
	std::map<std::string, std::string> m_Map;
};

extern CFeXLanguage g_FeXLanguage;

// Translate a FeX interface string. Falls back to the vanilla localization and
// finally to the untranslated English text.
const char *FeXLocalize(const char *pText);

#endif
