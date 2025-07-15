#include "Subtitles.h"

#include "Compatibility.h"
#include "ImGui/FontStyles.h"

Subtitle::Subtitle(const LocalizedSubtitle& a_subtitle) :
	lines(WrapText(a_subtitle.subtitle, a_subtitle.maxCharsPerLine)),
	fullSubtitle(a_subtitle.subtitle)
{}

std::vector<Subtitle::Line> Subtitle::WrapText(const std::string& text, std::uint32_t maxWidth)
{
	std::vector<Line>  lines;
	std::istringstream wordStream(text);
	std::string        word;
	std::string        currentLine;

	while (wordStream >> word) {
		std::string line = currentLine.empty() ? word : currentLine + ' ' + word;
		if (line.size() <= maxWidth) {
			currentLine = line;
		} else {
			if (!currentLine.empty()) {
				lines.emplace_back(currentLine, ImGui::CalcTextSize(currentLine.c_str()));
			}
			currentLine = word;
		}
	}

	if (!currentLine.empty()) {
		lines.emplace_back(currentLine, ImGui::CalcTextSize(currentLine.c_str()));
	}

	std::ranges::reverse(lines);  // for drawing lines from bottom to top
	return lines;
}

void Subtitle::DrawSubtitle(float a_posX, float& a_posY, float a_alpha, float a_lineHeight) const
{
	if (a_alpha < 0.01f) {
		return;
	}

	auto*       drawList = ImGui::GetForegroundDrawList();
	const auto& styleParams = ImGui::FontStyles::GetSingleton()->GetStyleParams(a_alpha);

	for (const auto& [line, textSize] : lines) {
		a_posY -= a_lineHeight;

		const ImVec2 textPos(a_posX - (textSize.x * 0.5f), a_posY);
		drawList->AddText(textPos + styleParams.shadowOffset, styleParams.shadowColor, line.c_str());
		drawList->AddText(textPos, styleParams.textColor, line.c_str());
	}
}

DualSubtitle::DualSubtitle(const LocalizedSubtitle& a_primarySubtitle) :
	primary(a_primarySubtitle)
{}

DualSubtitle::DualSubtitle(const LocalizedSubtitle& a_primarySubtitle, const LocalizedSubtitle& a_secondarySubtitle) :
	primary(a_primarySubtitle),
	secondary(a_secondarySubtitle)
{}

void DualSubtitle::DrawDualSubtitle(const ScreenParams& a_screenParams) const
{
	const auto lineHeight = ImGui::GetTextLineHeight();
	auto       posY = a_screenParams.pos.y;

	primary.DrawSubtitle(a_screenParams.pos.x, posY, a_screenParams.alphaPrimary, lineHeight);

	if (!secondary.lines.empty()) {
		posY -= lineHeight * a_screenParams.spacing;
		secondary.DrawSubtitle(a_screenParams.pos.x, posY, a_screenParams.alphaSecondary, lineHeight);
	}
}
