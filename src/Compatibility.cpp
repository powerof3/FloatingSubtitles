#include "Compatibility.h"

#include "RE.h"
#include "SettingLoader.h"

float ModAPIHandler::DisplayTweaks::GetResolutionScale() const
{
	return borderlessUpscale ?
	           resolutionScale :
	           RE::BSGraphics::Renderer::GetScreenSize().height / 1080.0f;
}

void ModAPIHandler::DisplayTweaks::LoadSettings()
{
	SettingLoader::GetSingleton()->Load(FileType::kDisplayTweaks, [this](auto& ini) {
		resolutionScale = static_cast<float>(ini.GetDoubleValue("Render", "ResolutionScale", resolutionScale));
		borderlessUpscale = static_cast<float>(ini.GetBoolValue("Render", "BorderlessUpscale", borderlessUpscale));
	});
}

void ModAPIHandler::BTPS::GetAPI()
{
	api = BTPS_API_decl::RequestPluginAPI_V0();
	if (api) {
		logger::info("Retrieving BTPS API...");
		SettingLoader::GetSingleton()->Load(FileType::kBTPS, [this](auto& ini) {
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

void ModAPIHandler::BTPS::GetWidgetPos(const RE::TESObjectREFRPtr& a_ref, std::optional<float>& a_posZOut) const
{
	if (a_posZOut.has_value()) {
		return;  // already set
	}

	if (api && api->GetWidget3DEnabled() && RE::IsCrosshairRef(a_ref)) {
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		api->GetSelectionWidgetPos3D(x, y, z);
		a_posZOut = static_cast<float>(z);
	}
}

void ModAPIHandler::TrueHUD::GetAPI()
{
	logger::info("Retrieving TrueHUD API...");
	api = reinterpret_cast<TRUEHUD_API::IVTrueHUD4*>(TRUEHUD_API::RequestPluginAPI());
	if (api) {
		logger::info("\tTrueHUD API is up to date!");
		SettingLoader::GetSingleton()->Load(FileType::kTrueHUD, [this](auto& ini) {
			infoBarAnchor = static_cast<WidgetAnchor>(ini.GetLongValue("ActorInfoBars", "uInfoBarAnchor", std::to_underlying(WidgetAnchor::kHead)));
			infoBarOffsetZ = static_cast<float>(ini.GetDoubleValue("ActorInfoBars", "fInfoBarOffsetZ", 30.0));
		});
	} else {
		logger::info("Unable to acquire TrueHUD API");
	}
}

void ModAPIHandler::TrueHUD::GetWidgetPos(const RE::TESObjectREFRPtr& a_ref, std::optional<float>& a_posZOut) const
{
	if (auto actor = a_ref->As<RE::Actor>()) {
		if (api && api->HasInfoBar(actor->CreateRefHandle(), true)) {
			switch (infoBarAnchor) {
			case WidgetAnchor::kHead:
				{
					auto pos = actor->GetLookingAtLocation();
					pos.z += infoBarOffsetZ;
					a_posZOut = pos.z;
				}
				break;
			case WidgetAnchor::kBody:
				{
					if (const auto torso = RE::GetTorsoNode(actor)) {
						auto pos = torso->world.translate;
						pos.z += infoBarOffsetZ;
						a_posZOut = pos.z;
					}
				}
				break;
			default:
				std::unreachable();
			}
		}
	}
}

void ModAPIHandler::NND::GetAPI()
{
	logger::info("Retrieving NPCs Names Distributor API...");
	api = reinterpret_cast<NND_API::IVNND2*>(NND_API::RequestPluginAPI());
	if (api) {
		logger::info("\tNPCs Names Distributor API is up to date!");
	} else {
		logger::info("\tUnable to acquire NPCs Names Distributor API");
	}
}

std::string ModAPIHandler::NND::GetReferenceName(const RE::TESObjectREFRPtr& a_ref) const
{
	if (api) {
		if (auto actor = a_ref->As<RE::Actor>()) {
			if (auto name = api->GetName(actor, NND_API::NameContext::kSubtitles); !name.empty()) {
				return name.data();
			}
		}
	}

	return a_ref->GetDisplayFullName();
}

void ModAPIHandler::LoadModSettings()
{
	displayTweaks.LoadSettings();
}

void ModAPIHandler::LoadAPIs()
{
	btps.GetAPI();
	trueHUD.GetAPI();
}

std::optional<float> ModAPIHandler::GetWidgetPosZ(const RE::TESObjectREFRPtr& a_ref, bool a_BTPS, bool a_trueHUD) const
{
	std::optional<float> posZ;
	if (a_trueHUD) {
		trueHUD.GetWidgetPos(a_ref, posZ);
	}
	if (a_BTPS) {
		btps.GetWidgetPos(a_ref, posZ);
	}
	return posZ;
}

float ModAPIHandler::GetResolutionScale() const
{
	return displayTweaks.GetResolutionScale();
}

std::string ModAPIHandler::GetReferenceName(const RE::TESObjectREFRPtr& a_ref) const
{
	return nnd.GetReferenceName(a_ref);
}
