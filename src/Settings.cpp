#include "Settings.h"

#include "Compatibility.h"
#include "ImGui/Renderer.h"
#include "Subtitles.h"

void Settings::SerializeINI(const wchar_t* a_path, const INIFunc a_func, bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(a_path); !a_generate && rc < SI_OK) {
		return;
	}

	a_func(ini);

	(void)ini.SaveFile(a_path);
}

void Settings::LoadSettings() const
{
	SerializeINI(defaultDisplayTweaksPath, userDisplayTweaksPath, [](auto& ini) {
		DisplayTweaks::LoadSettings(ini);
	});
}

void Settings::LoadMCMSettings() const
{
	SerializeINI(defaultMCMPath, userMCMPath, [](auto& ini) {
		Subtitles::Manager::GetSingleton()->LoadMCMSettings(ini);
	});

	Subtitles::Manager::GetSingleton()->PostMCMSettingsLoad();
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
