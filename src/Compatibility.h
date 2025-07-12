#pragma once

#include "API/BTPS_API_decl.h"
#include "API/TrueHUDAPI.h"

class ModAPIHandler
{
public:
	static ModAPIHandler* GetSingleton()
	{
		return &instance;
	}

	struct DisplayTweaks
	{
		void  LoadSettings();
		float GetResolutionScale() const;

		float resolutionScale{ 1.0f };
		bool  borderlessUpscale{ false };
	};

	struct BTPS
	{
		void GetAPI();
		void GetWidgetPos(const RE::TESObjectREFRPtr& a_ref, std::optional<float>& a_posZOut) const;

		BTPS_API_decl::API_V0* api{};
		bool                   validAPI{};
	};

	struct TrueHUD
	{
		enum class WidgetAnchor : std::uint32_t
		{
			kBody = 0,
			kHead = 1,
		};

		void GetAPI();
		void GetWidgetPos(const RE::TESObjectREFRPtr& a_ref, std::optional<float>& a_posZOut) const;

		TRUEHUD_API::IVTrueHUD4* api{};
		float                    infoBarOffsetZ{};
		WidgetAnchor             infoBarAnchor{};
	};

	void LoadModSettings();
	void LoadAPIs();

	std::optional<float> GetWidgetPosZ(const RE::TESObjectREFRPtr& a_ref) const;
	float                GetResolutionScale() const;

	DisplayTweaks displayTweaks;
	BTPS          btps;
	TrueHUD       trueHUD;

	static ModAPIHandler instance;
};

inline constinit ModAPIHandler ModAPIHandler::instance;
