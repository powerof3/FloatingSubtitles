#include "Compatibility.h"

#include "RE.h"
#include "Settings.h"

float DisplayTweaks::GetResolutionScale()
{
	return borderlessUpscale ?
	           resolutionScale :
	           RE::BSGraphics::Renderer::GetScreenSize().height / 1080.0f;
}

void DisplayTweaks::LoadSettings(const CSimpleIniA& a_ini)
{
	resolutionScale = static_cast<float>(a_ini.GetDoubleValue("Render", "ResolutionScale", resolutionScale));
	borderlessUpscale = static_cast<float>(a_ini.GetBoolValue("Render", "BorderlessUpscale", borderlessUpscale));
}

void BetterThirdPersonSelection::GetAPI()
{
	api = reinterpret_cast<BTPS_API_decl::API_V0*>(BTPS_API_decl::RequestPluginAPI_V0());
	if (api) {
		logger::info("Obtained BTPS API");
		Settings::GetSingleton()->LoadSettingsBTPS([](auto& ini) {
			auto widgetPos = ini.GetDoubleValue("Widgets", "fWidgetZOffsetAdditionalNPC", std::numeric_limits<double>::max());  // only available in updated BTPS with new API functions
			if (widgetPos == std::numeric_limits<double>::max()) {
				logger::warn("\tBTPS API is outdated!");
				api = nullptr;
			}
		});
	} else {
		logger::info("Unable to acquire BTPS API");
	}
}

RE::NiPoint3 BetterThirdPersonSelection::GetBTPSWidgetPos()
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	if (api && api->GetWidget3DEnabled()) {
		api->GetSelectionWidgetPos3D(x, y, z);
	}
	return RE::NiPoint3{ (float)x, (float)y, (float)z };
}
