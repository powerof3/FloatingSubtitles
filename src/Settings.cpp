#include "Settings.h"

void Settings::SerializeINI(const wchar_t* a_path, const INIFunc a_func, bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(a_path); !a_generate && rc < SI_OK) {
		return;
	}

	a_func(ini);

	if (!a_generate) {
		return;
	}

	(void)ini.SaveFile(a_path);
}

void Settings::SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, INIFunc a_func)
{
	SerializeINI(a_defaultPath, a_func);
	SerializeINI(a_userPath, a_func);
}

void Settings::SerializeStyles(INIFunc a_func) const
{
	SerializeINI(stylesPath, a_func, true);
}

void Settings::SerializeFonts(INIFunc a_func) const
{
	SerializeINI(fontsPath, a_func, true);
}

void Settings::SerializeMCM(INIFunc a_func) const
{
	SerializeINI(defaultMCMPath, userMCMPath, a_func);
}

void Settings::SerializeBTPS(INIFunc a_func) const
{
	SerializeINI(defaultBTPSPath, userBTPSPath, a_func);
}

void Settings::SerializeDisplayTweaks(INIFunc a_func) const
{
	SerializeINI(defaultDisplayTweaksPath, userDisplayTweaksPath, a_func);
}
