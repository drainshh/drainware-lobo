#pragma once
#include <functional>
#include "../instalisation fonts/fonts2.h"
#include "../hacks/menu/menu.h"
void child(const char* name, const ImVec2& size, const std::function<void()>& func) {
	/* state */
	const static float title_bar_height{ 30.0f };
	const static float spacing[2]{ 10.0f, 5.0f };
	const static float rounding{ 4.0f };
	const static float outline{ 1.0f };

	ImGui::SetCursorPosY(ImGui::GetCursorPos().y + title_bar_height);

	// ���������� BeginChild ������ Begin
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 10.0f, 8.0f ) );
	ImGui::PushStyleVar( ImGuiStyleVar_ScrollbarSize, 4.0f );
	ImGui::BeginChild(name, ImVec2(size.x, size.y - title_bar_height), 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
	ImGui::PopStyleVar( 2 );

	const auto draw_position = ImVec2(ImGui::GetCurrentWindow()->Pos.x, ImGui::GetCurrentWindow()->Pos.y - title_bar_height);

	/*
	*
	*	drawing
	*
	*/

	/* title bar */
	ImGui::PushClipRect(draw_position, draw_position + ImVec2(size.x, title_bar_height), false);
	ImGui::GetWindowDrawList()->AddRectFilled(draw_position, draw_position + ImVec2(size.x, title_bar_height),
	                                            ImColor( 10, 10, 10, int( 255 * g_menu.m_animation_progress ) ), rounding, ImDrawFlags_RoundCornersTop );
	ImGui::PopClipRect();

	/* background */
	ImGui::PushClipRect(draw_position + ImVec2(0.0f, title_bar_height), draw_position + size, false);
	ImGui::GetWindowDrawList()->AddRectFilled(draw_position + ImVec2(0.0f, title_bar_height), draw_position + size,
	                                            ImColor( 10, 10, 10, int( 255 * g_menu.m_animation_progress ) ), rounding,
	                                            ImDrawFlags_RoundCornersBottom );
	ImGui::PopClipRect();

	/* outline */
	ImGui::PushClipRect(draw_position, draw_position + size, false);
	ImGui::GetWindowDrawList()->AddRect(draw_position, draw_position + size, ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ),
	                                      rounding, ImDrawFlags_RoundCornersAll );
	ImGui::PopClipRect();

	/* separator for title bar */
	ImGui::PushClipRect(draw_position + ImVec2(0.0f, title_bar_height), draw_position + ImVec2(size.x, title_bar_height), false);
	ImGui::GetForegroundDrawList()->AddLine(draw_position + ImVec2(0.0, title_bar_height), draw_position + ImVec2(size.x, title_bar_height),
	                                          ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ), outline );
	ImGui::PopClipRect();

	/* name */
	ImGui::GetForegroundDrawList()->AddText(fonts_for_gui::bolder, fonts_for_gui::bolder->FontSize, draw_position + ImVec2(10, (title_bar_height / 4)),
	                                          ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ), name );

	ImGui::SetCursorPos({ ImGui::GetCursorPosX() + spacing[0] , ImGui::GetCursorPosY() + spacing[1] });
	ImGui::BeginGroup();
	func();
	ImGui::EndGroup();

	ImGui::EndChild();
}
