#include "SettingLoader.h"

void SettingLoader::LoadINI(const wchar_t* a_path, const INIFunc a_func, bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (ini.LoadFile(a_path) >= SI_OK || a_generate) {
		a_func(ini);

		if (a_generate) {
			(void)ini.SaveFile(a_path);
		}
	}
}

void SettingLoader::LoadINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, INIFunc a_func)
{
	LoadINI(a_defaultPath, a_func);
	LoadINI(a_userPath, a_func);
}

void SettingLoader::Load(FileType type, INIFunc a_func, bool a_generate) const
{
	switch (type) {
	case FileType::kFonts:
		LoadINI(fontsPath, a_func, a_generate);
		break;
	case FileType::kStyles:
		LoadINI(stylesPath, a_func, a_generate);
		break;
	case FileType::kMCM:
		LoadINI(defaultMCMPath, userMCMPath, a_func);
		break;
	case FileType::kBTPS:
		LoadINI(defaultBTPSPath, userBTPSPath, a_func);
		break;
	case FileType::kDisplayTweaks:
		LoadINI(defaultDisplayTweaksPath, userDisplayTweaksPath, a_func);
		break;
	case FileType::kTrueHUD:
		LoadINI(defaultTrueHUDPath, userTrueHUDPath, a_func);
		break;
	}
}
