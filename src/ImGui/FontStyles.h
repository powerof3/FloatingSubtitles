#pragma once

namespace ImGui
{
	struct Font
	{
		void LoadFontSettings(const CSimpleIniA& a_ini, const char* a_section);
		void LoadFont(ImFontConfig& config);

		void PushFont() const
		{
			ImGui::PushFont(font, size);
		}

		std::string name{ "Jost-Medium.ttf" };
		float       size{ 35.0f };
		float       spacing{ 1.0f };
		ImFont*     font;
	};

	struct Style
	{
		ImVec4 text{ 0.984f, 0.984f, 0.984f, 1.0f };
		ImVec4 shadowText{ 1.0f, 1.0f, 1.0f, 1.0f };

		ImVec2 shadowOffset{ 3.0f, 3.0f };
		float  shadowOffsetVar{ 3.0f };
	};

	class FontStyles : public REX::Singleton<FontStyles>
	{
	public:
		void LoadStyleSettings(CSimpleIniA& a_ini);
		void LoadFontStyles();

		void LoadFonts();

		void PushDragonFont();

	private:
		template <class T>
		std::pair<T, bool> ToStyle(const std::string& a_str);
		template <class T>
		std::string ToString(const T& a_style, bool a_hex);

		// members
		Style def;
		Style user;

		Font primaryFont{};
		Font secondaryFont{};
		Font dragonFont{};

		static FontStyles instance;
	};

	template <class T>
	inline std::pair<T, bool> FontStyles::ToStyle(const std::string& a_str)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			static srell::regex rgb_pattern("([0-9]+),([0-9]+),([0-9]+),([0-9]+)");
			static srell::regex hex_pattern("#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");

			srell::smatch rgb_matches;
			srell::smatch hex_matches;

			if (srell::regex_match(a_str, rgb_matches, rgb_pattern)) {
				auto red = std::stoi(rgb_matches[1]);
				auto green = std::stoi(rgb_matches[2]);
				auto blue = std::stoi(rgb_matches[3]);
				auto alpha = std::stoi(rgb_matches[4]);

				return { { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f }, false };
			}
			if (srell::regex_match(a_str, hex_matches, hex_pattern)) {
				auto red = std::stoi(hex_matches[1], 0, 16);
				auto green = std::stoi(hex_matches[2], 0, 16);
				auto blue = std::stoi(hex_matches[3], 0, 16);
				auto alpha = std::stoi(hex_matches[4], 0, 16);

				return { { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f }, true };
			}

			return { T(), false };
		} else {
			return { string::to_num<T>(a_str), false };
		}
	}

	template <class T>
	inline std::string FontStyles::ToString(const T& a_style, bool a_hex)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			if (a_hex) {
				return std::format("#{:02X}{:02X}{:02X}{:02X}", static_cast<std::uint8_t>(255.0f * a_style.x), static_cast<std::uint8_t>(255.0f * a_style.y), static_cast<std::uint8_t>(255.0f * a_style.z), static_cast<std::uint8_t>(255.0f * a_style.w));
			}
			return std::format("{},{},{},{}", static_cast<std::uint8_t>(255.0f * a_style.x), static_cast<std::uint8_t>(255.0f * a_style.y), static_cast<std::uint8_t>(255.0f * a_style.z), static_cast<std::uint8_t>(255.0f * a_style.w));
		} else if constexpr (std::is_same_v<float, T>) {
			return std::format("{:.3f}", a_style);
		} else {
			return std::format("{}", a_style);
		}
	}
}
