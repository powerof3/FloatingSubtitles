#pragma once

#include "API/BTPS_API_decl.h"

namespace DisplayTweaks
{
	void  LoadSettings(const CSimpleIniA& a_ini);
	float GetResolutionScale();

	// members
	inline float resolutionScale{ 1.0f };
	inline bool  borderlessUpscale{ false };
}

namespace BetterThirdPersonSelection
{
	void         GetAPI();
	bool         IsEnabled();
	RE::NiPoint3 GetBTPSWidgetPos();

	// member
	inline BTPS_API_decl::API_V0* api;
}
