#include "Subtitles.h"

#include "Compatibility.h"
#include "ImGui/FontStyles.h"
#include "ImGui/Util.h"
#include "RayCaster.h"
#include "SettingLoader.h"

namespace Subtitles
{
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

	void Manager::OnDataLoaded()
	{
		localizedSubs.BuildLocalizedSubtitles();
		LoadMCMSettings();

		const auto gameMaxDistance = RE::GetINISettingFloat("fMaxSubtitleDistance:Interface");
		maxDistanceStartSq = gameMaxDistance * gameMaxDistance;
		maxDistanceEndSq = (gameMaxDistance * 1.05f) * (gameMaxDistance * 1.05f);

		logger::info("Max subtitle distance: {:.2f} (start), {:.2f} (end)", gameMaxDistance, gameMaxDistance * 1.05f);
	}

	void Manager::LoadMCMSettings()
	{
		SettingLoader::GetSingleton()->Load(FileType::kMCM, [this](auto& ini) {
			LoadMCMSettings(ini);
		});

		localizedSubs.PostMCMSettingsLoad();
	}

	void Manager::MCMSettings::LoadMCMSettings(CSimpleIniA& a_ini)
	{
		showGeneralSubtitles = a_ini.GetBoolValue("Settings", "bGeneralSubtitles", showGeneralSubtitles);
		showDialogueSubtitles = a_ini.GetBoolValue("Settings", "bDialogueSubtitles", showDialogueSubtitles);

		subtitleHeadOffset = static_cast<float>(a_ini.GetDoubleValue("Settings", "fHeadOffset", 20.0)) * ModAPIHandler::GetSingleton()->GetResolutionScale();

		doRayCastChecks = a_ini.GetBoolValue("Settings", "bEnableRaycastChecks", doRayCastChecks);

		fadeSubtitles = a_ini.GetBoolValue("Settings", "bFadeOutOfSightSubtitles", fadeSubtitles);
		fadeSubtitleAlpha = static_cast<float>(a_ini.GetDoubleValue("Settings", "fFadedSubtitleOpacity", fadeSubtitleAlpha));

		showDualSubs = a_ini.GetBoolValue("Settings", "bDualSubtitles", showDualSubs);
		subtitleSpacing = static_cast<float>(a_ini.GetDoubleValue("Settings", "fDualSubtitleSpacing", subtitleSpacing));

		useBTPSWidgetPosition = a_ini.GetBoolValue("Settings", "bUseBTPSWidgetPosition", useBTPSWidgetPosition);
	}

	void Manager::LoadMCMSettings(CSimpleIniA& a_ini)
	{
		previous = current;

		current.LoadMCMSettings(a_ini);
		bool rebuildSubs = localizedSubs.LoadMCMSettings(a_ini);

		// force hide vanilla subtitle
		if (!previous.showGeneralSubtitles && current.showGeneralSubtitles || !previous.showDialogueSubtitles && current.showDialogueSubtitles) {
			if (auto hudData = static_cast<RE::HUDData*>(RE::UIMessageQueue::GetSingleton()->CreateUIMessageData(RE::InterfaceStrings::GetSingleton()->hudData))) {
				hudData->type = RE::HUD_MESSAGE_TYPE::kHideSubtitle;
				RE::UIMessageQueue::GetSingleton()->AddMessage(RE::InterfaceStrings::GetSingleton()->hudMenu, RE::UI_MESSAGE_TYPE::kUpdate, hudData);
			}
		}

		if (previous.showDualSubs != current.showDualSubs || rebuildSubs) {
			RebuildProcessedSubtitles();
		}
	}

	bool Manager::IsVisible() const
	{
		return visible;
	}

	void Manager::SetVisible(bool a_visible)
	{
		visible = a_visible;
	}

	bool Manager::ShowGeneralSubtitles() const { return RE::ShowGeneralSubsGame() && current.showGeneralSubtitles; }

	bool Manager::ShowDialogueSubtitles() const { return RE::ShowDialogueSubsGame() && current.showDialogueSubtitles; }

	RE::NiPoint3 Manager::GetSubtitleAnchorPosImpl(const RE::TESObjectREFRPtr& a_ref, float a_height)
	{
		RE::NiPoint3 pos = a_ref->GetPosition();
		if (const auto headNode = RE::GetHeadNode(a_ref)) {
			pos = headNode->world.translate;
		} else {
			pos.z += a_height;
		}
		return pos;
	}

	RE::NiPoint3 Manager::CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const
	{
		const auto ref = a_subInfo.speaker.get();
		const auto height = ref->GetHeight();

		auto pos = GetSubtitleAnchorPosImpl(ref, height);
		auto offset = current.subtitleHeadOffset;

		if (auto overridePosZ = current.useBTPSWidgetPosition ? ModAPIHandler::GetSingleton()->GetWidgetPosZ(ref) : std::optional<float>()) {
			pos.z = *overridePosZ;
			offset = current.subtitleHeadOffset * 0.75f;
		}

		pos.z += offset * (height / 128.0f);

		return pos;
	}

	DualSubtitle Manager::CreateDualSubtitles(const char* subtitle) const
	{
		auto primarySub = localizedSubs.GetPrimarySubtitle(subtitle);
		if (current.showDualSubs) {
			auto secondarySub = localizedSubs.GetSecondarySubtitle(subtitle);
			if (!primarySub.empty() && !secondarySub.empty() && primarySub != secondarySub) {
				return DualSubtitle(primarySub, secondarySub);
			}
		}
		return DualSubtitle(primarySub);
	}

	void Manager::AddSubtitle(RE::SubtitleManager* a_manager, const char* subtitle)
	{
		AddProcessedSubtitle(subtitle);

		RE::BSSpinLockGuard gameLocker(a_manager->lock);
		{
			auto& subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(a_manager->subtitles);
			if (!subtitleArray.empty()) {
				auto& subInfo = subtitleArray.back();
				subInfo.flagsRaw() = 0;  // reset any junk values
				subInfo.setFlag(SubtitleFlag::kObscured, false);
			}
		}
	}

	void Manager::AddProcessedSubtitle(const char* subtitle)
	{
		WriteLocker locker(subtitleLock);
		processedSubtitles.try_emplace(subtitle, CreateDualSubtitles(subtitle));
	}

	const DualSubtitle& Manager::GetProcessedSubtitles(const RE::BSString& subtitle)
	{
		ReadLocker locker(subtitleLock);
		return processedSubtitles.find(subtitle.c_str())->second;
	}

	void Manager::RebuildProcessedSubtitles()
	{
		WriteLocker locker(subtitleLock);
		for (auto& [text, subs] : processedSubtitles) {
			subs = CreateDualSubtitles(text.c_str());
		}
	}

	void Manager::UpdateSubtitles(RE::SubtitleManager* a_manager) const
	{
		const auto menuTopicMgr = RE::MenuTopicManager::GetSingleton();

		const bool showGeneral = ShowGeneralSubtitles();
		const bool showDialogue = ShowDialogueSubtitles();

		RE::BSSpinLockGuard locker(a_manager->lock);

		auto& subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(a_manager->subtitles);

		for (auto& subtitleInfo : subtitleArray) {
			subtitleInfo.setFlag(SubtitleFlag::kObscured, false);

			if (auto ref = subtitleInfo.speaker.get()) {
				bool isDialogueSpeaker = menuTopicMgr->IsCurrentSpeaker(subtitleInfo.speaker);
				subtitleInfo.setFlag(SubtitleFlag::kTypeDialogue, isDialogueSpeaker);

				if (auto actor = ref->As<RE::Actor>()) {
					if (!current.doRayCastChecks ||
						(isDialogueSpeaker && !showDialogue) ||
						(!isDialogueSpeaker && !showGeneral) ||
						(!subtitleInfo.forceDisplay && subtitleInfo.targetDistance > maxDistanceEndSq)) {
						continue;
					}

					RayCaster rayCaster(actor);
					subtitleInfo.setFlag(RE::SubtitleInfoEx::Flag::kObscured, !rayCaster.CanRayCastToTarget(false));
				}
			}
		}
	}

	void Manager::Draw()
	{
		const bool showGeneral = ShowGeneralSubtitles();
		const bool showDialogue = ShowDialogueSubtitles();

		if (!showGeneral && !showDialogue || !IsVisible()) {
			return;
		}

		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
		{
			auto  subtitleManager = RE::SubtitleManager::GetSingleton();
			auto& subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(subtitleManager->subtitles);

			RE::BSSpinLockGuard gameLocker(subtitleManager->lock);

			for (auto& subtitleInfo : subtitleArray | std::views::reverse) {  // reverse order so closer subtitles get rendered on top
				const bool isDialogueSubtitle = subtitleInfo.isFlagSet(SubtitleFlag::kTypeDialogue);

				if ((isDialogueSubtitle && !showDialogue) ||
					(!isDialogueSubtitle && !showGeneral)) {
					continue;
				}

				if (auto ref = subtitleInfo.speaker.get()) {
					auto actor = ref->As<RE::Actor>();

					if (subtitleInfo.forceDisplay || subtitleInfo.targetDistance <= maxDistanceEndSq) {
						ScreenParams params;
						RE::NiPoint3 anchorPos = CalculateSubtitleAnchorPos(subtitleInfo);
						auto         zVal = ImGui::WorldToScreenLoc(anchorPos, params.pos);

						if (zVal < 0.0f) {
							continue;
						}

						if (subtitleInfo.isFlagSet(SubtitleFlag::kObscured)) {
							if (current.fadeSubtitles) {
								params.alpha *= current.fadeSubtitleAlpha;
							} else {
								continue;
							}
						}

						if (subtitleInfo.targetDistance > maxDistanceStartSq) {
							const float t = (subtitleInfo.targetDistance - maxDistanceStartSq) / (maxDistanceEndSq - maxDistanceStartSq);
							params.alpha *= 1.0f - glm::cubicEaseOut(t);
						} else if (actor) {
							if (auto high = actor->GetHighProcess(); high && high->fadeAlpha < 1.0f) {
								params.alpha *= high->fadeAlpha;
							}
							if (actor->IsDead() && actor->voiceTimer < 1.0f) {
								params.alpha *= actor->voiceTimer;
							}
						}

						params.spacing = current.subtitleSpacing;

						GetProcessedSubtitles(subtitleInfo.subtitle).DrawDualSubtitle(params);
					}
				}
			}
		}
		ImGui::End();
	}
}
