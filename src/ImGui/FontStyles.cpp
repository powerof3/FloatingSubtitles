#include "FontStyles.h"

#include "BSFont.h"
#include "Compatibility.h"
#include "SettingLoader.h"

namespace ImGui
{
	void Font::LoadFontSettings(const CSimpleIniA& a_ini, const char* a_section)
	{
		name = a_ini.GetValue(a_section, "sFont", "");

		const auto resolutionScale = ModAPIHandler::GetSingleton()->GetResolutionScale();
		size = std::truncf((a_ini.GetLongValue(a_section, "iSize", 30)) * resolutionScale);

		spacing = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSpacing", 1.0));
	}

	void Font::LoadFont(ImFontConfig& config)
	{
		if (name.empty() || font) {
			return;
		}

		name = R"(Data\Interface\ImGuiIcons\Fonts\)" + name;

		const auto& io = ImGui::GetIO();
		config.GlyphExtraAdvanceX = spacing;
		font = io.Fonts->AddFontFromFileTTF(name.c_str(), size, &config);

		std::error_code ec;
		logger::info("Loaded font [{}|size: {}|spacing: {}] : {}", name, size, spacing, std::filesystem::exists(name, ec));
	}

	void FontStyles::LoadFonts()
	{
		ImFontConfig config;

		auto& io = ImGui::GetIO();
		if (UsingDefaultFont()) {
			logger::info("Using default font...");
			config.GlyphExtraAdvanceX = 1.0f;
			primaryFont.font = io.Fonts->AddFontFromMemoryCompressedTTF(BSFont_Data, BSFont_Size, std::truncf(28.0f * ModAPIHandler::GetSingleton()->GetResolutionScale()), &config);
		} else {
			primaryFont.LoadFont(config);
		}

		config.MergeMode = true;
		secondaryFont.LoadFont(config);

		config.MergeMode = false;
		dragonFont.LoadFont(config);

		io.FontDefault = primaryFont.font;
	}

	void FontStyles::PushDragonFont()
	{
		dragonFont.PushFont();
	}

	void FontStyles::LoadStyleSettings(CSimpleIniA& a_ini)
	{
#define GET_VALUE(a_value, a_section, a_key)                                                                                                        \
	bool a_value##_hex = false;                                                                                                                     \
	std::tie(user.a_value, a_value##_hex) = ToStyle<decltype(user.a_value)>(a_ini.GetValue(a_section, a_key, ToString(def.a_value, true).c_str())); \
	a_ini.SetValue(a_section, a_key, ToString(user.a_value, a_value##_hex).c_str());

		GET_VALUE(text, "Text", "rTextColor")
		GET_VALUE(shadowText, "Text", "rTextShadowColor")
		GET_VALUE(shadowOffsetVar, "Text", "fTextShadowOffset")

#undef GET_VALUE
	}

	void FontStyles::LoadFontStyles()
	{
		// load style
		SettingLoader::GetSingleton()->Load(FileType::kStyles, [&](auto& ini) { LoadStyleSettings(ini); });

		auto& style = GetStyle();
		auto& colors = style.Colors;

		colors[ImGuiCol_Text] = user.text;
		style.Colors[ImGuiCol_TextShadowDisabled] = user.shadowText;
		style.TextShadowOffset = { 2.0, 2.0 };

		// load fonts
		SettingLoader::GetSingleton()->Load(FileType::kFonts, [&](auto& ini) {
			primaryFont.LoadFontSettings(ini, "PrimaryFont");
			secondaryFont.LoadFontSettings(ini, "SecondaryFont");
			dragonFont.LoadFontSettings(ini, "DragonFont");
		});

		LoadFonts();

		style.ScaleAllSizes(ModAPIHandler::GetSingleton()->GetResolutionScale());
	}

	bool FontStyles::UsingDefaultFont()
	{
		RE::BSResourceNiBinaryStream fontConfig("sFontConfigFile:Fonts"_ini.value());
		if (!fontConfig.good()) {
			return false;
		}

		constexpr std::string_view target = "$EverywhereMediumFont"sv;

		std::string line;

		while (std::getline(fontConfig, line)) {
			if (line.contains(target)) {
				auto eq = line.find('=');
				auto q1 = line.find('"', eq);
				auto q2 = line.find('"', q1 + 1);
				auto font = line.substr(q1 + 1, q2 - q1 - 1);
				if (font == "Futura Condensed") {
					return true;
				}
			}
		}

		return false;
	}
}
