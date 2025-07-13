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
	void AddProcessedSubtitle(const char* subtitle);
	void UpdateSubtitles(RE::SubtitleManager* a_manager) const;

	void SetVisible(bool a_visible);

	bool ShowGeneralSubtitles() const;
	bool ShowDialogueSubtitles() const;

private:
	struct MCMSettings
	{
		void LoadMCMSettings(CSimpleIniA& a_ini);

		float subtitleHeadOffset{ 15.0f };
		float subtitleSpacing{ 0.5f };
		bool  showGeneralSubtitles{ true };
		bool  showDialogueSubtitles{ false };
		bool  showDualSubs{ false };
		bool  useBTPSWidgetPosition{ true };
		bool  useTrueHUDWidgetPosition{ true };
		bool  doRayCastChecks{ true };
		bool  fadeSubtitles{ true };
		float fadeSubtitleAlpha{ 0.35f };
	};

	using SubtitleFlag = RE::SubtitleInfoEx::Flag;

	using RWLock = std::shared_mutex;
	using ReadLocker = std::shared_lock<RWLock>;
	using WriteLocker = std::unique_lock<RWLock>;

	void LoadMCMSettings(CSimpleIniA& a_ini);

	bool IsVisible() const;

	RE::NiPoint3        CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const;
	static RE::NiPoint3 GetSubtitleAnchorPosImpl(const RE::TESObjectREFRPtr& a_ref, float a_height);

	DualSubtitle        CreateDualSubtitles(const char* subtitle) const;
	const DualSubtitle& GetProcessedSubtitles(const RE::BSString& subtitle);
	void                RebuildProcessedSubtitles();

	// members
	mutable RWLock                     subtitleLock;
	FlatMap<std::string, DualSubtitle> processedSubtitles;
	MCMSettings                        previous;
	MCMSettings                        current;
	float                              maxDistanceStartSq{ 4194304.0f };
	float                              maxDistanceEndSq{ 4624220.16f };
	bool                               visible{ true };
	LocalizedSubtitles                 localizedSubs;
};
