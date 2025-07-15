#pragma once

#include "Localization.h"
#include "RE.h"
#include "Subtitles.h"

class Manager : public REX::Singleton<Manager>
{
public:
	void OnDataLoaded();
	void LoadMCMSettings();

	void Draw();

	void AddSubtitle(RE::SubtitleManager* a_manager, const char* a_subtitle);
	void UpdateSubtitleInfo(RE::SubtitleManager* a_manager);

	void SetVisible(bool a_visible);

	bool HandlesGeneralSubtitles(RE::BSString& a_text) const;
	bool HandlesDialogueSubtitles(RE::BSString* a_text) const;

private:
	struct MCMSettings
	{
		void LoadMCMSettings(CSimpleIniA& a_ini);

		float        subtitleHeadOffset{ 15.0f };
		float        subtitleSpacing{ 0.5f };
		bool         showGeneralSubtitles{ true };
		bool         showDialogueSubtitles{ false };
		bool         showDualSubs{ false };
		bool         useBTPSWidgetPosition{ true };
		bool         useTrueHUDWidgetPosition{ true };
		bool         doRayCastChecks{ true };
		bool         fadeSubtitles{ true };
		float        fadeSubtitleAlpha{ 0.35f };
		bool         useOffscreenSubs;
		std::uint32_t maxOffscreenSubs{ 3 };
	};

	using SubtitleFlag = RE::SubtitleInfoEx::Flag;

	using RWLock = std::shared_mutex;
	using ReadLocker = std::shared_lock<RWLock>;
	using WriteLocker = std::unique_lock<RWLock>;

	void LoadMCMSettings(CSimpleIniA& a_ini);

	bool ShowGeneralSubtitles() const;
	bool ShowDialogueSubtitles() const;

	bool IsVisible() const;

	DualSubtitle CreateDualSubtitles(const char* subtitle) const;

	void                AddProcessedSubtitle(const char* subtitle);
	const DualSubtitle& GetProcessedSubtitle(const RE::BSString& a_subtitle);
	void                RebuildProcessedSubtitles();

	RE::NiPoint3        CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const;
	static RE::NiPoint3 GetSubtitleAnchorPosImpl(const RE::TESObjectREFRPtr& a_ref, float a_height);

	void CalculateAlpha(RE::SubtitleInfoEx& a_subInfo) const;

	void BuildOffscreenSubtitle(std::string& a_subtitle, const RE::TESObjectREFRPtr& a_speaker, const RE::BSString& a_subtitleRaw);

	// members
	mutable RWLock                     subtitleLock;
	FlatMap<std::string, DualSubtitle> processedSubtitles;
	MCMSettings                        previous;
	MCMSettings                        current;
	float                              maxDistanceStartSq{ 4194304.0f };
	float                              maxDistanceEndSq{ 4624220.16f };
	std::int32_t                       speakerColor{};
	bool                               visible{ true };
	LocalizedSubtitles                 localizedSubs;
	std::string                        offscreenSub{};
	std::string                        lastOffscreenSub{};
	std::uint32_t                      offscreenSubCount{ 0 };
};
