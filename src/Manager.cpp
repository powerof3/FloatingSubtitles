#include "Manager.h"

#include "Compatibility.h"
#include "ImGui/Util.h"
#include "RayCaster.h"
#include "SettingLoader.h"

std::pair<bool, bool> Manager::MCMSettings::LoadMCMSettings(const CSimpleIniA& a_ini)
{
	previous = current;

	current.showGeneralSubtitles = a_ini.GetBoolValue("Settings", "bGeneralSubtitles", current.showGeneralSubtitles);
	current.showDialogueSubtitles = a_ini.GetBoolValue("Settings", "bDialogueSubtitles", current.showDialogueSubtitles);
	current.showDualSubs = a_ini.GetBoolValue("Settings", "bDualSubtitles", current.showDualSubs);

	subtitleHeadOffset = static_cast<float>(a_ini.GetDoubleValue("Settings", "fHeadOffset", 20.0)) * ModAPIHandler::GetSingleton()->GetResolutionScale();

	doRayCastChecks = a_ini.GetBoolValue("Settings", "bEnableRaycastChecks", doRayCastChecks);

	obscuredSubtitleAlpha = static_cast<float>(a_ini.GetDoubleValue("Settings", "fObscuredSubtitleOpacity", obscuredSubtitleAlpha));

	subtitleSpacing = static_cast<float>(a_ini.GetDoubleValue("Settings", "fDualSubtitleSpacing", subtitleSpacing));

	useBTPSWidgetPosition = a_ini.GetBoolValue("Settings", "bUseBTPSWidgetPosition", useBTPSWidgetPosition);
	useTrueHUDWidgetPosition = a_ini.GetBoolValue("Settings", "bUseTrueHUDWidgetPosition", useTrueHUDWidgetPosition);

	subtitleAlphaPrimary = static_cast<float>(a_ini.GetDoubleValue("Settings", "fSubtitleAlphaPrimary", subtitleAlphaPrimary));
	subtitleAlphaSecondary = static_cast<float>(a_ini.GetDoubleValue("Settings", "fSubtitleAlphaSecondary", subtitleAlphaSecondary));

	offscreenSubs = static_cast<OffscreenSubtitle>(a_ini.GetLongValue("Settings", "iOffscreenSubtitles", std::to_underlying(offscreenSubs)));
	maxOffscreenSubs = a_ini.GetLongValue("Settings", "iMaxOffscreenSubtitles", maxOffscreenSubs);

	return {
		previous.showDualSubs != current.showDualSubs, (!previous.showGeneralSubtitles && current.showGeneralSubtitles || !previous.showDialogueSubtitles && current.showDialogueSubtitles)
	};  // rebuild subs, hide subtitles
}

void Manager::LoadMCMSettings()
{
	SettingLoader::GetSingleton()->Load(FileType::kMCM, [this](auto& ini) {
		bool rebuildSubs = false;
		bool hideSubs = false;

		std::tie(rebuildSubs, hideSubs) = settings.LoadMCMSettings(ini);
		rebuildSubs |= localizedSubs.LoadMCMSettings(ini);

		// force hide vanilla subtitle
		if (hideSubs) {
			RE::SendHUDMenuMessage(RE::HUD_MESSAGE_TYPE::kHideSubtitle);
		}

		if (rebuildSubs) {
			RebuildProcessedSubtitles();
		}
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

bool Manager::HasObjectTag(RE::BSString& a_text)
{
	std::string text = a_text.c_str();
	if (text.ends_with(objectTag)) {
		text.erase(text.length() - objectTag.length());
		a_text = text;
		return true;
	}
	return false;
}

bool Manager::ShowGeneralSubtitles() const
{
	return RE::ShowGeneralSubsGame() && settings.current.showGeneralSubtitles;
}

bool Manager::HandlesDialogueSubtitles(RE::BSString* a_text) const
{
	if (a_text && HasObjectTag(*a_text)) {
		return false;
	}

	return ShowDialogueSubtitles();
}

bool Manager::ShowDialogueSubtitles() const
{
	return RE::ShowDialogueSubsGame() && settings.current.showDialogueSubtitles;
}

DualSubtitle Manager::CreateDualSubtitles(const char* subtitle) const
{
	auto primarySub = localizedSubs.GetPrimarySubtitle(subtitle);
	if (settings.current.showDualSubs) {
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
				subInfo.alphaModifier() = std::bit_cast<std::uint32_t>(1.0f);
				subInfo.setFlag(RE::SubtitleInfoEx::Flag::kInitialized, true);
				if (const auto ref = subInfo.speaker.get()) {
					if (!ref->IsActor()) {
						subInfo.subtitle = std::format("{}{}", a_subtitle, objectTag);
					}
				}
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

void Manager::CalculateAlphaModifier(RE::SubtitleInfoEx& a_subInfo) const
{
	const auto ref = a_subInfo.speaker.get();
	const auto actor = ref->As<RE::Actor>();

	float alpha = 1.0f;

	if (a_subInfo.isFlagSet(SubtitleFlag::kObscured)) {
		alpha *= settings.obscuredSubtitleAlpha;
	}

	if (a_subInfo.targetDistance > maxDistanceStartSq) {
		const float t = (a_subInfo.targetDistance - maxDistanceStartSq) / (maxDistanceEndSq - maxDistanceStartSq);
		alpha *= 1.0f - glm::cubicEaseOut(t);
	} else if (auto high = actor->GetHighProcess(); high && high->fadeAlpha < 1.0f) {
		alpha *= high->fadeAlpha;
	} else if (actor->IsDead() && actor->voiceTimer < 1.0f) {
		alpha *= actor->voiceTimer;
	}

	a_subInfo.alphaModifier() = std::bit_cast<std::uint32_t>(alpha);
}

std::string Manager::GetScaleformSubtitle(const RE::BSString& a_subtitle)
{
	auto subtitle = GetProcessedSubtitle(a_subtitle).GetScaleformCompatibleSubtitle(settings.offscreenSubs == OffscreenSubtitle::kDual);
	return subtitle.empty() ? a_subtitle.c_str() : subtitle;
}

void Manager::BuildOffscreenSubtitle(const RE::TESObjectREFRPtr& a_speaker, const RE::BSString& a_subtitle)
{
	if (offscreenSubCount > settings.maxOffscreenSubs) {
		return;
	}

	bool dualSubs = settings.offscreenSubs == OffscreenSubtitle::kDual;

	auto scaleformSub = GetScaleformSubtitle(a_subtitle);
	if (std::string name = ModAPIHandler::GetSingleton()->GetReferenceName(a_speaker); !name.empty()) {
		offscreenSub.append(std::format("<font color='#{:6X}'>{}</font>: {}", speakerColor, name, scaleformSub));
	} else {
		offscreenSub.append(scaleformSub);
	}
	offscreenSub.append(dualSubs && scaleformSub.contains("\n") ? "\n\n" : "\n");

	offscreenSubCount++;
}

void Manager::QueueOffscreenSubtitle() const
{
	if (settings.offscreenSubs == OffscreenSubtitle::kDisabled) {
		return;
	}

	if (lastOffscreenSub != offscreenSub) {
		const std::string currentSub = offscreenSub;
		const std::string prevSub = lastOffscreenSub;

		SKSE::GetTaskInterface()->AddUITask([currentSub, prevSub]() {
			if (auto hudMenu = RE::UI::GetSingleton()->GetMenu<RE::HUDMenu>()) {
				if (!prevSub.empty()) {
					RE::GFxValue subtitleText(prevSub);
					hudMenu->root.Invoke("HideSubtitle", nullptr, &subtitleText, 1);
				}
				if (!currentSub.empty()) {
					RE::GFxValue subtitleText(currentSub);
					hudMenu->root.Invoke("ShowSubtitle", nullptr, &subtitleText, 1);
				}
			}
		});
	}
}

void Manager::CalculateVisibility(RE::SubtitleInfoEx& a_subInfo)
{
	const auto ref = a_subInfo.speaker.get();
	const auto actor = ref->As<RE::Actor>();

	switch (RayCaster(actor).GetResult(false)) {
	case RayCaster::Result::kOffscreen:
		{
			a_subInfo.setFlag(RE::SubtitleInfoEx::Flag::kOffscreen, true);
			a_subInfo.setFlag(RE::SubtitleInfoEx::Flag::kObscured, false);

			BuildOffscreenSubtitle(ref, a_subInfo.subtitle);
		}
		break;
	case RayCaster::Result::kObscured:
		{
			a_subInfo.setFlag(RE::SubtitleInfoEx::Flag::kObscured, true);
			a_subInfo.setFlag(RE::SubtitleInfoEx::Flag::kOffscreen, false);
		}
		break;
	case RayCaster::Result::kVisible:
		{
			a_subInfo.setFlag(RE::SubtitleInfoEx::Flag::kOffscreen, false);
			a_subInfo.setFlag(RE::SubtitleInfoEx::Flag::kObscured, false);
		}
		break;
	default:
		std::unreachable();
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
		offscreenSubCount = 0;

		for (auto& subtitleInfo : subtitleArray) {
			if (!subtitleInfo.isFlagSet(RE::SubtitleInfoEx::Flag::kInitialized)) {
				subtitleInfo.flagsRaw() = 0;
				subtitleInfo.alphaModifier() = std::bit_cast<std::uint32_t>(1.0f);
				subtitleInfo.setFlag(RE::SubtitleInfoEx::Flag::kInitialized, true);
			}

			if (const auto& ref = subtitleInfo.speaker.get()) {
				bool isDialogueSpeaker = menuTopicMgr->IsCurrentSpeaker(subtitleInfo.speaker);

				if ((isDialogueSpeaker && !showDialogue) ||
					(!isDialogueSpeaker && !showGeneral) ||
					!subtitleInfo.forceDisplay && subtitleInfo.targetDistance > maxDistanceEndSq) {
					continue;
				}

				if (!ref->IsActor()) {
					continue;
				}

				CalculateVisibility(subtitleInfo);
				CalculateAlphaModifier(subtitleInfo);
			}
		}

		QueueOffscreenSubtitle();
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

RE::NiPoint3 Manager::CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const
{
	const auto ref = a_subInfo.speaker.get();
	const auto height = ref->GetHeight();

	auto pos = GetSubtitleAnchorPosImpl(ref, height);
	auto offset = settings.subtitleHeadOffset;

	if (auto overridePosZ = ModAPIHandler::GetSingleton()->GetWidgetPosZ(ref, settings.useBTPSWidgetPosition, settings.useTrueHUDWidgetPosition)) {
		pos.z = *overridePosZ;
		offset = settings.subtitleHeadOffset * 0.75f;
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

	const auto subtitleManager = RE::SubtitleManager::GetSingleton();
	const auto menuTopicMgr = RE::MenuTopicManager::GetSingleton();
	auto&      subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(subtitleManager->subtitles);

	ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
	ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

	ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
	{
		RE::BSSpinLockGuard gameLocker(subtitleManager->lock);
		{
			DualSubtitle::ScreenParams params;
			params.spacing = settings.subtitleSpacing;
			params.alphaPrimary = settings.subtitleAlphaPrimary;
			params.alphaSecondary = settings.subtitleAlphaSecondary;

			for (auto& subtitleInfo : subtitleArray | std::views::reverse) {  // reverse order so closer subtitles get rendered on top
				if (const auto& ref = subtitleInfo.speaker.get()) {
					if (!ref->IsActor()) {
						continue;
					}

					bool isDialogueSpeaker = menuTopicMgr->IsCurrentSpeaker(subtitleInfo.speaker);

					if ((isDialogueSpeaker && !showDialogue) ||
						(!isDialogueSpeaker && !showGeneral) ||
						!subtitleInfo.forceDisplay && subtitleInfo.targetDistance > maxDistanceEndSq) {
						continue;
					}

					if (subtitleInfo.isFlagSet(SubtitleFlag::kOffscreen) || subtitleInfo.isFlagSet(SubtitleFlag::kObscured) && settings.obscuredSubtitleAlpha == 0.0f) {
						continue;
					}

					auto anchorPos = CalculateSubtitleAnchorPos(subtitleInfo);
					auto zDepth = ImGui::WorldToScreenLoc(anchorPos, params.pos);
					if (zDepth < 0.0f) {
						continue;
					}

					auto alphaMult = std::bit_cast<float>(subtitleInfo.alphaModifier());
					params.alphaPrimary *= alphaMult;
					params.alphaSecondary *= alphaMult;

					GetProcessedSubtitle(subtitleInfo.subtitle).DrawDualSubtitle(params);
				}
			}
		}
	}
	ImGui::End();
}
