#pragma once

class Settings
{
public:
	using INIFunc = std::function<void(CSimpleIniA&)>;

	static Settings* GetSingleton()
	{
		return &instance;
	};

	void SerializeStyles(INIFunc a_func) const;
	void SerializeFonts(INIFunc a_func) const;
	void SerializeMCM(INIFunc a_func) const;
	void SerializeBTPS(INIFunc a_func) const;
	void SerializeDisplayTweaks(INIFunc a_func) const;

private:
	static void SerializeINI(const wchar_t* a_path, INIFunc a_func, bool a_generate = false);
	static void SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, INIFunc a_func);

	// members
	const wchar_t* fontsPath{ L"Data/Interface/FloatingSubtitles/fonts.ini" };
	const wchar_t* stylesPath{ L"Data/Interface/FloatingSubtitles/styles.ini" };

	const wchar_t* defaultDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks.ini" };
	const wchar_t* userDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks_Custom.ini" };

	const wchar_t* defaultMCMPath{ L"Data/MCM/Config/FloatingSubtitles/settings.ini" };
	const wchar_t* userMCMPath{ L"Data/MCM/Settings/FloatingSubtitles.ini" };

	const wchar_t* defaultBTPSPath{ L"Data/MCM/Config/BetterThirdPersonSelection/settings.ini" };
	const wchar_t* userBTPSPath{ L"Data/MCM/Settings/BetterThirdPersonSelection.ini" };

	static Settings instance;
};

inline constinit Settings Settings::instance;
