#include "Papyrus.h"

#include "Manager.h"

namespace Papyrus
{
	void OnConfigClose(RE::TESQuest*)
	{
		Manager::GetSingleton()->LoadMCMSettings();
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		if (!a_vm) {
			return false;
		}

		a_vm->RegisterFunction("OnConfigClose", MCM, OnConfigClose);

		logger::info("Registered {} class", MCM);

		return true;
	}
}
