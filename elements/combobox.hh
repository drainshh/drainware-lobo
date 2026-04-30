#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_internal.h"
#include <string>

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

static float EaseOutCubic( float t )
{
	t = ImClamp( t, 0.0f, 1.0f );
	return 1.0f - ImPow( 1.0f - t, 3.0f );
}

bool header_box( const char* name, const char* preview_value, int items_count, ImVec2 sz )
{
	ImGuiContext& g = *GImGui;

	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	const auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const float width{ sz.x };
	const float height{ sz.y };
	const static float rounding{ 3.0f };
	const static float thickness{ 1.0f };
	const static float padding{ 5.0f };
	const static float spacing{ 5.0f };

	const ImRect frame_bb( ImGui::GetCurrentWindow( )->DC.CursorPos, ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( width, height ) );
	const ImRect total_bb( ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( 0.0f, name_text_size.y + 5.0f ), frame_bb.Max );

	ImGui::ItemSize( frame_bb, height );

	if ( !ImGui::ItemAdd( frame_bb, ImGui::GetID( name ) ) )
		return false;

	bool hovered, held;
	bool pressed    = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );
	bool popup_open = ImGui::IsPopupOpen( ImGui::GetID( name ), ImGuiPopupFlags_::ImGuiPopupFlags_None );

	ImGui::GetWindowDrawList( )->PushClipRect( total_bb.Min, total_bb.Max );
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ), rounding,
	                                            ImDrawFlags_::ImDrawFlags_RoundCornersAll );

	if ( popup_open )
		ImGui::GetWindowDrawList( )->AddRect(
			total_bb.Min, total_bb.Max,
			ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) ), rounding,
			ImDrawFlags_::ImDrawFlags_RoundCornersAll, thickness );

	ImGui::GetWindowDrawList( )->AddText( total_bb.Min + ImVec2( 5.0f, 2.0f ), ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
	                                      preview_value );
	ImGui::GetWindowDrawList( )->PopClipRect( );
	ImGui::GetWindowDrawList( )->AddText( frame_bb.Min, ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ), name );

	if ( ( pressed || g.NavActivateId == ImGui::GetID( name ) ) && !popup_open ) {
		if ( ImGui::GetCurrentWindow( )->DC.NavLayerCurrent == 0 )
			ImGui::GetCurrentWindow( )->NavLastIds[ 0 ] = ImGui::GetID( name );

		ImGui::OpenPopupEx( ImGui::GetID( name ), ImGuiPopupFlags_::ImGuiPopupFlags_None );
		popup_open = true;
	}

	// Если popup не открыт, выходим, больше ничего не делая.
	if ( !popup_open )
		return false;

	const float full_popup_height = CalcMaxPopupHeightFromItemCount( items_count );
	const float max_popup_height = CalcMaxPopupHeightFromItemCount( ImMin( items_count, 7 ) );
	const float constrained_height = ImMin( full_popup_height, max_popup_height );
	const ImGuiID anim_id = ImGui::GetID( name ) ^ 0x5A17C0DE;
	float anim_progress = ImGui::GetStateStorage( )->GetFloat( anim_id, 0.0f );
	anim_progress = ImMin( anim_progress + ImGui::GetIO( ).DeltaTime * 10.5f, 1.0f );
	ImGui::GetStateStorage( )->SetFloat( anim_id, anim_progress );
	const float animated_height = ImMax( 2.0f, constrained_height * EaseOutCubic( anim_progress ) );

	ImVec2 popup_pos = total_bb.Min + ImVec2( 0, ( height / 2.0f ) + spacing );
	const float display_bottom = ImGui::GetIO( ).DisplaySize.y - 8.0f;
	if ( popup_pos.y + constrained_height > display_bottom )
		popup_pos.y = ImMax( 8.0f, total_bb.Min.y - constrained_height - spacing );

	ImGui::SetNextWindowPos( popup_pos );
	ImGui::SetNextWindowSize( ImVec2( width, animated_height ), ImGuiCond_Always );

	// Флаги окна popup-а.
	ImGuiWindowFlags flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar |
	                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
	                         ImGuiWindowFlags_NoMove;

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( padding, padding ) );

	bool call_once = ImGui::BeginPopup( name, flags );

	if ( call_once ) {
		ImGui::PushClipRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ), false );
		ImGui::GetWindowDrawList( )->AddRectFilled( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                            ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ), rounding,
		                                            ImDrawFlags_RoundCornersAll );
		ImGui::GetWindowDrawList( )->AddRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                      ImColor( 55, 55, 55, int( 150 * g_menu.m_animation_progress ) ), rounding );
		ImGui::PopClipRect( );
	} else {
		ImGui::EndPopup( );
		return false;
	}

	ImGui::PopStyleVar( );

	return true;
}


void combo(const char* label, int& selected_index, const char* items[], int items_count,ImVec2 sz = ImVec2(180,40))
{
	if ( items_count <= 0 )
		return;
	selected_index = ImClamp( selected_index, 0, items_count - 1 );
	// later do a custom text selector
	if ( header_box( label, items[ selected_index ], items_count, sz ) ) {
		for (int i = 0; i < items_count; i++) {
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       ( selected_index == i )
			                           ? ImVec4( GET_VARIABLE( g_variables.m_accent, c_color ).get_vec4( float( 1.f * g_menu.m_animation_progress ) ) )
								   : ImVec4( 255, 255, 255, float( 1.f * g_menu.m_animation_progress ) ) );
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));

			const ImGuiID item_id = ImGui::GetID( items[ i ] );
			float hover_anim = ImGui::GetStateStorage( )->GetFloat( item_id, 0.0f );
			const bool hovered = ImGui::IsMouseHoveringRect( ImGui::GetCursorScreenPos( ), ImGui::GetCursorScreenPos( ) + ImVec2( sz.x - 10.0f, ImGui::GetTextLineHeightWithSpacing( ) ) );
			hover_anim = ImClamp( hover_anim + ( hovered ? 1.0f : -1.0f ) * ImGui::GetIO( ).DeltaTime * 9.0f, 0.0f, 1.0f );
			ImGui::GetStateStorage( )->SetFloat( item_id, hover_anim );
			if ( hover_anim > 0.01f )
				ImGui::GetWindowDrawList( )->AddRectFilled( ImGui::GetCursorScreenPos( ), ImGui::GetCursorScreenPos( ) + ImVec2( sz.x - 10.0f, ImGui::GetTextLineHeightWithSpacing( ) ),
				                                            ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( hover_anim * 0.14f ) ), 3.0f );

			if (ImGui::Selectable(items[i], (selected_index == i)))
				selected_index = i;  

			ImGui::PopStyleColor(4);
		}
		ImGui::EndPopup();
	}
}

void multi_combo( const char* label, std::vector< bool >& combos, const char* items[], int items_count, ImVec2 sz = ImVec2( 180, 40 ) )
{
	if ( items_count <= 0 )
		return;
	if ( combos.size( ) < static_cast< std::size_t >( items_count ) )
		combos.resize( items_count );
	std::vector< std::string > selected;

	for ( int i = 0; i < items_count; i++ )
		if ( combos[ i ] )
			selected.push_back( items[ i ] );

	std::string preview = selected.empty( ) ? "none" : selected[ 0 ];

	for ( int i = 1; i < selected.size( ); i++ )
		i >= 2 ? preview = "..." : preview += ", " + selected[ i ];

	// later do a custom text selector
	if ( header_box( label, preview.c_str( ), items_count, sz ) ) {
		for ( int i = 0; i < items_count; i++ ) {
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       combos[ i ]
			                           ? ImVec4( GET_VARIABLE( g_variables.m_accent, c_color ).get_vec4( float( 255 * g_menu.m_animation_progress ) ) )
			                                                  : ImVec4( 255, 255, 255, float( 1.f * g_menu.m_animation_progress ) ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0, 0, 0, 0 ) );

			// Use a temporary variable to handle the proxy object issue
			bool temp = combos[ i ];
			if ( ImGui::Selectable( items[ i ], &temp, ImGuiSelectableFlags_DontClosePopups ) )
				combos[ i ] = temp;

			ImGui::PopStyleColor( 4 );
		}
		ImGui::EndPopup( );
	}
}
void multi_combo( const char* label, bool combos[], const char* items[], int items_count, ImVec2 sz = ImVec2( 180, 40 ) )
{
	if ( items_count <= 0 )
		return;
	std::vector< std::string > selected;

	for ( int i = 0; i < items_count; i++ )
		if ( combos[ i ] )
			selected.push_back( items[ i ] );

	std::string preview = selected.empty( ) ? "none" : selected[ 0 ];

	for ( int i = 1; i < selected.size( ); i++ )
		i >= 2 ? preview = "..." : preview += ", " + selected[ i ];

	// later do a custom text selector
	if ( header_box( label, preview.c_str( ), items_count, sz ) ) {
		for ( int i = 0; i < items_count; i++ ) {
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       combos[ i ] ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, 255 / 255.f ) : ImVec4( 255, 255, 255, 255 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0, 0, 0, 0 ) );

			// Use a temporary variable to handle the proxy object issue
			bool temp = combos[ i ];
			if ( ImGui::Selectable( items[ i ], &temp, ImGuiSelectableFlags_DontClosePopups ) )
				combos[ i ] = temp;

			ImGui::PopStyleColor( 4 );
		}
		ImGui::EndPopup( );
	}
}
