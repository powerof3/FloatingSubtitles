#include "Compatibility.h"

#include "Settings.h"

namespace Compatibility
{

	float DisplayTweaks::GetResolutionScale()
	{
		return borderlessUpscale ?
		           resolutionScale :
		           RE::BSGraphics::Renderer::GetScreenSize().height / 1080.0f;
	}

	void DisplayTweaks::LoadSettings()
	{
		Settings::GetSingleton()->SerializeDisplayTweaks([](auto& ini) {
			resolutionScale = static_cast<float>(ini.GetDoubleValue("Render", "ResolutionScale", resolutionScale));
			borderlessUpscale = static_cast<float>(ini.GetBoolValue("Render", "BorderlessUpscale", borderlessUpscale));
		});
	}

	void BTPS::GetAPI()
	{
		api = BTPS_API_decl::RequestPluginAPI_V0();
		if (api) {
			logger::info("Retrieving BTPS API...");
			Settings::GetSingleton()->SerializeBTPS([](auto& ini) {
				if (api == nullptr || validAPI) {
					return;
				}
				auto widgetPos = ini.GetDoubleValue("Widgets", "fWidgetZOffsetAdditionalNPC", std::numeric_limits<double>::max());  // only available in updated BTPS with new API functions
				if (widgetPos == std::numeric_limits<double>::max()) {
					logger::warn("\tBTPS API is outdated!");
					api = nullptr;
				} else {
					logger::info("\tBTPS API is up to date!");
					validAPI = true;
				}
			});
		} else {
			logger::info("Unable to acquire BTPS API");
		}
	}

	RE::NiPoint3 BTPS::GetWidgetPos()
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		if (api && api->GetWidget3DEnabled()) {
			api->GetSelectionWidgetPos3D(x, y, z);
		}
		return RE::NiPoint3{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
	}
}
