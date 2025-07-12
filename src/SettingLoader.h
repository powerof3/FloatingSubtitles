#pragma once

enum class FileType
{
	kFonts,
	kStyles,
	kMCM,
	kDisplayTweaks,
	kBTPS,
	kTrueHUD
};

class SettingLoader
{
public:
	using INIFunc = std::function<void(CSimpleIniA&)>;

	static SettingLoader* GetSingleton()
	{
		return &instance;
	}

	void Load(FileType type, INIFunc a_func, bool a_generate = false) const;

private:
	static void LoadINI(const wchar_t* a_path, INIFunc a_func, bool a_generate = false);
	static void LoadINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, INIFunc a_func);

	// members
	const wchar_t* fontsPath{ L"Data/Interface/FloatingSubtitles/fonts.ini" };
	const wchar_t* stylesPath{ L"Data/Interface/FloatingSubtitles/styles.ini" };

	const wchar_t* defaultDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks.ini" };
	const wchar_t* userDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks_Custom.ini" };

	const wchar_t* defaultMCMPath{ L"Data/MCM/Config/FloatingSubtitles/settings.ini" };
	const wchar_t* userMCMPath{ L"Data/MCM/Settings/FloatingSubtitles.ini" };

	const wchar_t* defaultBTPSPath{ L"Data/MCM/Config/BetterThirdPersonSelection/settings.ini" };
	const wchar_t* userBTPSPath{ L"Data/MCM/Settings/BetterThirdPersonSelection.ini" };

	const wchar_t* defaultTrueHUDPath{ L"Data/MCM/Config/TrueHUD/settings.ini" };
	const wchar_t* userTrueHUDPath{ L"Data/MCM/Settings/TrueHUD.ini" };

	static SettingLoader instance;
};

inline constinit SettingLoader SettingLoader::instance;
