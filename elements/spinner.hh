#pragma once
#include "../instalisation fonts/fonts2.h"

bool spinner(const char* name) {
	auto& g = *GImGui;

	const static float width{ 300.0f };
	const static float height{ 300.0f };
	const static float radius{ 30.0f };
	const static float thickness{ 3.0f };
	const static float rounding{ 0.0f };

	const static auto name_text_size = ImGui::CalcTextSize(name, NULL, true);

	const ImRect frame_bb{ ImGui::GetMainViewport()->Pos, ImGui::GetMainViewport()->Pos + ImGui::GetMainViewport()->Size };
	const ImRect total_bb{ ((frame_bb.Min + frame_bb.Max) / 2), frame_bb.Min + ImVec2(width, height) };

	ImGui::ItemSize(frame_bb);

	if (!ImGui::ItemAdd(frame_bb, ImGui::GetID(name)))
		return false;

	ImGui::GetBackgroundDrawList()->AddRectFilled(frame_bb.Min, frame_bb.Max, ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 128 / 255.f), rounding, ImDrawFlags_RoundCornersAll);

	float time = static_cast<float>(g.Time) * 1.0f;

	ImGui::GetBackgroundDrawList()->PathClear();

	float start = abs(ImSin(time) * (32.0f - 5.0f));

	const float a_min = IM_PI * 2.0f * start / 32.0f;
	const float a_max = IM_PI * 2.0f * 29.0f / 32.0f;
	const auto center = total_bb.Min + ImVec2(radius, radius);

	for (float i = 0; i < 32; i++) {
		const float a = a_min + (i / 32.0f ) * (a_max - a_min);
		ImGui::GetBackgroundDrawList()->PathLineTo({ center.x + ImCos(a + time * 8.0f) * radius, center.y + ImSin(a + time * 8.0f) * radius });
	}

	ImGui::GetBackgroundDrawList()->PathStroke(ImColor(255, 255, 255, 255), false, thickness);

	ImGui::GetBackgroundDrawList()->AddText(fonts_for_gui::bolder, fonts_for_gui::bolder->FontSize, { center.x - (name_text_size.x / 2) + 1.0f, center.y - (name_text_size.y / 2) - ((radius * 2.0f) - 1.0f) },
		ImColor(0, 0, 0, 255), name);

	ImGui::GetBackgroundDrawList()->AddText(fonts_for_gui::bolder, fonts_for_gui::bolder->FontSize, { center.x - (name_text_size.x / 2), center.y - (name_text_size.y / 2) - (radius * 2.0f) },
		ImColor(255, 255, 255, 255), name);

	return true;
}