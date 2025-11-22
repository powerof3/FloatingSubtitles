#pragma once

namespace RE
{
	struct StringFileInfo
	{
		struct Entry
		{
			std::uint32_t id;
			std::uint32_t offset;
		};

		Entry*                              entryArray;
		std::uint8_t*                       stringBlock;
		std::uint32_t                       stringBlockSize;
		std::uint32_t                       stringBlockOffset;
		BSTSmartPointer<BSResource::Stream> stream;
		BSSpinLock                          lock;
		BSFixedString                       filePath;
		std::uint32_t                       unk30;
	};

	[[nodiscard]] inline static auto GetILStringMap() -> BSTHashMap<BSFixedString, StringFileInfo*>&
	{
		REL::Relocation<BSTHashMap<BSFixedString, StringFileInfo*>*> map{ RELOCATION_ID(501146, 359462) };
		return *map;
	}

	// https://en.uesp.net/wiki/Tes5Mod:String_Table_File_Format
	struct ILStringTable
	{
		struct DirectoryEntry
		{
			std::uint32_t stringID;  //	String ID
			std::uint32_t offset;    //	Offset (relative to beginning of data) to the string.
		};

		ILStringTable(const std::vector<std::byte>& a_buffer);
		std::string GetStringAtOffset(std::uint32_t offset) const;

		// members
		std::uint32_t               entryCount;  // Number of entries in the string table.
		std::uint32_t               dataSize;    // Size of string data that follows after header and directory.
		std::vector<DirectoryEntry> directory;
		std::vector<std::byte>      rawData;

	private:
		// increments offset
		static void read_uint32(std::uint32_t& val, const std::vector<std::byte>& a_buffer, std::uint32_t& a_bufferPosition);
	};

	class SubtitleInfoEx : public SubtitleInfo
	{
	public:
		enum class Flag : std::uint8_t
		{
			kNone = 0,
			kSkip = 1 << 0,
			kOffscreen = 1 << 1,
			kObscured = 1 << 2,
			kInitialized = 1 << 7,
		};

		std::uint32_t& alphaModifier() { return pad04; }
		std::uint8_t&  flagsRaw() { return pad1D; }
		bool           isFlagSet(Flag a_flag) const { return (pad1D & static_cast<std::uint8_t>(a_flag)) != 0; }

		void setFlag(Flag a_flag, bool a_set);
	};

	bool        IsCrosshairRef(const TESObjectREFRPtr& a_ref);
	NiAVObject* GetHeadNode(const TESObjectREFRPtr& a_ref);
	NiAVObject* GetTorsoNode(const Actor* a_actor);
	bool        HasLOSToTarget(PlayerCharacter* a_player, TESObjectREFR* a_target, bool& pickPerformed);
	void        QueueDialogSubtitles(const char* a_text);
	void        SendHUDMenuMessage(HUD_MESSAGE_TYPE a_type, const std::string& a_text = "", bool a_show = true);
	const char* GetSpeakerName(const RE::TESObjectREFRPtr& a_ref);

	template <class... Args>
	bool DispatchStaticCall(BSFixedString a_class, BSFixedString a_fnName, Args&&... a_args)
	{
		if (auto vm = BSScript::Internal::VirtualMachine::GetSingleton()) {
			BSTSmartPointer<BSScript::IStackCallbackFunctor> callback;
			auto                                             args = MakeFunctionArguments(std::forward<Args>(a_args)...);
			return vm->DispatchStaticCall(a_class, a_fnName, args, callback);
		}
		return false;
	}
}
