#pragma once

#include "Localization.h"

namespace ImGui
{
	struct StyleParams;
}

struct Subtitle
{
	struct Line
	{
		std::string line;
		ImVec2      lineSize;
	};

	Subtitle() = default;
	Subtitle(const LocalizedSubtitle& a_subtitle);

	void DrawSubtitle(float a_posX, float& a_posY, float a_alpha, float a_lineHeight) const;

	std::vector<Line> lines;
	std::string       fullSubtitle;

private:
	static std::vector<Line> WrapText(const std::string& text, std::uint32_t maxWidth);
};

struct DualSubtitle
{
	struct ScreenParams
	{
		ImVec2 pos{};
		float  alphaPrimary{ 1.0f };
		float  alphaSecondary{ 1.0f };
		float  spacing{ 0.5f };
	};

	DualSubtitle() = default;
	DualSubtitle(const LocalizedSubtitle& a_primarySubtitle);
	DualSubtitle(const LocalizedSubtitle& a_primarySubtitle, const LocalizedSubtitle& a_secondarySubtitle);

	void DrawDualSubtitle(const ScreenParams& a_screenParams) const;

	// members
	Subtitle primary{};
	Subtitle secondary{};
};
