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

	void Draw();

	void AddSubtitle(RE::SubtitleManager* a_manager, const char* a_subtitle);
	void UpdateSubtitleInfo(RE::SubtitleManager* a_manager);

	void SetVisible(bool a_visible);

	bool ShowGeneralSubtitles() const;
	bool ShowDialogueSubtitles() const;

	bool HandlesDialogueSubtitles(RE::BSString* a_text) const;

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

	bool IsVisible() const;

	static bool HasObjectTag(RE::BSString& a_text);

	DualSubtitle CreateDualSubtitles(const char* subtitle) const;

	void                AddProcessedSubtitle(const char* subtitle);
	const DualSubtitle& GetProcessedSubtitle(const RE::BSString& a_subtitle);
	void                RebuildProcessedSubtitles();

	RE::NiPoint3        CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const;
	static RE::NiPoint3 GetSubtitleAnchorPosImpl(const RE::TESObjectREFRPtr& a_ref, float a_height);

	void CalculateAlphaModifier(RE::SubtitleInfoEx& a_subInfo) const;
	void CalculateVisibility(RE::SubtitleInfoEx& a_subInfo);

	std::string GetScaleformSubtitle(const RE::BSString& a_subtitle);
	void        BuildOffscreenSubtitle(const RE::TESObjectREFRPtr& a_speaker, const RE::BSString& a_subtitle);
	void        QueueOffscreenSubtitle() const;
	void        QueueDialogueSubtitle(const RE::BSString& a_subtitle);

	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

	// members
	mutable RWLock                     subtitleLock;
	FlatMap<std::string, DualSubtitle> processedSubtitles;
	MCMSettings                        settings;
	float                              maxDistanceStartSq{ 4194304.0f };
	float                              maxDistanceEndSq{ 4624220.16f };
	std::int32_t                       speakerColor{};
	bool                               visible{ true };
	LocalizedSubtitles                 localizedSubs;
	std::string                        offscreenSub{};
	std::string                        lastOffscreenSub{};
	std::string                        talkingActivatorSub{};
	std::string                        lasttalkingActivatorSub{};
	std::uint32_t                      offscreenSubCount{ 0 };

	static constexpr std::string_view objectTag{ "[REF]" };
};
