#pragma once

#include "Localization.h"

namespace ImGui
{
	struct StyleParams;
}

struct Subtitle
{
	struct Word
	{
		std::string word;
		ImVec2      size;
		bool        isDragonFont;
	};

	struct Line
	{
		std::vector<Word> words;
		float             sizeX;
	};

	Subtitle() = default;
	Subtitle(const LocalizedSubtitle& a_subtitle);

	void DrawSubtitle(float a_posX, float& a_posY, float a_alpha, float a_lineHeight) const;

	std::vector<Line> lines;
	std::string       fullLine;
	bool              validForScaleform{ false };

private:
	static std::vector<Line>        WrapText(const LocalizedSubtitle& a_subtitle);
	static void                     WrapCJKText(std::vector<Line>& lines, const std::string& text, std::uint32_t maxLineWidth);
	static void                     WrapLatinText(std::vector<Line>& lines, const std::string& text, std::uint32_t maxLineWidth);
	static std::uint8_t             GetUTF8CharLength(const std::string& str, std::size_t pos);
	static bool                     IsTextCJK(const std::string& str);
	static std::vector<std::string> SplitText(const std::string& a_text);
};

struct DualSubtitle
{
	struct ScreenParams
	{
		ImVec2      pos{};
		float       alphaPrimary{ 1.0f };
		float       alphaSecondary{ 1.0f };
		float       spacing{ 0.5f };
		std::string speakerName{};
		ImVec4      speakerColor;
	};

	DualSubtitle() = default;
	DualSubtitle(const LocalizedSubtitle& a_primarySubtitle);
	DualSubtitle(const LocalizedSubtitle& a_primarySubtitle, const LocalizedSubtitle& a_secondarySubtitle);

	void        DrawDualSubtitle(const ScreenParams& a_screenParams) const;
	std::string GetScaleformCompatibleSubtitle(bool a_dualSubs) const;

	// members
	Subtitle primary{};
	Subtitle secondary{};
};
