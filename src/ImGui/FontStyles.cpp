#include "FontStyles.h"

#include "Compatibility.h"
#include "SettingLoader.h"
#include "Util.h"

namespace ImGui
{
	void Font::LoadFontSettings(const CSimpleIniA& a_ini, const char* a_section)
	{
		name = a_ini.GetValue(a_section, "sFont", "");

		const auto resolutionScale = ModAPIHandler::GetSingleton()->GetResolutionScale();
		size = std::roundf((a_ini.GetLongValue(a_section, "iSize", 30)) * resolutionScale);

		spacing = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSpacing", -1.5));
	}

	void Font::LoadFont(ImFontConfig& config, const ImWchar* glyph_ranges)
	{
		if (name.empty() || font) {
			return;
		}

		name = R"(Data\Interface\ImGuiIcons\Fonts\)" + name;

		std::error_code ec;
		if (!std::filesystem::exists(name, ec)) {
			return;
		}

		const auto& io = ImGui::GetIO();
		config.GlyphExtraAdvanceX = spacing;
		font = io.Fonts->AddFontFromFileTTF(name.c_str(), size, &config, glyph_ranges);
	}

	void FontStyles::LoadFonts()
	{
		ImFontConfig config;
		primaryFont.LoadFont(config);
		config.MergeMode = true;
		secondaryFont.LoadFont(config);
		config.MergeMode = false;
		dragonFont.LoadFont(config);
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
		style.TextShadowOffset = ImVec2(1.50f, 1.50f);

		// load fonts
		SettingLoader::GetSingleton()->Load(FileType::kFonts, [&](auto& ini) {
			primaryFont.LoadFontSettings(ini, "PrimaryFont");
			secondaryFont.LoadFontSettings(ini, "SecondaryFont");
			dragonFont.LoadFontSettings(ini, "DragonFont");
		});

		style.ScaleAllSizes(ModAPIHandler::GetSingleton()->GetResolutionScale());

		LoadFonts();
	}
}
