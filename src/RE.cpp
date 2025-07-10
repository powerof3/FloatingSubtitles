#include "RE.h"

namespace RE
{
	ILStringTable::ILStringTable(const std::vector<std::byte>& a_buffer)
	{
		std::uint32_t bufferPosition = 0;

		read_uint32(entryCount, a_buffer, bufferPosition);
		read_uint32(dataSize, a_buffer, bufferPosition);

		directory.reserve(entryCount);
		for (uint32_t i = 0; i < entryCount; ++i) {
			DirectoryEntry entry;
			read_uint32(entry.stringID, a_buffer, bufferPosition);
			read_uint32(entry.offset, a_buffer, bufferPosition);
			directory.emplace_back(entry);
		}

		rawData.assign(a_buffer.data() + bufferPosition, a_buffer.data() + bufferPosition + dataSize);
	}

	std::string ILStringTable::GetStringAtOffset(std::uint32_t offset) const
	{
		std::uint32_t length;
		read_uint32(length, rawData, offset);

		const char* strData = reinterpret_cast<const char*>(rawData.data() + offset);
		return std::string(strData, length ? length - 1 : 0);
	}

	void ILStringTable::read_uint32(std::uint32_t& val, const std::vector<std::byte>& a_buffer, std::uint32_t& a_bufferPosition)
	{
		std::memcpy(&val, a_buffer.data() + a_bufferPosition, sizeof(std::uint32_t));
		a_bufferPosition += sizeof(std::uint32_t);
	}

	void SubtitleInfoEx::setFlag(Flag a_flag, bool a_set)
	{
		if (a_set) {
			pad1D |= static_cast<std::uint8_t>(a_flag);
		} else {
			pad1D &= ~(static_cast<std::uint8_t>(a_flag));
		}
	}

	bool IsCrosshairRef(const RE::TESObjectREFRPtr& a_ref)
	{
		auto crosshairPick = RE::CrosshairPickData::GetSingleton();
		return crosshairPick && crosshairPick->target.get() == a_ref;
	}

	RE::NiAVObject* GetHeadNode(const RE::TESObjectREFRPtr& a_ref)
	{
		if (auto actor = a_ref->As<RE::Actor>()) {
			if (auto middle = actor->GetMiddleHighProcess()) {
				return middle->headNode;
			}
		}
		return nullptr;
	}

	bool HasLOSToTarget(RE::PlayerCharacter* a_player, RE::TESObjectREFR* a_target, bool& pickPerformed)
	{
		using func_t = decltype(&RE::HasLOSToTarget);
		static REL::Relocation<func_t> func{ RELOCATION_ID(39444, 40520) };
		return func(a_player, a_target, pickPerformed);
	}

	void QueueDialogSubtitles(const char* a_text)
	{
		using func_t = decltype(&RE::QueueDialogSubtitles);
		static REL::Relocation<func_t> func{ RELOCATION_ID(51916, 52854) };
		return func(a_text);
	}

	std::string GetINISettingString(std::string_view a_setting)
	{
		return string::toupper(RE::INISettingCollection::GetSingleton()->GetSetting(a_setting)->GetString());
	}

	bool GetINIPrefsSettingBool(std::string_view a_setting)
	{
		return RE::INIPrefSettingCollection::GetSingleton()->GetSetting(a_setting)->GetBool();
	}

	bool ShowGeneralSubsGame()
	{
		return RE::GetINIPrefsSettingBool("bGeneralSubtitles:Interface");
	}

	bool ShowDialogueSubsGame()
	{
		return RE::GetINIPrefsSettingBool("bDialogueSubtitles:Interface");
	}
}
