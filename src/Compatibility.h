#pragma once

#include "API/BTPS_API_decl.h"

namespace Compatibility
{
	namespace DisplayTweaks
	{
		void  LoadSettings();
		float GetResolutionScale();

		// members
		inline float resolutionScale{ 1.0f };
		inline bool  borderlessUpscale{ false };
	}

	namespace BTPS
	{
		void         GetAPI();
		RE::NiPoint3 GetWidgetPos();

		// member
		inline BTPS_API_decl::API_V0* api;
	}
}
