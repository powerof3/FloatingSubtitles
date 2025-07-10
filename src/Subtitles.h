#pragma once

#include "Localization.h"
#include "RE.h"

namespace ImGui
{
	struct StyleParams;
}

namespace Subtitles
{
	struct ScreenParams
	{
		ImVec2 pos{};
		float  alpha{ 1.0f };
		float  spacing;
	};

	struct Subtitle
	{
		struct Line
		{
			std::string line;
			ImVec2      lineSize;
		};

		Subtitle() = default;
		Subtitle(const char* a_subtitle, std::uint32_t a_maxChars);

		void DrawSubtitle(const ImVec2& a_screenPos, const ImGui::StyleParams& a_params, float a_lineHeight, float& a_startPosY) const;

		std::vector<Line> lines;

	private:
		static std::vector<Line> WrapText(const char* text, std::uint32_t maxWidth);
	};

	struct DualSubtitle
	{
		DualSubtitle() = default;
		DualSubtitle(const char* a_subtitle, std::uint32_t a_maxChars);
		DualSubtitle(const std::string& a_primarySub, std::uint32_t a_maxCharsPrimary, const std::string& a_secondarySub, std::uint32_t a_maxCharsSecondary);

		void DrawDualSubtitle(const ScreenParams& a_screenParams) const;

		// members
		Subtitle primary{};
		Subtitle secondary{};
	};

	class Manager : public REX::Singleton<Manager>
	{
	public:
		void OnDataLoaded();

		void LoadMCMSettings(CSimpleIniA& a_ini);
		void PostMCMSettingsLoad();

		void Draw();

		void AddSubtitle(RE::SubtitleManager* a_manager, const char* subtitle);
		void AddProcessedSubtitle(const char* subtitle);
		void UpdateSubtitles(RE::SubtitleManager* a_manager) const;

		void SetVisible(bool a_visible);

		bool ShowGeneralSubtitles() const;
		bool ShowDialogueSubtitles() const;

	private:
		struct Settings
		{
			void LoadMCMSettings(CSimpleIniA& a_ini);

			float subtitleHeadOffset{ 15.0f };
			float subtitleSpacing{ 0.5f };
			float maxDistanceStart{ 2048.0f };
			bool  showGeneralSubtitles{ true };
			bool  showDialogueSubtitles{ false };
			bool  showDualSubs{ false };
			bool  staticSubtitles{ false };
			bool  BTPSInstalled{ false };
			bool  useBTPSWidgetPosition{ true };
			bool  doRayCastChecks{ true };
			bool  fadeSubtitles{ true };
			float fadeSubtitleAlpha{ 0.35f };
		};

		using SubtitleFlag = RE::SubtitleInfoEx::Flag;

		using RWLock = std::shared_mutex;
		using ReadLocker = std::shared_lock<RWLock>;
		using WriteLocker = std::unique_lock<RWLock>;

		bool IsVisible() const;

		RE::NiPoint3 CalculateSubtitleAnchorPos(const RE::SubtitleInfoEx& a_subInfo) const;
		RE::NiPoint3 GetSubtitleAnchorPosImpl(const RE::TESObjectREFRPtr& a_ref, float a_height) const;

		DualSubtitle        CreateDualSubtitles(const char* subtitle);
		const DualSubtitle& GetProcessedSubtitles(const RE::BSString& subtitle);
		void                RebuildProcessedSubtitles();

		// members
		mutable RWLock                     subtitleLock;
		FlatMap<std::string, DualSubtitle> processedSubtitles;
		Settings                           previous;
		Settings                           current;
		float                              maxDistanceStartSq{ 4194304.0f };
		float                              maxDistanceEndSq{ 4624220.16f };
		bool                               visible{ true };
		LocalizedSubtitles                 localizedSubs;
	};
}
