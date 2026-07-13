#include "Hooks.h"

#include "Manager.h"

namespace Hooks
{
	struct ShowSubtitle
	{
		static void thunk(RE::SubtitleManager* a_manager, RE::TESObjectREFR* ref, const char* subtitle, bool alwaysDisplay)
		{
			func(a_manager, ref, subtitle, alwaysDisplay);

			Manager::GetSingleton()->AddSubtitle(a_manager, subtitle);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct ApplyDistanceCheck
	{
		static void thunk(RE::SubtitleManager* a_manager)
		{
			func(a_manager);

			Manager::GetSingleton()->UpdateSubtitleInfo(a_manager);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct HUDMenu_ProcessMessage
	{
		static RE::UI_MESSAGE_RESULTS thunk(RE::HUDMenu* a_this, RE::UIMessage& a_message)
		{
			if (auto hudData = static_cast<RE::HUDData*>(a_message.data)) {
				// VR shifts HUD_MESSAGE_TYPE by +3 for values >= kShowSubtitle; resolve the
				// runtime value via CommonLibVR instead of comparing against the raw SE constant.
				const auto type = hudData->type.get();
				if (type == RE::GetHUDMessageType(RE::HUD_MESSAGE_TYPE::kShowSubtitle)) {
					// VR's DialogueMenu forwards real dialogue text as a second kShowSubtitle to HUDMenu
					// (SE/AE never do this); don't let general-suppression swallow that forward.
					const bool dialogueForwarding = REL::Module::IsVR() && !Manager::GetSingleton()->HandlesDialogueSubtitles() && RE::MenuTopicManager::GetSingleton()->menuOpen;
					const bool handles = Manager::GetSingleton()->HandlesGeneralSubtitles() && !dialogueForwarding;
					if (Manager::GetSingleton()->GetSettings().debugLog) {
						logger::debug("[HUDMenu] kShowSubtitle: HandlesGeneral={} dialogueForwarding={} -> {}", handles, dialogueForwarding, handles ? "suppress(kIgnore)" : "pass-through");
					}
					if (handles) {
						return RE::UI_MESSAGE_RESULTS::kIgnore;
					}
				} else if (type == RE::GetHUDMessageType(RE::HUD_MESSAGE_TYPE::kSetMode)) {
					static constexpr std::array badModes{
						"TweenMode"sv,
						"InventoryMode"sv,
						"WorldMapMode"sv,
						"BookMode"sv,
						"JournalMode"sv
					};
					if (std::ranges::any_of(badModes, [&](const auto& mode) { return string::iequals(hudData->text, mode); })) {
						Manager::GetSingleton()->SetVisible(!hudData->show);
					}
				}
			}

			return func(a_this, a_message);
		}

		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx = 0x4;
	};

	struct DialogueMenu_ProcessMessage
	{
		static RE::UI_MESSAGE_RESULTS thunk(RE::DialogueMenu* a_this, RE::UIMessage& a_message)
		{
			if (a_message.type == RE::UI_MESSAGE_TYPE::kUpdate) {
				if (auto dialogueData = static_cast<RE::BSUIMessageData*>(a_message.data)) {
					auto        interfaceStrings = RE::InterfaceStrings::GetSingleton();
					const auto& showText = REL::Module::IsVR() ? interfaceStrings->GetVRRuntimeData().showText : interfaceStrings->GetRuntimeData().showText;
					const bool  match = dialogueData->fixedStr == showText;
					const bool  handles = Manager::GetSingleton()->HandlesDialogueSubtitles();
					if (Manager::GetSingleton()->GetSettings().debugLog) {
						logger::debug("[DialogueMenu] kUpdate fixedStr='{}' showText='{}' match={} handlesDialogue={}",
							dialogueData->fixedStr.c_str(), showText.c_str(), match, handles);
					}
					if (match && handles) {
						return RE::UI_MESSAGE_RESULTS::kIgnore;
					}
				}
			}

			return func(a_this, a_message);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx = 0x4;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> showSubtitle(RELOCATION_ID(51753, 52626));
		stl::hook_function_prologue<ShowSubtitle, 5>(showSubtitle.address());

		REL::Relocation<std::uintptr_t> subtitleUpdate(RELOCATION_ID(51756, 52629), OFFSET(0x41, 0x37));
		stl::write_thunk_call<ApplyDistanceCheck>(subtitleUpdate.address());

		stl::write_vfunc<RE::HUDMenu, HUDMenu_ProcessMessage>();
		stl::write_vfunc<RE::DialogueMenu, DialogueMenu_ProcessMessage>();

		logger::info("Installed subtitle hooks");
	}
}
