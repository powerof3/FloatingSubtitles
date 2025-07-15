#include "Subtitles.h"

#include "Compatibility.h"
#include "ImGui/FontStyles.h"

Subtitle::Subtitle(const LocalizedSubtitle& a_subtitle) :
	lines(WrapText(a_subtitle)),
	fullLine(a_subtitle.subtitle),
	validForScaleform(RE::BSScaleformManager::GetSingleton()->IsValidName(a_subtitle.subtitle.c_str()))
{}

std::vector<Subtitle::Line> Subtitle::WrapText(const LocalizedSubtitle& a_subtitle)
{
	std::vector<Line> lines;

	const auto& [text, maxLineWidth, lang] = a_subtitle;

	if (lang == Language::kChinese || lang == Language::kJapanese || lang == Language::kKorean) {
		WrapCJKText(lines, text, maxLineWidth);
	} else {
		WrapLatinText(lines, text, maxLineWidth);
	}

	// for drawing lines from bottom to top
	std::ranges::reverse(lines);

	return lines;
}

void Subtitle::WrapCJKText(std::vector<Line>& lines, const std::string& text, std::uint32_t maxLineWidth)
{
	constexpr auto GetUTF8CharLength = [](const std::string& str, std::size_t pos) {
		const auto ch = static_cast<unsigned char>(str[pos]);
		if ((ch & 0x80) == 0) {  // ASCII
			return 1;
		}
		if ((ch & 0xE0) == 0xC0) {  // 2-byte UTF8
			return 2;
		}
		if ((ch & 0xF0) == 0xE0) {  // 3-byte UTF8
			return 3;
		}
		if ((ch & 0xF8) == 0xF0) {  // 4-byte UTF8
			return 4;
		}
		return 1;
	};

	std::string currentLine;
	std::size_t i = 0;

	while (i < text.size()) {
		auto charLen = GetUTF8CharLength(text, i);
		auto ch = text.substr(i, charLen);

		if (currentLine.size() + ch.size() > maxLineWidth && !currentLine.empty()) {
			lines.emplace_back(currentLine, ImGui::CalcTextSize(currentLine.c_str()));
			currentLine = ch;
		} else {
			currentLine += ch;
		}

		i += charLen;
	}

	if (!currentLine.empty()) {
		lines.emplace_back(currentLine, ImGui::CalcTextSize(currentLine.c_str()));
	}
}

void Subtitle::WrapLatinText(std::vector<Line>& lines, const std::string& text, std::uint32_t maxLineWidth)
{
	std::istringstream wordStream(text);
	std::string        word;
	std::string        currentLine;

	while (wordStream >> word) {
		std::string line = currentLine.empty() ? word : currentLine + ' ' + word;
		if (line.size() <= maxLineWidth) {
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
}

void Subtitle::DrawSubtitle(float a_posX, float& a_posY, float a_alpha, float a_lineHeight) const
{
	if (a_alpha < 0.01f) {
		return;
	}

	auto* drawList = ImGui::GetForegroundDrawList();
	const auto& [textColor, shadowColor, shadowOffset] = ImGui::FontStyles::GetSingleton()->GetStyleParams(a_alpha);

	for (const auto& [line, textSize] : lines) {
		a_posY -= a_lineHeight;

		const ImVec2 textPos(a_posX - (textSize.x * 0.5f), a_posY);
		drawList->AddText(textPos + shadowOffset, shadowColor, line.c_str());
		drawList->AddText(textPos, textColor, line.c_str());
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

std::string DualSubtitle::GetScaleformCompatibleSubtitle(bool a_dualSubs) const
{
	std::string subtitle;
	if (primary.validForScaleform) {
		subtitle = primary.fullLine;
	}
	if (a_dualSubs && secondary.validForScaleform) {
		if (!subtitle.empty()) {
			subtitle.append("\n");
		}
		subtitle.append(secondary.fullLine);
	}
	return subtitle;
}
