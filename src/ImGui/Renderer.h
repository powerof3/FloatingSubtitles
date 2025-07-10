#pragma once

namespace ImGui::Renderer
{
	void Install();

	void RenderMenus(bool a_render);

	// members
	inline std::atomic initialized{ false };
	inline std::atomic renderMenus{ false };
}
