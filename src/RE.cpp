#include "RE.h"

namespace RE
{
	ILStringTable::ILStringTable(const std::vector<std::byte>& a_buffer)
	{
		std::uint32_t bufferPosition = 0;

		read_uint32(entryCount, a_buffer, bufferPosition);
		read_uint32(dataSize, a_buffer, bufferPosition);

		directory.reserve(entryCount);
		for (std::uint32_t i = 0; i < entryCount; ++i) {
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

	bool IsCrosshairRef(const TESObjectREFRPtr& a_ref)
	{
		auto crosshairPick = CrosshairPickData::GetSingleton();
		return crosshairPick && crosshairPick->target.get() == a_ref;
	}

	NiAVObject* GetHeadNode(const TESObjectREFRPtr& a_ref)
	{
		if (auto actor = a_ref->As<Actor>()) {
			if (auto middle = actor->GetMiddleHighProcess()) {
				return middle->headNode;
			}
		}
		return nullptr;
	}

	NiAVObject* GetTorsoNode(const Actor* a_actor)
	{
		if (auto middle = a_actor->GetMiddleHighProcess()) {
			return middle->headNode;
		}
		return nullptr;
	}

	bool HasLOSToTarget(PlayerCharacter* a_player, TESObjectREFR* a_target, bool& pickPerformed)
	{
		using func_t = decltype(&HasLOSToTarget);
		static REL::Relocation<func_t> func{ RELOCATION_ID(39444, 40520) };
		return func(a_player, a_target, pickPerformed);
	}

	void QueueDialogSubtitles(const char* a_text)
	{
		using func_t = decltype(&QueueDialogSubtitles);
		static REL::Relocation<func_t> func{ RELOCATION_ID(51916, 52854) };
		return func(a_text);
	}

	void SendHUDMenuMessage(HUD_MESSAGE_TYPE a_type, const std::string& a_text, bool a_show)
	{
		if (auto hudData = static_cast<HUDData*>(UIMessageQueue::GetSingleton()->CreateUIMessageData(InterfaceStrings::GetSingleton()->hudData))) {
			hudData->type = a_type;
			hudData->show = a_show;
			hudData->text = a_text;
			UIMessageQueue::GetSingleton()->AddMessage(InterfaceStrings::GetSingleton()->hudMenu, UI_MESSAGE_TYPE::kUpdate, hudData);
		}
	}

	const char* GetSpeakerName(const RE::TESObjectREFRPtr& a_ref)
	{
		if (!a_ref->IsActor() || a_ref->extraList.HasType<ExtraTextDisplayData>()) {
			return a_ref->GetDisplayFullName();
		} else {
			if (auto speakerActor = a_ref->As<Actor>()) {
				if (auto speakerNPC = static_cast<TESNPC*>(speakerActor->GetObjectReference())) {
					const auto& name = speakerNPC->shortName;
					if (name.empty()) {
						return speakerNPC->GetFullName();
					} else {
						return name.c_str();
					}
				}
			}
		}
		return nullptr;
	}
}
