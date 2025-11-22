#include "Subtitles.h"

#include "Compatibility.h"
#include "ImGui/FontStyles.h"
#include "ImGui/Util.h"

Subtitle::Subtitle(const LocalizedSubtitle& a_subtitle) :
	lines(WrapText(a_subtitle)),
	fullLine(a_subtitle.subtitle),
	validForScaleform(RE::BSScaleformManager::GetSingleton()->IsValidName(a_subtitle.subtitle.c_str()))
{}

std::vector<Subtitle::Line> Subtitle::WrapText(const LocalizedSubtitle& a_subtitle)
{
	std::vector<Line> lines;

	const auto& [text, maxLineWidth, lang] = a_subtitle;

	if (IsTextCJK(text)) {
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
	Line   currentLine;
	size_t currentLineLength = 0;

	const auto flush_line = [&]() {
		float totalSize = 0.0f;
		for (const auto& word : currentLine.words) {
			totalSize += word.size.x;
		}
		currentLine.sizeX = totalSize;

		lines.push_back(currentLine);
		currentLine.words.clear();
		currentLineLength = 0;
	};

	std::size_t i = 0;

	while (i < text.size()) {
		auto charLen = GetUTF8CharLength(text, i);
		auto ch = text.substr(i, charLen);

		if (currentLineLength + ch.size() > maxLineWidth && !currentLine.words.empty()) {
			flush_line();
		}

		ImVec2 charSize = ImGui::CalcTextSize(ch.c_str());
		currentLine.words.emplace_back(ch, charSize, false);
		currentLineLength += ch.size();

		i += charLen;
	}

	if (!currentLine.words.empty()) {
		flush_line();
	}
}

void Subtitle::WrapLatinText(std::vector<Line>& lines, const std::string& text, std::uint32_t maxLineWidth)
{
	static const srell::regex fontRegex(R"(<font\s+face=['"]([^'"]+)['"]>(.*?)</font>)", srell::regex::optimize);

	std::vector<Word> currentWords;
	bool              currentDragonFont = false;
	std::size_t       currentWordsLength = 0;

	Line        currentLine;
	std::size_t currentLineLength = 0;

	const auto flush_words = [&]() {
		if (!currentWords.empty()) {
			std::string mergedText;
			for (std::size_t i = 0; i < currentWords.size(); ++i) {
				if (i > 0) {
					mergedText += " ";
				}
				mergedText += currentWords[i].word;
			}

			ImVec2 mergedSize;
			if (currentDragonFont) {
				ImGui::FontStyles::GetSingleton()->PushDragonFont();
				mergedSize = ImGui::CalcTextSize(mergedText.c_str());
				ImGui::PopFont();
			} else {
				mergedSize = ImGui::CalcTextSize(mergedText.c_str());
			}

			currentLine.words.emplace_back(mergedText, mergedSize, currentDragonFont);
			currentLineLength += mergedText.size();

			currentWords.clear();
			currentWordsLength = 0;
		}
	};
	const auto flush_line = [&]() {
		flush_words();

		float totalSize = 0.0f;
		for (std::size_t i = 0; i < currentLine.words.size(); ++i) {
			if (i > 0) {
				totalSize += ImGui::CalcTextSize(" ").x;
			}
			totalSize += currentLine.words[i].size.x;
		}
		currentLine.sizeX = totalSize;

		lines.push_back(currentLine);
		currentLine.words.clear();
		currentLineLength = 0;
	};
	const auto add_word = [&](const std::string& word, bool isDragonFont) {
		ImVec2 wordSize;
		if (isDragonFont) {
			ImGui::FontStyles::GetSingleton()->PushDragonFont();
			wordSize = ImGui::CalcTextSize(word.c_str());
			ImGui::PopFont();
		} else {
			wordSize = ImGui::CalcTextSize(word.c_str());
		}

		if (!currentWords.empty() && currentDragonFont != isDragonFont) {
			flush_words();
			if (!currentLine.words.empty()) {
				currentLineLength += 1;
			}
		}

		std::size_t spaceLength = currentWords.empty() ? 0 : 1;
		std::size_t newWordsLength = currentWordsLength + spaceLength + word.size();
		std::size_t totalLineLength = currentLineLength;

		if (!currentLine.words.empty() && currentWords.empty()) {
			totalLineLength += 1;
		}
		totalLineLength += newWordsLength;

		if (totalLineLength <= maxLineWidth || (currentLine.words.empty() && currentWords.empty())) {
			currentWords.emplace_back(word, wordSize, isDragonFont);
			currentWordsLength = newWordsLength;
			if (currentWords.size() == 1) {
				currentDragonFont = isDragonFont;
			}
		} else {
			if (!currentWords.empty()) {
				flush_words();
			}

			std::size_t wordLineLength = currentLineLength;
			if (!currentLine.words.empty()) {
				wordLineLength += 1;
			}
			wordLineLength += word.size();

			if (wordLineLength <= maxLineWidth || currentLine.words.empty()) {
				currentWords.emplace_back(word, wordSize, isDragonFont);
				currentWordsLength = word.size();
				currentDragonFont = isDragonFont;
			} else {
				flush_line();
				currentWords.emplace_back(word, wordSize, isDragonFont);
				currentWordsLength = word.size();
				currentDragonFont = isDragonFont;
			}
		}
	};

	for (const auto& token : SplitText(text)) {
		if (token == "<BR_MARKER>") {
			flush_line();
			continue;
		}

		bool        hasFontTag = false;
		bool        wordHasDragonFont = false;
		std::string strippedWord;

		srell::smatch match;
		std::string   remaining = token;

		while (srell::regex_search(remaining, match, fontRegex)) {
			hasFontTag = true;
			const std::string& font = match[1].str();
			const std::string& innerText = match[2].str();

			if (font == "$DragonFont") {
				wordHasDragonFont = true;
			}

			if (auto prefix = match.prefix().str(); !prefix.empty()) {
				strippedWord += prefix;
			}

			strippedWord += innerText;
			remaining = match.suffix().str();
		}
		strippedWord += remaining;

		if (hasFontTag && !strippedWord.empty()) {
			static const srell::regex trailingPunct(R"(^(.+?)([!?,.:;]+)$)");
			srell::smatch             punctMatch;

			if (srell::regex_match(strippedWord, punctMatch, trailingPunct)) {
				std::string mainWord = punctMatch[1].str();
				std::string punct = punctMatch[2].str();


				add_word(mainWord, wordHasDragonFont);
				add_word(punct, false);
			} else {
				add_word(strippedWord, wordHasDragonFont);
			}
		} else {
			add_word(strippedWord, wordHasDragonFont);
		}
	}

	if (!currentWords.empty() || !currentLine.words.empty()) {
		flush_line();
	}
}

std::vector<std::string> Subtitle::SplitText(const std::string& a_text)
{
	static const srell::regex br_tag(R"(<br\s*/?>)", srell::regex::optimize);
	static const srell::regex re(R"([^\s<]*<[^>]+/>(?:[^\s<>])*|[^\s<]*<[^>]+>(?:[^<]|<(?!/))*?</[^>]+>(?:[^\s<>])*|[^\s]+)", srell::regex::optimize);

	std::vector<std::string> result;
	std::string              remaining = a_text;
	srell::smatch            match;

	while (srell::regex_search(remaining, match, br_tag)) {
		std::string prefix = match.prefix().str();
		if (!prefix.empty()) {
			for (auto it = srell::sregex_iterator(prefix.begin(), prefix.end(), re);
				it != srell::sregex_iterator(); ++it) {
				result.push_back(it->str());
			}
		}

		result.push_back("<BR_MARKER>");

		remaining = match.suffix().str();
	}

	if (!remaining.empty()) {
		for (auto it = srell::sregex_iterator(remaining.begin(), remaining.end(), re);
			it != srell::sregex_iterator(); ++it) {
			result.push_back(it->str());
		}
	}

	return result;
}

std::uint8_t Subtitle::GetUTF8CharLength(const std::string& str, std::size_t pos)
{
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
}

bool Subtitle::IsTextCJK(const std::string& str)
{
	constexpr auto IsCJKCodePoint = [](char32_t cp) {
		return (cp >= 0x4E00 && cp <= 0x9FFF) ||
		       (cp >= 0x3400 && cp <= 0x4DBF) ||
		       (cp >= 0x20000 && cp <= 0x2EBEF) ||
		       (cp >= 0xF900 && cp <= 0xFAFF) ||
		       (cp >= 0x2F800 && cp <= 0x2FA1F) ||
		       (cp >= 0x3040 && cp <= 0x309F) ||
		       (cp >= 0x30A0 && cp <= 0x30FF) ||
		       (cp >= 0xAC00 && cp <= 0xD7AF);
	};

	std::size_t i = 0;
	while (i < str.size()) {
		auto charLen = GetUTF8CharLength(str, i);
		if (i + charLen > str.size()) {
			break;
		}

		char32_t cp = 0;
		switch (charLen) {
		case 1:
			cp = static_cast<unsigned char>(str[i]);
			break;
		case 2:
			cp = ((static_cast<unsigned char>(str[i]) & 0x1F) << 6) |
			     (static_cast<unsigned char>(str[i + 1]) & 0x3F);
			break;
		case 3:
			cp = ((static_cast<unsigned char>(str[i]) & 0x0F) << 12) |
			     ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 6) |
			     (static_cast<unsigned char>(str[i + 2]) & 0x3F);
			break;
		case 4:
			cp = ((static_cast<unsigned char>(str[i]) & 0x07) << 18) |
			     ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 12) |
			     ((static_cast<unsigned char>(str[i + 2]) & 0x3F) << 6) |
			     (static_cast<unsigned char>(str[i + 3]) & 0x3F);
			break;
		default:
			break;
		}

		if (IsCJKCodePoint(cp)) {
			return true;
		}

		i += charLen;
	}

	return false;
}

void Subtitle::DrawSubtitle(float a_posX, float& a_posY, float a_alpha, float a_lineHeight) const
{
	if (a_alpha < 0.01f) {
		return;
	}

	auto* drawList = ImGui::GetForegroundDrawList();

	const auto& style = ImGui::GetStyle();
	auto        textColor = ImGui::GetColorU32(style.Colors[ImGuiCol_Text], a_alpha);
	auto        textShadow = ImGui::GetColorU32(style.Colors[ImGuiCol_TextShadow], a_alpha);
	auto        shadowOffset = style.TextShadowOffset;

	for (const auto& line : lines) {
		a_posY -= a_lineHeight;

		float currentX = a_posX - (line.sizeX * 0.5f);

		for (const auto& word : line.words) {
			if (word.isDragonFont) {
				ImGui::FontStyles::GetSingleton()->PushDragonFont();
			}

			const ImVec2 textPos(currentX, a_posY);
			drawList->AddText(textPos + shadowOffset, textShadow, word.word.c_str());
			drawList->AddText(textPos, textColor, word.word.c_str());

			if (word.isDragonFont) {
				ImGui::PopFont();
			}

			currentX += word.size.x;
		}
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
	auto [posX, posY] = a_screenParams.pos;

	if (!secondary.lines.empty()) {
		posY -= lineHeight * a_screenParams.spacing;
		secondary.DrawSubtitle(posX, posY, a_screenParams.alphaSecondary, lineHeight);
	}

	primary.DrawSubtitle(posX, posY, a_screenParams.alphaPrimary, lineHeight);

	if (!a_screenParams.speakerName.empty() && a_screenParams.alphaPrimary >= 0.01f) {
		posY -= lineHeight;

		auto& style = ImGui::GetStyle();
		auto  textColor = ImGui::GetColorU32(a_screenParams.speakerColor, a_screenParams.alphaPrimary);
		auto  textShadow = ImGui::GetColorU32(style.Colors[ImGuiCol_TextShadow], a_screenParams.alphaPrimary);
		auto  shadowOffset = style.TextShadowOffset;

		const ImVec2      textPos(posX - (ImGui::CalcTextSize(a_screenParams.speakerName.c_str()).x * 0.5f), posY);
		const std::string line = std::format("{}:", a_screenParams.speakerName);

		auto* drawList = ImGui::GetForegroundDrawList();
		drawList->AddText(textPos + shadowOffset, textShadow, line.c_str());
		drawList->AddText(textPos, textColor, line.c_str());
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
