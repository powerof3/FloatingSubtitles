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
		float  spacing{ 0.5f };
	};

	struct Subtitle
	{
		struct Line
		{
			std::string line;
			ImVec2      lineSize;
		};

		Subtitle() = default;
		Subtitle(const LocalizedSubtitle& a_subtitle);

		void DrawSubtitle(float a_posX, float& a_posY, const ImGui::StyleParams& a_params, float a_lineHeight) const;

		std::vector<Line> lines;

	private:
		static std::vector<Line> WrapText(const std::string& text, std::uint32_t maxWidth);
	};

	struct DualSubtitle
	{
		DualSubtitle() = default;
		DualSubtitle(const LocalizedSubtitle& a_primarySubtitle);
		DualSubtitle(const LocalizedSubtitle& a_primarySubtitle, const LocalizedSubtitle& a_secondarySubtitle);

		void DrawDualSubtitle(const ScreenParams& a_screenParams) const;

		// members
		Subtitle primary{};
		Subtitle secondary{};
	};

	class Manager : public REX::Singleton<Manager>
	{
	public:
		void OnDataLoaded();
		void LoadMCMSettings();

		void Draw();

		void AddSubtitle(RE::SubtitleManager* a_manager, const char* subtitle);
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
			float maxDistanceStart{ 2048.0f };
			bool  showGeneralSubtitles{ true };
			bool  showDialogueSubtitles{ false };
			bool  showDualSubs{ false };
			bool  useBTPSWidgetPosition{ true };
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
}
