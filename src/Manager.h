#pragma once

#include "Localization.h"
#include "RE.h"
#include "Subtitles.h"

class Manager : 
	public REX::Singleton<Manager>,
	public RE::BSTEventSink<RE::MenuOpenCloseEvent>

{
public:
	void OnDataLoaded();
	void LoadMCMSettings();

	bool SkipRender() const;
	void Draw();
	void SetVisible(bool a_visible);

	void AddSubtitle(RE::SubtitleManager* a_manager, const char* a_subtitle);
	void UpdateSubtitleInfo(RE::SubtitleManager* a_manager);

	bool HandlesGeneralSubtitles() const;
	bool HandlesDialogueSubtitles() const;

private:
	enum class OffscreenSubtitle
	{
		kDisabled = 0,
		kSingle,
		kDual
	};

	struct MCMSettings
	{
		std::pair<bool, bool> LoadMCMSettings(const CSimpleIniA& a_ini);

		struct StoredSettings
		{
			bool showGeneralSubtitles{ true };
			bool showDialogueSubtitles{ false };
			bool showDualSubs{ false };
		};

		StoredSettings    previous{};
		StoredSettings    current{};
		bool              showSpeakerName{ false };
		float             subtitleHeadOffset{ 15.0f };
		float             subtitleSpacing{ 0.5f };
		bool              useBTPSWidgetPosition{ true };
		bool              useTrueHUDWidgetPosition{ true };
		bool              doRayCastChecks{ true };
		float             obscuredSubtitleAlpha{ 0.35f };
		float             subtitleAlphaPrimary{ 1.0f };
		float             subtitleAlphaSecondary{ 1.0f };
		OffscreenSubtitle offscreenSubs{ OffscreenSubtitle::kSingle };
		std::uint32_t     maxOffscreenSubs{ 3 };
	};

	using SubtitleFlag = RE::SubtitleInfoEx::Flag;

	using RWLock = std::shared_mutex;
	using ReadLocker = std::shared_lock<RWLock>;
	using WriteLocker = std::unique_lock<RWLock>;

	bool ShowGeneralSubtitles() const;
	bool ShowDialogueSubtitles() const;

	DualSubtitle CreateDualSubtitles(const char* subtitle) const;

	void                AddProcessedSubtitle(const char* subtitle);
	const DualSubtitle& GetProcessedSubtitle(const RE::BSString& a_subtitle);
	void                RebuildProcessedSubtitles();

	RE::NiPoint3        CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const;
	static RE::NiPoint3 GetSubtitleAnchorPosImpl(const RE::TESObjectREFRPtr& a_ref, float a_height);

	void CalculateAlphaModifier(RE::SubtitleInfoEx& a_subInfo) const;
	void CalculateVisibility(RE::SubtitleInfoEx& a_subInfo);

	std::string GetScaleformSubtitle(const RE::BSString& a_subtitle, bool a_dual);
	void        BuildOffscreenSubtitle(const RE::TESObjectREFRPtr& a_speaker, const RE::BSString& a_subtitle, bool a_dialogueSubtitle = false);
	void        QueueOffscreenSubtitle() const;

	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

	// members
	mutable RWLock                     subtitleLock;
	FlatMap<std::string, DualSubtitle> processedSubtitles;
	MCMSettings                        settings;
	float                              maxDistanceStartSq{ 4194304.0f };
	float                              maxDistanceEndSq{ 4624220.16f };
	ImU32                              speakerColorU32{};
	ImVec4                             speakerColorFloat4{};
	bool                               visible{ true };
	LocalizedSubtitles                 localizedSubs;
	std::string                        talkingActivatorSub{};
	std::string                        lastTalkingActivatorSub{};
	std::string                        offscreenSub{};
	std::string                        lastOffscreenSub{};
	std::uint32_t                      offscreenSubCount{ 0 };
};
