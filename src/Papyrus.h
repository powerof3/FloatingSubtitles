#pragma once

namespace Papyrus
{
	inline constexpr auto MCM = "FloatingSubtitles_MCM"sv;

	void OnConfigClose(RE::TESQuest*);

	bool Register(RE::BSScript::IVirtualMachine* a_vm);
}
