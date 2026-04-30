#pragma once

#include "combobox.hh"
#include "../instalisation fonts/fonts2.h"
#include <algorithm>

void checkbox(
	const char* name, bool* variable, bool disabled = false, bool popup = false, const ImVec2& popup_size = ImVec2( ),
	const std::function< void( ) >& popup_function = [] { } )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return;

	const auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const float width = ImMax( 120.0f, ImGui::GetContentRegionAvail( ).x - 4.0f );
	const static float size{ 14.0f };
	const static float spacing{ 5.0f };
	const static float rounding{ 2.0f };
	const static float fix_shit{ 5.0f };

	ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;

	ImRect total_bb( position, ImVec2( position.x + width, position.y + size ) );
	ImRect check_bb( total_bb.Min, ImVec2( total_bb.Min.x + size, total_bb.Min.y + size ) );

	ImGui::ItemSize( total_bb, size + 8.f );

	if ( !ImGui::ItemAdd( total_bb , ImGui::GetID( name ) ) )
		return;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	if ( pressed && !disabled )
		*variable = !( *variable );

	// Animation logic
	ImGuiID id               = ImGui::GetID( name );
	ImGuiStorage* storage    = ImGui::GetStateStorage( );
	float animation_progress = storage->GetFloat( id, *variable ? 1.0f : 0.0f );
	float target             = *variable ? 1.0f : 0.0f;
	float step               = ImClamp( 8.5f * ImGui::GetIO( ).DeltaTime, 0.0f, 1.0f );
	if ( animation_progress < target )
		animation_progress = std::min( animation_progress + step, target );
	else if ( animation_progress > target )
		animation_progress = std::max( animation_progress - step, target );
	storage->SetFloat( id, animation_progress );

	// Draw checkbox with animation
	ImGui::PushClipRect( check_bb.Min, check_bb.Max, false );
	ImColor unchecked_color = ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) );
	ImColor checked_color   = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) );
	ImColor background_color;
	background_color.Value = ImLerp( unchecked_color.Value, checked_color.Value, animation_progress );
	ImGui::GetWindowDrawList( )->AddRectFilled( check_bb.Min, check_bb.Max, background_color, rounding, ImDrawFlags_::ImDrawFlags_RoundCornersAll );

	if ( animation_progress > 0.0f ) {
		float pad               = ImMax( 1.0f, size / 6.0f );
		ImColor checkmark_color = ImColor( 0, 0, 0, ( int )( 255 * animation_progress ) );
		ImGui::RenderCheckMark( ImGui::GetWindowDrawList( ), ImVec2( check_bb.Min.x + pad, check_bb.Min.y + pad ), checkmark_color, size - pad * 2.0f );
	}
	ImGui::PopClipRect( );

	// Draw label
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::regular, fonts_for_gui::regular->FontSize,
	                                      ImVec2( check_bb.Max.x + spacing, check_bb.Min.y + ( size - name_text_size.y - fix_shit ) / 2 ),
	                                      disabled ? ImColor( 180, 180, 180, int( 255 * g_menu.m_animation_progress ) )
	                                               : ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
	                                      name );

	// Popup handling (unchanged)
	if ( *variable && hovered && !disabled && popup ) {
		ImGui::PushClipRect( total_bb.Min + ImVec2( 5.0f, size + 6.0f ), total_bb.Min + ImVec2( 5.0f, size + 6.0f ) + ImVec2( 140, 20 ), true );
		ImGui::GetForegroundDrawList( )->AddRectFilled( total_bb.Min + ImVec2( 5.0f, size + 6.0f ),
		                                                total_bb.Min + ImVec2( 5.0f, size + 6.0f ) + ImVec2( 140, 20 ),
		                                                ImColor( 7, 7, 7, int( 255 * g_menu.m_animation_progress ) ),
		                                                rounding );
		ImGui::GetForegroundDrawList( )->AddRect( total_bb.Min + ImVec2( 5.0f, size + 6.0f ),
		                                          total_bb.Min + ImVec2( 5.0f, size + 6.0f ) + ImVec2( 140, 20 ),
		                                          ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ),
		                                          rounding );
		ImGui::GetForegroundDrawList( )->AddText( fonts_for_gui::regular, fonts_for_gui::regular->FontSize,
		                                          total_bb.Min + ImVec2( 10.0f, size + 7.0f ),
		                                          ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
		                                          "right click to customize" );
		ImGui::PopClipRect( );

		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
			ImGui::OpenPopup( name );
	}

	ImGui::SetNextWindowSize( popup_size );
	if ( ImGui::BeginPopup( name, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground ) ) {
		ImVec2 pos = ImGui::GetWindowPos( ), size = ImGui::GetWindowSize( );
		ImGui::PushClipRect( pos, pos + size, false );
		ImGui::GetWindowDrawList( )->AddRectFilled( pos, pos + size, ImColor( 7, 7, 7, int( 255 * g_menu.m_animation_progress ) ), rounding );
		ImGui::GetWindowDrawList( )->AddRect( pos, pos + size, ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ), rounding );
		ImGui::PopClipRect( );

		ImGui::BeginGroup( );
		popup_function( );
		ImGui::EndGroup( );

		ImGui::EndPopup( );
	}
}
inline const char* key_names[] = {
	"-",   "m1",  "m2",  "cn",  "m3",  "m4",  "m5",  "-",   "bac", "tab", "-",   "-",   "clr", "ret", "-",   "-",
	"shi", "ctl", "men", "pau", "cap", "kan", "-",   "jun", "fin", "kan", "-",   "esc", "con", "nco", "acc", "mad",
	"spa", "pgu", "pgd", "end", "hom", "lef", "up",  "rig", "dow", "sel", "pri", "exe", "pri", "ins", "del", "hel",
	"0",   "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",   "-",   "-",   "-",   "-",   "-",   "-",
	"-",   "a",   "b",   "c",   "d",   "e",   "f",   "g",   "h",   "i",   "j",   "k",   "l",   "m",   "n",   "o",
	"p",   "q",   "r",   "s",   "t",   "u",   "v",   "w",   "x",   "y",   "z",   "win", "win", "app", "-",   "sle",
	"num", "num", "num", "num", "num", "num", "num", "num", "num", "num", "mul", "add", "sep", "min", "dec", "div",
	"f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",  "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15", "f16",
	"f17", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "-",   "-",   "-",   "-",   "-",   "-",   "-",   "-",
	"num", "scr", "equ", "mas", "toy", "oya", "oya", "-",   "-",   "-",   "-",   "-",   "-",   "-",   "-",   "-",
	"shi", "shi", "ctr", "ctr", "alt", "alt"
};

enum e_key_styles {
	always_on = 0,
	hold,
	toggle
};



bool keybind( const char* name, key_bind_t* key, bool set_key_mode = true )
{
	if (ImGui::GetCurrentWindow()->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;

	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	char buf_display[64] = ("none");

	if (key->m_key != 0 && g.ActiveId != ImGui::GetCurrentWindow()->GetID(name))
		strcpy_s(buf_display, key_names[key->m_key]);

	const static auto name_text_size = ImGui::CalcTextSize(key_names[key->m_key], NULL, true);
	const auto buffer_text_size = ImGui::CalcTextSize(buf_display, NULL, true);
	const static float width{ 40.0f };
	const static float height{ 20.0f };
	const static float spacing{ 5.0f };
	const static float fix_shit{ 5.0f };
	const static float rounding{ 2.0f };

	ImGui::SameLine(ImGui::GetCurrentWindow()->Size.x - (width + (height - spacing)));

	const ImRect frame_bb(ImGui::GetCurrentWindow()->DC.CursorPos, ImGui::GetCurrentWindow()->DC.CursorPos + ImVec2(width, height));

	ImGui::ItemSize(frame_bb, height);

	if (!ImGui::ItemAdd(frame_bb, ImGui::GetCurrentWindow()->GetID(name)))
		return false;

	bool value_changed = false;
	int k = key->m_key;

	const bool hovered = ImGui::IsItemHovered();

	if (hovered && io.MouseClicked[0]) {
		if (g.ActiveId != ImGui::GetCurrentWindow()->GetID(name)) {
			memset(io.MouseDown, 0, sizeof(io.MouseDown));
			memset(io.KeysDown, 0, sizeof(io.KeysDown));
			key->m_key = 0;
		}
		ImGui::SetActiveID(ImGui::GetCurrentWindow()->GetID(name), ImGui::GetCurrentWindow());
		ImGui::FocusWindow(ImGui::GetCurrentWindow());
	}
	else if (io.MouseClicked[0]) {
		if (g.ActiveId == ImGui::GetCurrentWindow()->GetID(name))
			ImGui::ClearActiveID();
	}
	else if (hovered && io.MouseClicked[1] && set_key_mode)
		ImGui::OpenPopup(name);

	if (g.ActiveId == ImGui::GetCurrentWindow()->GetID(name)) {
		for (auto i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) {
			if (io.MouseDown[i]) {
				switch (i) {
				case 0:
					k = 0x01;
					break;
				case 1:
					k = 0x02;
					break;
				case 2:
					k = 0x04;
					break;
				case 3:
					k = 0x05;
					break;
				case 4:
					k = 0x06;
					break;
				}

				value_changed = true;
				ImGui::ClearActiveID();
			}
		}

		if (!value_changed) {
			for (auto i = 0x08; i <= 0xA5; i++) {
				if (io.KeysDown[i]) {
					k = i;
					value_changed = true;
					ImGui::ClearActiveID();
				}
			}
		}

		if (ImGui::IsKeyPressedMap(ImGuiKey_Escape)) {
			key->m_key = 0;
			ImGui::ClearActiveID();
		}
		else
			key->m_key = k;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, ImVec2(spacing, spacing));

	ImGui::SetNextWindowSize(ImVec2(70.f, CalcMaxPopupHeightFromItemCount(sizeof(e_key_styles) - 1) + spacing), ImGuiCond_Always);
	ImGui::SetNextWindowPos(frame_bb.Min + ImVec2(0.0f, height + spacing), ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_Popup
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoMove;

	if (ImGui::BeginPopup(name, flags)) {
		ImGui::PushClipRect(ImGui::GetCurrentWindow()->Pos, ImGui::GetCurrentWindow()->Pos + ImGui::GetWindowSize(), false);
		ImGui::GetCurrentWindow( )->DrawList->AddRectFilled( ImGui::GetCurrentWindow( )->Pos,
		                                                     ImGui::GetCurrentWindow( )->Pos + ImGui::GetWindowSize( ),
		                                                     ImColor( 7, 7, 7, int( 255 * g_menu.m_animation_progress ) ), rounding );
		ImGui::GetCurrentWindow( )->DrawList->AddRect( ImGui::GetCurrentWindow( )->Pos, ImGui::GetCurrentWindow( )->Pos + ImGui::GetWindowSize( ),
		                                               ImColor( 50, 50, 50, int( 255 * g_menu.m_animation_progress ) ), rounding );
		ImGui::PopClipRect();

		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor( ImGuiCol_Text, key->m_key_style == always_on
		                                          ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, float( 1.f * g_menu.m_animation_progress ) )
		                                          : ImVec4( 1.0f, 1.0f, 1.0f, float( 1.f * g_menu.m_animation_progress ) ) );
        if (ImGui::Selectable("always on", key->m_key_style == always_on))  
           key->m_key_style = always_on;  
		ImGui::PopStyleColor();

		ImGui::PushStyleColor( ImGuiCol_Text, key->m_key_style == hold
		                                          ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, float( 1.f * g_menu.m_animation_progress ) )
		                                                               : ImVec4( 1.0f, 1.0f, 1.0f, float( 1.f * g_menu.m_animation_progress ) ) );
        if (ImGui::Selectable("hold", key->m_key_style == hold))  
           key->m_key_style = hold;  
        ImGui::PopStyleColor();  

		ImGui::PushStyleColor( ImGuiCol_Text, key->m_key_style == toggle
		                                          ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, float( 1.f * g_menu.m_animation_progress ) )
		                                          : ImVec4( 1.0f, 1.0f, 1.0f, float( 1.f * g_menu.m_animation_progress ) ) );
        if (ImGui::Selectable("toggle", key->m_key_style == toggle))  
           key->m_key_style = toggle;  
        ImGui::PopStyleColor();
		ImGui::PopStyleColor(3);

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar();

	ImGui::GetCurrentWindow( )->DrawList->AddRectFilled( frame_bb.Min, frame_bb.Max, ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ),
	                                                     rounding );

    ImVec2 text_pos = frame_bb.Min + (frame_bb.GetSize() - buffer_text_size - ImVec2(0, fix_shit)) * 0.5f;

    ImGui::GetCurrentWindow( )->DrawList->AddText( text_pos,
	                                               g.ActiveId == ImGui::GetCurrentWindow( )->GetID( name )
	                                                   ? ImColor( 230, 210, 255, int( 255 * g_menu.m_animation_progress ) )
	                                               : key->m_key != 0 ? ImColor( 230, 210, 255, int( 255 * g_menu.m_animation_progress ) )
	                                                                 : ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
	                                               buf_display );

	return value_changed;
}
