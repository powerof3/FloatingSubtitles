#include "Localization.h"

#include "Compatibility.h"
#include "ImGui/FontStyles.h"
#include "RE.h"

namespace Subtitles
{
	std::string to_string(Language lang)
	{
		switch (lang) {
		case Language::kChinese:
			return "CHINESE";
		case Language::kCzech:
			return "CZECH";
		case Language::kFrench:
			return "FRENCH";
		case Language::kGerman:
			return "GERMAN";
		case Language::kItalian:
			return "ITALIAN";
		case Language::kJapanese:
			return "JAPANESE";
		case Language::kKorean:
			return "KOREAN";
		case Language::kPolish:
			return "POLISH";
		case Language::kPortuguese:
			return "PORTUGUESE";
		case Language::kRussian:
			return "RUSSIAN";
		case Language::kSpanish:
			return "SPANISH";
		default:
			return "ENGLISH";
		}
	}

	Language to_language(const std::string& string)
	{
		switch (string::const_hash(string)) {
		case "CHINESE"_h:
			return Language::kChinese;
		case "CZECH"_h:
			return Language::kCzech;
		case "FRENCH"_h:
			return Language::kFrench;
		case "GERMAN"_h:
			return Language::kGerman;
		case "ITALIAN"_h:
			return Language::kItalian;
		case "JAPANESE"_h:
			return Language::kJapanese;
		case "KOREAN"_h:
			return Language::kKorean;
		case "POLISH"_h:
			return Language::kPolish;
		case "PORTUGUESE"_h:
			return Language::kPortuguese;
		case "RUSSIAN"_h:
			return Language::kRussian;
		case "SPANISH"_h:
			return Language::kSpanish;
		default:
			return Language::kEnglish;
		}
	}

	bool LanguageSetting::LoadMCMSettings(CSimpleIniA& a_ini, const char* a_section, Language gameLanguage)
	{
		prevLanguage = language;
		auto lastMaxCharsPerLine = maxCharsPerLine;

		std::string langKey = std::string("iLanguage").append(a_section);
		language = static_cast<Language>(a_ini.GetLongValue("Settings", langKey.c_str(), std::to_underlying(language)));

		if (language == Language::kNative) {
			language = gameLanguage;
			a_ini.SetLongValue("Settings", langKey.c_str(), std::to_underlying(language));;
		}

		std::string maxCharKey = std::string("iMaxCharactersPerLine").append(a_section);
		maxCharsPerLine = a_ini.GetLongValue("Settings", maxCharKey.c_str(), maxCharsPerLine);

		return language != prevLanguage || maxCharsPerLine != lastMaxCharsPerLine;  // rebuild subtitles
	}

	bool LocalizedSubtitles::LoadMCMSettings(CSimpleIniA& a_ini)
	{
		bool rebuildSubs = false;
		rebuildSubs |= primaryLanguage.LoadMCMSettings(a_ini, "Primary", gameLanguage);
		rebuildSubs |= secondaryLanguage.LoadMCMSettings(a_ini, "Secondary", gameLanguage);
		return rebuildSubs;
	}

	void LocalizedSubtitles::PostMCMSettingsLoad()
	{
		if (primaryLanguage.language == Language::kNative) {
			primaryLanguage.language = gameLanguage;
			RE::DispatchStaticCall("MCM", "SetModSettingInt", RE::BSFixedString("FloatingSubtitles"), RE::BSFixedString("iLanguagePrimary:Settings"), std::to_underlying(gameLanguage));
		}
		if (secondaryLanguage.language == Language::kNative) {
			secondaryLanguage.language = gameLanguage;
			RE::DispatchStaticCall("MCM", "SetModSettingInt", RE::BSFixedString("FloatingSubtitles"), RE::BSFixedString("iLanguageSecondary:Settings"), std::to_underlying(gameLanguage));
		}
	}

	void LocalizedSubtitles::ReadILStringFiles(MultiSubtitleToIDMap& a_multiSubToID, MultiIDToSubtitleMap& a_multiIDToSub) const
	{
		const auto& ilStringMap = RE::GetILStringMap();
		for (const auto& [fileName, info] : ilStringMap) {
			auto mod = RE::TESDataHandler::GetSingleton()->LookupModByName(fileName);
			if (!mod) {
				continue;
			}

			std::string_view baseName = fileName;
			baseName.remove_suffix(4);  // remove ".esm"

			for (auto language : stl::enum_range(Language::kChinese, Language::kTotal)) {
				std::string                  path = std::format("STRINGS\\{}_{}.ILSTRINGS", baseName, to_string(language));
				RE::BSResourceNiBinaryStream stream(path.c_str());

				if (!stream.good() || stream.stream->totalSize < 8) {
					continue;
				}

				std::vector<std::byte> buffer(stream.stream->totalSize);
				stream.read(buffer.data(), static_cast<std::uint32_t>(buffer.size()));

				RE::ILStringTable stringTable(buffer);

				for (const auto& [stringID, offset] : stringTable.directory) {
					std::string str = stringTable.GetStringAtOffset(offset);
					if (str.empty() || string::is_only_space(str)) {
						continue;
					}
					auto hashedStringID = hash::szudzik_pair(mod->compileIndex, stringID);
					if (language == gameLanguage) {
						a_multiSubToID[str].emplace(hashedStringID);
					}
					a_multiIDToSub[hashedStringID][language].emplace(str);
				}
			}
		}
	}

	void LocalizedSubtitles::MergeDuplicateSubtitles(const MultiSubtitleToIDMap& a_multiSubToID, const MultiIDToSubtitleMap& a_multiIDToSub)
	{
		const auto pick_best_id = [&](const FlatSet<SubtitleID>& ids) {
			SubtitleID best = *ids.begin();

			std::size_t bestCount = std::numeric_limits<std::size_t>::max();
			std::size_t bestTotalLen = 0;

			for (SubtitleID id : ids) {
				const auto& langMap = a_multiIDToSub.at(id);

				std::size_t count = 0;
				std::size_t totalLen = 0;

				for (const auto& [lang, subs] : langMap) {
					count += subs.size();
					for (const auto& s : subs) {
						totalLen += s.size();
					}
				}

				if (count < bestCount || (count == bestCount && totalLen > bestTotalLen)) {
					best = id;
					bestCount = count;
					bestTotalLen = totalLen;
				}
			}
			return best;
		};

		const auto pick_best_subtitle = [&](SubtitleID bestID) {
			FlatMap<Language, std::string> singleStrings;
			for (auto& [lang, set] : a_multiIDToSub.at(bestID)) {
				if (!set.empty()) {
					singleStrings[lang] = *set.begin();  // take the first string
				}
			}
			return singleStrings;
		};

		for (auto& [subtitle, ids] : a_multiSubToID) {
			auto [it, result] = subtitleToID.try_emplace(subtitle, pick_best_id(ids));
			if (result) {
				idToSubtitle.try_emplace(it->second, pick_best_subtitle(it->second));
			}
		}
	}

	void LocalizedSubtitles::BuildLocalizedSubtitles()
	{
		gameLanguage = to_language(RE::GetINISettingString("sLanguage:General"));

		Timer timer;
		timer.start();

		MultiSubtitleToIDMap multiSubtitleToID;
		MultiIDToSubtitleMap multiIDToSubtitle;

		ReadILStringFiles(multiSubtitleToID, multiIDToSubtitle);
		MergeDuplicateSubtitles(multiSubtitleToID, multiIDToSubtitle);

		timer.end();

		logger::info("Parsing ILString files took {}", timer.duration());
	}

	std::pair<std::string, std::string> LocalizedSubtitles::GetTranslatedSubtitles(const char* a_localSubtitle)
	{
		const auto resolve_subtitle = [&](LanguageSetting& a_setting) -> std::string {
			if (a_setting == gameLanguage) {
				return a_localSubtitle;
			}

			auto idIt = subtitleToID.find(a_localSubtitle);
			if (idIt == subtitleToID.end()) {
				return {};
			}

			auto mapIt = idToSubtitle.find(idIt->second);
			if (mapIt == idToSubtitle.end()) {
				return {};
			}

			auto subtitleIt = mapIt->second.find(a_setting.language);
			if (subtitleIt == mapIt->second.end()) {
				return {};
			}

			return subtitleIt->second;
		};

		if (primaryLanguage == secondaryLanguage) {
			return {};
		}

		return { resolve_subtitle(primaryLanguage), resolve_subtitle(secondaryLanguage) };
	}
}
