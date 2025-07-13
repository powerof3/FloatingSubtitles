#include "Subtitles.h"

#include "Compatibility.h"
#include "ImGui/FontStyles.h"

Subtitle::Subtitle(const LocalizedSubtitle& a_subtitle) :
	lines(WrapText(a_subtitle.subtitle, a_subtitle.maxCharsPerLine))
{}

std::vector<Subtitle::Line> Subtitle::WrapText(const std::string& text, std::uint32_t maxWidth)
{
	std::vector<Line>  lines;
	std::istringstream wordStream(text);
	std::string        word;
	std::string        currentLine;

	while (wordStream >> word) {
		if (currentLine.size() + 1 + word.size() <= maxWidth) {
			currentLine += ' ' + word;
		} else {
			lines.emplace_back(currentLine, ImGui::CalcTextSize(currentLine.c_str()));
			currentLine = word;
		}
		string::trim(currentLine);
	}

	if (!currentLine.empty()) {
		lines.emplace_back(currentLine, ImGui::CalcTextSize(currentLine.c_str()));
	}

	std::ranges::reverse(lines);  // for drawing lines from bottom to top

	return lines;
}

void Subtitle::DrawSubtitle(float a_posX, float& a_posY, const ImGui::StyleParams& a_params, float a_lineHeight) const
{
	auto* drawList = ImGui::GetForegroundDrawList();

	for (const auto& [line, textSize] : lines) {
		a_posY -= a_lineHeight;

		const ImVec2 textPos(a_posX - (textSize.x * 0.5f), a_posY);
		drawList->AddText(textPos + a_params.shadowOffset, a_params.shadowColor, line.c_str());
		drawList->AddText(textPos, a_params.textColor, line.c_str());
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
	const auto& [screenPos, alpha, spacing] = a_screenParams;
	if (alpha < 0.01f) {
		return;
	}

	const auto& styleParams = ImGui::FontStyles::GetSingleton()->GetStyleParams(alpha);
	const auto  lineHeight = ImGui::GetTextLineHeight();
	auto        posY = screenPos.y;

	primary.DrawSubtitle(screenPos.x, posY, styleParams, lineHeight);

	if (!secondary.lines.empty()) {
		posY -= lineHeight * spacing;
		secondary.DrawSubtitle(screenPos.x, posY, styleParams, lineHeight);
	}
}
