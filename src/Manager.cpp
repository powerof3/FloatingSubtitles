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

	showSpeakerName = a_ini.GetBoolValue("Settings", "bShowSpeakerName", showSpeakerName);

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

	const auto gameMaxDistance = "fMaxSubtitleDistance:Interface"_ini.value();
	maxDistanceStartSq = gameMaxDistance * gameMaxDistance;
	maxDistanceEndSq = (gameMaxDistance * 1.05f) * (gameMaxDistance * 1.05f);

	logger::info("Max subtitle distance: {:.2f} (start), {:.2f} (end)", gameMaxDistance, gameMaxDistance * 1.05f);

	speakerColorU32 = "iSubtitleSpeakerNameColor:Interface"_ini.value();
	speakerColorFloat4 = ImGui::ColorConvertU32ToFloat4(speakerColorU32);
	speakerColorFloat4.w = 1.0f;

	logger::info("Subtitle speaker color: {},{},{} ({:X})", speakerColorFloat4.x, speakerColorFloat4.y, speakerColorFloat4.z, speakerColorU32);
}

bool Manager::SkipRender() const
{
	const bool showGeneral = ShowGeneralSubtitles();
	const bool showDialogue = ShowDialogueSubtitles();

	return !showGeneral && !showDialogue || !visible;
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

bool Manager::HandlesGeneralSubtitles(RE::BSString& a_text) const
{
	if (HasObjectTag(a_text)) {
		return false;
	}

	return ShowGeneralSubtitles();
}

bool Manager::ShowGeneralSubtitles() const
{
	return "bGeneralSubtitles:Interface"_pref.value() && settings.current.showGeneralSubtitles;
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
	return "bDialogueSubtitles:Interface"_pref.value() && settings.current.showDialogueSubtitles && !ModAPIHandler::GetSingleton()->ACCInstalled();
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
				if (const auto ref = subInfo.speaker.get()) {
					if (!ref->IsActor()) {
						subInfo.subtitle = std::format("{}{}", a_subtitle, objectTag);
					}
				}
				subInfo.setFlag(RE::SubtitleInfoEx::Flag::kInitialized, true);
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
	if (a_subInfo.isFlagSet(SubtitleFlag::kOffscreen)) {
		return;
	}

	const auto ref = a_subInfo.speaker.get();
	const auto actor = ref->As<RE::Actor>();

	float alpha = 1.0f;

	if (a_subInfo.isFlagSet(SubtitleFlag::kObscured)) {
		alpha *= settings.obscuredSubtitleAlpha;
	}

	if (a_subInfo.targetDistance > maxDistanceStartSq) {
		const float t = (a_subInfo.targetDistance - maxDistanceStartSq) / (maxDistanceEndSq - maxDistanceStartSq);

		constexpr auto cubicEaseOut = [](float t) -> float {
			return 1.0f - (t * t * t);
		};

		alpha *= 1.0f - cubicEaseOut(t);
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
		offscreenSub.append(std::format("<font color='#{:6X}'>{}</font>: {}", speakerColorU32, name, scaleformSub));
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

		for (auto& subInfo : subtitleArray) {
			if (const auto& ref = subInfo.speaker.get()) {
				if (!subInfo.isFlagSet(SubtitleFlag::kInitialized)) {
					subInfo.flagsRaw() = 0;
					subInfo.alphaModifier() = std::bit_cast<std::uint32_t>(1.0f);
					if (!ref->IsActor()) {
						if (std::string text = subInfo.subtitle.c_str(); !text.ends_with(objectTag)) {
							subInfo.subtitle = std::format("{}{}", text, objectTag);
						}
					}
					subInfo.setFlag(SubtitleFlag::kInitialized, true);
				}

				if (!ref->IsActor()) {
					continue;
				}

				if (ref->IsPlayerRef()) {
					if (auto pcCamera = RE::PlayerCamera::GetSingleton(); pcCamera && pcCamera->IsInFirstPerson()) {
						BuildOffscreenSubtitle(ref, subInfo.subtitle);
						continue;
					}
				}

				bool isDialogueSpeaker = menuTopicMgr->IsCurrentSpeaker(subInfo.speaker);

				if ((isDialogueSpeaker && !showDialogue) ||
					(!isDialogueSpeaker && !showGeneral) ||
					!subInfo.forceDisplay && subInfo.targetDistance > maxDistanceEndSq) {
					continue;
				}

				CalculateVisibility(subInfo);
				CalculateAlphaModifier(subInfo);
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
	const auto subtitleManager = RE::SubtitleManager::GetSingleton();
	const auto menuTopicMgr = RE::MenuTopicManager::GetSingleton();

	const bool showGeneral = ShowGeneralSubtitles();
	const bool showDialogue = ShowDialogueSubtitles();

	RE::BSSpinLockGuard gameLocker(subtitleManager->lock);
	{
		DualSubtitle::ScreenParams params;
		params.spacing = settings.subtitleSpacing;
		params.speakerColor = speakerColorFloat4;

		auto& subtitleArray = reinterpret_cast<RE::BSTArray<RE::SubtitleInfoEx>&>(subtitleManager->subtitles);

		for (auto& subInfo : subtitleArray | std::views::reverse) {  // reverse order so closer subtitles get rendered on top
			if (const auto& ref = subInfo.speaker.get()) {
				if (!ref->IsActor()) {
					continue;
				}

				bool isDialogueSpeaker = menuTopicMgr->IsCurrentSpeaker(subInfo.speaker);

				if ((isDialogueSpeaker && !showDialogue) ||
					(!isDialogueSpeaker && !showGeneral) ||
					!subInfo.forceDisplay && subInfo.targetDistance > maxDistanceEndSq) {
					continue;
				}

				if (subInfo.isFlagSet(SubtitleFlag::kOffscreen) || subInfo.isFlagSet(SubtitleFlag::kObscured) && settings.obscuredSubtitleAlpha == 0.0f) {
					continue;
				}

				auto anchorPos = CalculateSubtitleAnchorPos(subInfo);
				auto zDepth = ImGui::WorldToScreenLoc(anchorPos, params.pos);
				if (zDepth < 0.0f) {
					continue;
				}

				auto alphaMult = std::bit_cast<float>(subInfo.alphaModifier());
				params.alphaPrimary = settings.subtitleAlphaPrimary * alphaMult;
				params.alphaSecondary = settings.subtitleAlphaSecondary * alphaMult;
				if (settings.showSpeakerName && !RE::IsCrosshairRef(ref)) {
					params.speakerName = ModAPIHandler::GetSingleton()->GetReferenceName(ref);
				} else {
					params.speakerName.clear();
				}

				GetProcessedSubtitle(subInfo.subtitle).DrawDualSubtitle(params);
			}
		}
	}
}
