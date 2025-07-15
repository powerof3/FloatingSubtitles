#include "Manager.h"

#include "Compatibility.h"
#include "ImGui/FontStyles.h"
#include "ImGui/Util.h"
#include "RayCaster.h"
#include "SettingLoader.h"

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
	useTrueHUDWidgetPosition = a_ini.GetBoolValue("Settings", "bUseTrueHUDWidgetPosition", useTrueHUDWidgetPosition);
}

void Manager::LoadMCMSettings(CSimpleIniA& a_ini)
{
	previous = current;

	current.LoadMCMSettings(a_ini);
	bool rebuildSubs = localizedSubs.LoadMCMSettings(a_ini);

	// force hide vanilla subtitle
	if (!previous.showGeneralSubtitles && current.showGeneralSubtitles || !previous.showDialogueSubtitles && current.showDialogueSubtitles) {
		RE::SendHUDMenuMessage(RE::HUD_MESSAGE_TYPE::kHideSubtitle);
	}

	if (previous.showDualSubs != current.showDualSubs || rebuildSubs) {
		RebuildProcessedSubtitles();
	}
}

void Manager::LoadMCMSettings()
{
	SettingLoader::GetSingleton()->Load(FileType::kMCM, [this](auto& ini) {
		LoadMCMSettings(ini);
	});

	localizedSubs.PostMCMSettingsLoad();
}

void Manager::OnDataLoaded()
{
	localizedSubs.BuildLocalizedSubtitles();
	LoadMCMSettings();

	const auto gameMaxDistance = RE::GetINISetting<float>("fMaxSubtitleDistance:Interface");
	maxDistanceStartSq = gameMaxDistance * gameMaxDistance;
	maxDistanceEndSq = (gameMaxDistance * 1.05f) * (gameMaxDistance * 1.05f);

	logger::info("Max subtitle distance: {:.2f} (start), {:.2f} (end)", gameMaxDistance, gameMaxDistance * 1.05f);

	speakerColor = RE::GetINISetting<std::int32_t>("iSubtitleSpeakerNameColor:Interface");
}

bool Manager::IsVisible() const
{
	return visible;
}

void Manager::SetVisible(bool a_visible)
{
	visible = a_visible;
}

bool Manager::ShowGeneralSubtitles() const
{
	return RE::ShowGeneralSubsGame() && current.showGeneralSubtitles;
}

bool Manager::ShowDialogueSubtitles() const
{
	return RE::ShowDialogueSubsGame() && current.showDialogueSubtitles;
}

bool Manager::HandlesGeneralSubtitles(RE::BSString&) const
{
	return ShowGeneralSubtitles();
}

bool Manager::HandlesDialogueSubtitles(RE::BSString*) const
{
	return ShowDialogueSubtitles();
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

void Manager::AddProcessedSubtitle(const char* subtitle)
{
	WriteLocker locker(subtitleLock);
	processedSubtitles.try_emplace(subtitle, CreateDualSubtitles(subtitle));
}

const DualSubtitle& Manager::GetProcessedSubtitle(const RE::BSString& a_subtitle)
{
	{
		ReadLocker readLock(subtitleLock);
		if (auto it = processedSubtitles.find(a_subtitle.c_str()); it != processedSubtitles.end()) {
			return it->second;
		}
	}

	{
		WriteLocker writeLock(subtitleLock);
		auto [it, inserted] = processedSubtitles.try_emplace(a_subtitle.c_str(), CreateDualSubtitles(a_subtitle.c_str()));
		return it->second;
	}
}

void Manager::AddSubtitle(RE::SubtitleManager* a_manager, const char* a_subtitle)
{
	if (!string::is_empty(a_subtitle) && !string::is_only_space(a_subtitle)) {
		AddProcessedSubtitle(a_subtitle);

		RE::BSSpinLockGuard gameLocker(a_manager->lock);
		{
			auto& subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(a_manager->subtitles);
			if (!subtitleArray.empty()) {
				auto& subInfo = subtitleArray.back();
				subInfo.flagsRaw() = 0;  // reset any junk values
			}
		}
	}
}

void Manager::RebuildProcessedSubtitles()
{
	WriteLocker locker(subtitleLock);
	for (auto& [text, subs] : processedSubtitles) {
		subs = CreateDualSubtitles(text.c_str());
	}
}

void Manager::UpdateSubtitleInfo(RE::SubtitleManager* a_manager)
{
	const auto menuTopicMgr = RE::MenuTopicManager::GetSingleton();

	const bool showGeneral = ShowGeneralSubtitles();
	const bool showDialogue = ShowDialogueSubtitles();

	RE::BSSpinLockGuard locker(a_manager->lock);
	{
		auto& subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(a_manager->subtitles);

		lastOffscreenSub = offscreenSub;
		offscreenSub.clear();

		for (auto& subtitleInfo : subtitleArray) {
			subtitleInfo.flagsRaw() = 0;

			if (const auto& ref = subtitleInfo.speaker.get()) {
				bool isDialogueSpeaker = menuTopicMgr->IsCurrentSpeaker(subtitleInfo.speaker);

				if ((isDialogueSpeaker && !showDialogue) ||
					(!isDialogueSpeaker && !showGeneral) ||
					!subtitleInfo.forceDisplay && subtitleInfo.targetDistance > maxDistanceEndSq) {
					subtitleInfo.setFlag(SubtitleFlag::kSkip, true);
					continue;  // vanilla subtitle skip
				}

				if (!ref->IsActor()) {
					subtitleInfo.setFlag(SubtitleFlag::kSkip, true);
					BuildOffscreenSubtitle(offscreenSub, ref, subtitleInfo.subtitle);
					continue;
				}

				CalculateAlpha(subtitleInfo);

				const auto actor = ref->As<RE::Actor>();

				switch (RayCaster(actor).GetResult(false)) {
				case RayCaster::Result::kOffScreen:
					{
						subtitleInfo.setFlag(RE::SubtitleInfoEx::Flag::kOffScreen, true);
						BuildOffscreenSubtitle(offscreenSub, ref, subtitleInfo.subtitle);
					}
					break;
				case RayCaster::Result::kObscured:
					subtitleInfo.setFlag(RE::SubtitleInfoEx::Flag::kObscured, true);
					break;
				default:
					break;
				}
			}
		}

		if (lastOffscreenSub != offscreenSub || !offscreenSub.empty()) {
			SKSE::GetTaskInterface()->AddUITask([this]() {
				if (auto hudMenu = RE::UI::GetSingleton()->GetMenu<RE::HUDMenu>()) {
					if (lastOffscreenSub != offscreenSub) {
						RE::GFxValue subtitleText(lastOffscreenSub);
						hudMenu->root.Invoke("HideSubtitle", nullptr, &subtitleText, 1);
					} else if (!offscreenSub.empty()) {
						RE::GFxValue subtitleText(offscreenSub);
						hudMenu->root.Invoke("ShowSubtitle", nullptr, &subtitleText, 1);
					}
				}
			});
		}
	}
}

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

void Manager::CalculateAlpha(RE::SubtitleInfoEx& a_subInfo) const
{
	const auto ref = a_subInfo.speaker.get();
	const auto actor = ref->As<RE::Actor>();

	float alpha = 1.0f;

	if (a_subInfo.isFlagSet(SubtitleFlag::kObscured)) {
		if (current.fadeSubtitles) {
			alpha *= current.fadeSubtitleAlpha;
		}
	}

	if (a_subInfo.targetDistance > maxDistanceStartSq) {
		const float t = (a_subInfo.targetDistance - maxDistanceStartSq) / (maxDistanceEndSq - maxDistanceStartSq);
		alpha *= 1.0f - glm::cubicEaseOut(t);
	} else if (auto high = actor->GetHighProcess(); high && high->fadeAlpha < 1.0f) {
		alpha *= high->fadeAlpha;
	} else if (actor->IsDead() && actor->voiceTimer < 1.0f) {
		alpha *= actor->voiceTimer;
	}

	a_subInfo.textAlpha() = std::bit_cast<std::uint32_t>(alpha);
}

void Manager::BuildOffscreenSubtitle(std::string& a_subtitle, const RE::TESObjectREFRPtr& a_speaker, const RE::BSString& a_subtitleRaw)
{
	if (offscreenSubCount > current.maxOffscreenSubs) {
		return;
	}
	a_subtitle.append(std::format("<font color='#{:6X}'>{}</font>: {}\n", speakerColor, a_speaker->GetDisplayFullName(), a_subtitleRaw.c_str()));
	offscreenSubCount++;
}

RE::NiPoint3 Manager::CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const
{
	const auto ref = a_subInfo.speaker.get();
	const auto height = ref->GetHeight();

	auto pos = GetSubtitleAnchorPosImpl(ref, height);
	auto offset = current.subtitleHeadOffset;

	if (auto overridePosZ = ModAPIHandler::GetSingleton()->GetWidgetPosZ(ref, current.useBTPSWidgetPosition, current.useTrueHUDWidgetPosition)) {
		pos.z = *overridePosZ;
		offset = current.subtitleHeadOffset * 0.75f;
	}

	pos.z += offset * (height / 128.0f);

	return pos;
}

void Manager::Draw()
{
	const bool showGeneral = ShowGeneralSubtitles();
	const bool showDialogue = ShowDialogueSubtitles();

	if (!showGeneral && !showDialogue || !IsVisible()) {
		return;
	}

	auto  subtitleManager = RE::SubtitleManager::GetSingleton();
	auto& subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(subtitleManager->subtitles);

	ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
	ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

	ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
	{
		RE::BSSpinLockGuard gameLocker(subtitleManager->lock);
		{
			DualSubtitle::ScreenParams params;
			params.spacing = current.subtitleSpacing;

			for (auto& subtitleInfo : subtitleArray | std::views::reverse) {  // reverse order so closer subtitles get rendered on top
				if (subtitleInfo.isFlagSet(SubtitleFlag::kSkip) || subtitleInfo.isFlagSet(SubtitleFlag::kOffScreen)) {
					continue;
				}

				if (subtitleInfo.isFlagSet(SubtitleFlag::kObscured) && !current.fadeSubtitles) {
					continue;
				}

				auto anchorPos = CalculateSubtitleAnchorPos(subtitleInfo);
				auto zDepth = ImGui::WorldToScreenLoc(anchorPos, params.pos);
				if (zDepth < 0.0f) {
					continue;
				}

				params.alpha = std::bit_cast<float>(subtitleInfo.textAlpha());
				if (params.alpha < 0.01f) {
					continue;
				}

				GetProcessedSubtitle(subtitleInfo.subtitle).DrawDualSubtitle(params);
			}
		}
	}
	ImGui::End();
}
