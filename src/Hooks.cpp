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
				switch (hudData->type.get()) {
				case RE::HUD_MESSAGE_TYPE::kShowSubtitle:
					{
						if (Manager::GetSingleton()->HandlesGeneralSubtitles()) {
							return RE::UI_MESSAGE_RESULTS::kIgnore;
						}
					}
					break;
				case RE::HUD_MESSAGE_TYPE::kSetMode:
					{
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
					break;
				default:
					break;
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
					if (dialogueData->fixedStr == RE::InterfaceStrings::GetSingleton()->showText && Manager::GetSingleton()->HandlesDialogueSubtitles()) {
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
