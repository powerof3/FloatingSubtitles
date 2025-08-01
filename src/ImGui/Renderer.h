#pragma once

namespace ImGui::Renderer
{
	void Install();

	// members
	inline std::atomic initialized{ false };
}
