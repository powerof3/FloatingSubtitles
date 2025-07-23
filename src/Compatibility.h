#pragma once

#include "API/BTPS_API_decl.h"
#include "API/NND_API.h"
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

	struct NND
	{
		void        GetAPI();
		std::string GetReferenceName(const RE::TESObjectREFRPtr& a_ref) const;

		NND_API::IVNND2* api{};
	};

	void LoadModSettings();
	void LoadAPIs();

	std::optional<float> GetWidgetPosZ(const RE::TESObjectREFRPtr& a_ref, bool a_BTPS, bool a_trueHUD) const;
	float                GetResolutionScale() const;
	std::string          GetReferenceName(const RE::TESObjectREFRPtr& a_ref) const;
	bool                 ACCInstalled() const { return altCCInstalled; }

	DisplayTweaks displayTweaks;
	BTPS          btps;
	TrueHUD       trueHUD;
	NND           nnd;
	bool          altCCInstalled{ false };

	static ModAPIHandler instance;
};

inline constinit ModAPIHandler ModAPIHandler::instance;
