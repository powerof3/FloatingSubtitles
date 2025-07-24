#include "Util.h"

namespace ImGui
{
	ImU32 GetColorU32(const ImVec4& col, float alpha_mul)
	{
		ImGuiStyle& style = GImGui->Style;
		ImVec4      c = col;
		c.w *= style.Alpha * alpha_mul;
		return ColorConvertFloat4ToU32(c);
	}

	float WorldToScreenLoc(const RE::NiPoint3& worldLocIn, ImVec2& screenLocOut)
	{
		float zVal;
		RE::Main::WorldRootCamera()->WorldPtToScreenPt3(worldLocIn, screenLocOut.x, screenLocOut.y, zVal, 1e-5f);

		const ImVec2 rect = ImGui::GetIO().DisplaySize;
		screenLocOut.x = rect.x * screenLocOut.x;
		screenLocOut.y = rect.y * (1.0f - screenLocOut.y);

		return zVal;
	}

	void DrawCircle(const RE::NiPoint3& a_pos, float radius, ImU32 color)
	{
		ImVec2 screenPos;
		auto   zDepth = WorldToScreenLoc(a_pos, screenPos);
		if (zDepth > 0.0f) {
			auto drawList = ImGui::GetBackgroundDrawList();
			drawList->AddCircle(screenPos, radius, color, 0, 3.0f);
		}
	}

	void DrawLine(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to, ImU32 color)
	{
		ImVec2 screenFrom;
		ImVec2 screenTo;
		WorldToScreenLoc(a_from, screenFrom);
		WorldToScreenLoc(a_to, screenTo);
		auto drawList = ImGui::GetBackgroundDrawList();
		drawList->AddLine(screenFrom, screenTo, color, 3.0f);
	}

	void DrawTextAtPoint(const RE::NiPoint3& a_pos, const char* a_text, ImU32 color)
	{
		ImVec2 screenPos;
		auto   zDepth = WorldToScreenLoc(a_pos, screenPos);
		if (zDepth > 0.0f) {
			ImGui::PushFont(nullptr, 30);
			{
				auto drawList = ImGui::GetBackgroundDrawList();
				drawList->AddCircleFilled(screenPos, 6.0f, color);
				drawList->AddText(ImVec2(screenPos.x + 6.0f + ImGui::GetStyle().ItemSpacing.x, screenPos.y - 3.f), color, a_text);
			}
			ImGui::PopFont();
		}
	}

	void DrawBSBound(const RE::BSBound& bound, const RE::NiPoint3& a_position, ImU32 color)
	{
		std::array<RE::NiPoint3, 8> corners;

		int index = 0;
		for (int dx : { -1, 1 }) {
			for (int dy : { -1, 1 }) {
				for (int dz : { -1, 1 }) {
					corners[index++] = RE::NiPoint3{
						bound.center.x + dx * bound.extents.x,
						bound.center.y + dy * bound.extents.y,
						bound.center.z + dz * bound.extents.z
					} + a_position;
				}
			}
		}

		static const int edgeIdx[12][2] = {
			// pairs of corner indices
			{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },  // bottom rectangle
			{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },  // top rectangle
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }   // vertical legs
		};

		for (auto [i0, i1] : edgeIdx) {
			ImGui::DrawLine(corners[i0], corners[i1], color);
		}
	}

	ImVec2 GetNativeViewportPos()
	{
		return GetMainViewport()->Pos;
	}

	ImVec2 GetNativeViewportSize()
	{
		return GetMainViewport()->Size;
	}

	ImVec2 GetNativeViewportCenter()
	{
		const auto Size = GetNativeViewportSize();
		return { Size.x * 0.5f, Size.y * 0.5f };
	}
}
