#pragma once

namespace Subtitles
{
	enum class Language
	{
		kNative = -1,

		kChinese = 0,
		kCzech,
		kEnglish,
		kFrench,
		kGerman,
		kItalian,
		kJapanese,
		kKorean,
		kPolish,
		kPortuguese,
		kRussian,
		kSpanish,

		kTotal
	};

	std::string to_string(Language lang);
	Language    to_language(const std::string& string);

	struct LanguageSetting
	{
		bool operator==(const LanguageSetting& rhs) const { return language == rhs.language; }
		bool operator==(Language rhs) const { return language == rhs; }

		bool LoadMCMSettings(CSimpleIniA& a_ini, const char* a_section, Language gameLanguage);

		Language      prevLanguage;
		Language      language;
		std::uint32_t maxCharsPerLine{ 150 };
	};

	class LocalizedSubtitles
	{
	public:
		void BuildLocalizedSubtitles();

		bool LoadMCMSettings(CSimpleIniA& a_ini);
		void PostMCMSettingsLoad();

		std::pair<std::string, std::string> GetTranslatedSubtitles(const char* a_localSubtitle);
		std::uint32_t                       GetMaxCharactersPrimary() const { return primaryLanguage.maxCharsPerLine; }
		std::uint32_t                       GetMaxCharactersSecondary() const { return secondaryLanguage.maxCharsPerLine; }

	private:
		using SubtitleID = uint64_t;  // hashed id (string id + mod index)

		using MultiSubtitleToIDMap = FlatMap<std::string, FlatSet<SubtitleID>>;
		using MultiIDToSubtitleMap = FlatMap<SubtitleID, FlatMap<Language, FlatSet<std::string>>>;

		using SubtitleToIDMap = FlatMap<std::string, SubtitleID>;
		using IDToSubtitleMap = FlatMap<SubtitleID, FlatMap<Language, std::string>>;

		void ReadILStringFiles(MultiSubtitleToIDMap& a_multiSubToID, MultiIDToSubtitleMap& a_multiIDToSub) const;
		void MergeDuplicateSubtitles(const MultiSubtitleToIDMap& a_multiSubToID, const MultiIDToSubtitleMap& a_multiIDToSub);

		// members
		Language        gameLanguage{ Language::kEnglish };
		LanguageSetting primaryLanguage;
		LanguageSetting secondaryLanguage;
		SubtitleToIDMap subtitleToID;
		IDToSubtitleMap idToSubtitle;
	};
}
