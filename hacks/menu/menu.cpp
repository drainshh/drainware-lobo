#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "../../globals/branding/branding.h"
#include "../../globals/fonts/fonts.h"
#include "../../globals/includes/includes.h"
#include "../../globals/logger/logger.h"
#include "../misc/misc.h"
#include "../misc/scaleform/scaleform.h"
#include "../movement/movement.h"
#include "../inventory/inventory_changer.h"
#include "../../dependencies/imgui/imgui.h"
#include "../inventory/items_manager.h"
#include "../elements/tabs.hh"
#include "../elements/childbox.hh"
#include "../elements/checkbox.hh"
#include "../elements/button.hh"
#include "../elements/sliders.hh"
#include "../elements/spinner.hh"
#include "../elements/combobox.hh"
#include "../elements/i_text.hh"
#include "../aimbot/aimbot.h"
#include <cstring>
constexpr int color_picker_alpha_flags = ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf |
                                         ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoInputs |
                                         ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar |
                                         ImGuiColorEditFlags_NoBorder;

constexpr int color_picker_no_alpha_flags = ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_InputRGB |
                                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop |
                                            ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder;

constexpr static const auto chams_materials = "flat\0textured\0metallic\0glow";

void save_popup( const char* str_id, const ImVec2& window_size, const std::function< void( ) >& fn )
{
	ImGuiContext& g         = *GImGui;
	const ImGuiStyle& style = g.Style;

	const auto window = g.CurrentWindow;

	const auto hashed_str_id = ImHashStr( str_id );
	const auto text_size     = g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->CalcTextSizeA(
        g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->FontSize, FLT_MAX, 0.f, str_id );

	const ImColor accent_color = ImGui::GetColorU32( ImGuiCol_::ImGuiCol_Accent );

	ImGui::OpenPopup( str_id );

	ImGui::SetNextWindowSize( window_size );

	if ( ImGui::BeginPopup( str_id ) ) {
		const auto draw_list = ImGui::GetWindowDrawList( );

		const ImVec2 position = ImGui::GetWindowPos( ), size = ImGui::GetWindowSize( );

		auto text_animation = ImAnimationHelper( hashed_str_id, ImGui::GetIO( ).DeltaTime );

		ImGui::PushClipRect( ImVec2( position ), ImVec2( position.x + size.x, position.y + 20.f ), false );

		draw_list->AddRectFilled( ImVec2( position ), ImVec2( position.x + size.x, position.y + 20.f ), ImColor( 25 / 255.f, 25 / 255.f, 25 / 255.f ),
		                          g.Style.WindowRounding - 2.f, ImDrawFlags_RoundCornersTop );

		ImGui::PopClipRect( );

		ImGui::PushClipRect( position, ImVec2( position.x + size.x, position.y + size.y ), false );
		draw_list->AddRect( position, ImVec2( position.x + size.x, position.y + size.y ), ImColor( 50, 50, 50, 100 ), g.Style.WindowRounding - 2.f );

		text_animation.Update( ImGui::IsMouseHoveringRect( position, ImVec2( position.x + size.x, position.y + size.y ) ) ? 3.f : -2.f, 1.f, 0.5f,
		                       1.f );

		ImGui::PopClipRect( );

		const ImColor text_color = ImGui::GetColorU32( ImGuiCol_Text );

		draw_list->AddText(
			g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ], g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->FontSize,
			ImVec2( position.x + ( size.x - text_size.x ) / 2.f, position.y + ( 20.f - text_size.y ) / 2.f ),
			ImColor( text_color.Value.x, text_color.Value.y, text_color.Value.z, text_color.Value.w * text_animation.AnimationData->second ),
			str_id );

		RenderFadedGradientLine( draw_list, ImVec2( position.x, position.y + 20.f ), ImVec2( size.x, 1.f ),
		                         ImColor( accent_color.Value.x, accent_color.Value.y, accent_color.Value.z ) );

		ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) + 20.f );

		fn( );

		ImGui::EndPopup( );
	}
}


extern void loadConfig( const std::string& filename );
extern void saveConfig( const std::string& filename );
extern void loadConfig2( const std::string& filename );
extern void saveConfig2( const std::string& filename );
constexpr std::array skyboxList{ "default",
	                             "baggage",
	                             "tibet",
	                             "embassy",
	                             "italy",
	                             "aztec",
	                             "nukeblank",
	                             "office",
	                             "daylight",
	                             "daylight 2",
	                             "daylight 3",
	                             "daylight 4",
	                             "cloudy",
	                             "night flat",
	                             "night",
	                             "night 2",
	                             "gray",
	                             "clear",
	                             "dusty",
	                             "rural",
	                             "canals",
	                             "vertigo hdr",
	                             "vertigo",
	                             "vertigo blue",
	                             "rainy",
	                             "lunacy",
	                             "aztec hr",
	                             "custom" };
extern bool point_menu_is_opened( );

IDirect3DTexture9* get_menu_logo_texture( )
{
	static IDirect3DTexture9* logo_texture = nullptr;
	static bool tried_logo = false;

	if ( !tried_logo && g_interfaces.m_direct_device ) {
		D3DXCreateTextureFromFileA( g_interfaces.m_direct_device, n_branding::k_logo_file, &logo_texture );
		tried_logo = true;
	}

	return logo_texture;
}



ImColor RarityGetter( int rarity )
{
	if ( rarity == 1 )
		return ImColor( 0.69f, 0.76f, 0.85f, float( 1.f * g_menu.m_animation_progress ) );
	else if ( rarity == 2 )
		return ImColor( 0.29f, 0.41f, 1.f, float( 1.f * g_menu.m_animation_progress ) );
	else if ( rarity == 3 )
		return ImColor( 0.36f, 0.60f, 0.85f, float( 1.f * g_menu.m_animation_progress ) );
	else if ( rarity == 4 )
		return ImColor( 0.53f, 0.27f, 1.f, float( 1.f * g_menu.m_animation_progress ) );
	else if ( rarity == 5 )
		return ImColor( 0.82f, 0.17f, 0.90f, float( 1.f * g_menu.m_animation_progress ) );
	else if ( rarity == 6 )
		return ImColor( 0.92f, 0.29f, 0.29f, float( 1.f * g_menu.m_animation_progress ) );
	else if ( rarity == 7 )
		return ImColor( 0.89f, 0.68f, 0.22f, float( 1.f * g_menu.m_animation_progress ) );
}

std::string ToUtf888( std::wstring_view wstr )
{
	if ( wstr.empty( ) )
		return std::string( );

	auto size_needed = WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int32_t >( wstr.size( ) ), NULL, 0, NULL, NULL );

	std::string out( size_needed, 0 );
	WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int >( wstr.size( ) ), &out[ 0 ], size_needed, NULL, NULL );

	return out;
}

std::string LocalizeTex22( const char* in )
{
	const auto wide_name = g_interfaces.m_localize->find_safe( in );

	if ( !wcslen( wide_name ) )
		return "";

	return ToUtf888( wide_name );
}

bool InventoryItem( InventoryItem_t Item )
{
	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	ImGuiContext& g     = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id        = window->GetID( std::to_string( Item.m_nItemID ).c_str( ) );
	const ImVec2 label_size = ImGui::CalcTextSize( Item.SkinName.c_str( ), NULL, true );
	const ImVec2 pos        = window->DC.CursorPos;
	const ImVec2 size       = ImGui::CalcItemSize( { 140, 142 }, 142, 142 );
	ImRect bb( pos, pos + size );

	ImGui::ItemSize( size, style.FramePadding.y );
	ImGui::ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held, 0 );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, ImColor( 28, 28, 28, int( 255 * g_menu.m_animation_progress ) ), 3 );
	window->DrawList->AddRectFilled( bb.Min + ImVec2( 0, 140 ), bb.Max, RarityGetter( Item.m_iRarity ), 3, ImDrawCornerFlags_Bot );

	bool renderedImage = false;

	if ( renderedImage == false )
		for ( const auto& weapon : g_items_manager.m_vWeapons ) {
			if ( weapon->m_tPaintKit->m_nID == Item.m_iPaintKit && weapon->m_tItem->m_nID == Item.m_nItemID ) {
				window->DrawList->AddImage( weapon->m_tTexture, bb.Min + ImVec2( 17, 37 - 10 ), bb.Min + ImVec2( 125, 105 - 10 ), ImVec2( 0, 0 ),
				                            ImVec2( 1, 1 ), ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ) );
				renderedImage = true;
			}
		}

	if ( renderedImage == false )
		for ( const auto& weapon : g_items_manager.m_vAgents ) {
			if ( weapon->m_nItemID == Item.m_nItemID ) {
				window->DrawList->AddImage( weapon->m_tTexture, bb.Min + ImVec2( 17, 37 - 10 ), bb.Min + ImVec2( 125, 105 - 10 ), ImVec2( 0, 0 ),
				                            ImVec2( 1, 1 ), ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ) );
				renderedImage = true;
			}
		}

	if ( renderedImage == false ) {
		auto time = static_cast< float >( g.Time ) * 1.8f;
		window->DrawList->PathClear( );
		int start           = static_cast< int >( abs( ImSin( time ) * ( 32 - 5 ) ) );
		const float a_min   = IM_PI * 2.0f * ( ( float )start ) / ( float )32;
		const float a_max   = IM_PI * 2.0f * ( ( float )32 - 3 ) / ( float )32;
		const auto&& centre = ImVec2( pos.x + 70, pos.y + 60 );
		for ( auto i = 0; i < 32; i++ ) {
			const float a = a_min + ( ( float )i / ( float )32 ) * ( a_max - a_min );
			window->DrawList->PathLineTo( { centre.x + ImCos( a + time * 8 ) * 20, centre.y + ImSin( a + time * 8 ) * 20 } );
		}
		window->DrawList->PathStroke( ImColor( 190, 190, 190, int( 255 * g_menu.m_animation_progress ) ), false, 4 );
	}

	window->DrawList->PushClipRect( bb.Min, bb.Max, true );
	window->DrawList->AddText( bb.Min + ImVec2( 10, 142 - label_size.y - 10 ), ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
	                           Item.SkinName.c_str( ) );
	window->DrawList->PopClipRect( );

	//window->DrawList->AddRectFilledMultiColor( bb.Min + ImVec2( 110, 142 - label_size.y - 10 ), bb.Max - ImVec2( 0, 6 ), ImColor( 58, 58, 58, 0 ),
	//                                           ImColor( 58, 58, 58, 255 ), ImColor( 58, 58, 58, 255 ), ImColor( 58, 58, 58, 0 ) );

	return pressed;
}

bool InventoryItemDefault( DefaultItem_t* Item )
{
	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	ImGuiContext& g     = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id        = window->GetID( std::to_string( Item->m_nItemID ).c_str( ) );
	const ImVec2 label_size = ImGui::CalcTextSize( LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ), NULL, true );
	const ImVec2 pos        = window->DC.CursorPos;
	const ImVec2 size       = ImGui::CalcItemSize( { 140, 142 }, 142, 142 );
	ImRect bb( pos, pos + size );

	ImGui::ItemSize( size, style.FramePadding.y );
	ImGui::ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held, 0 );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ), 3 );
	window->DrawList->AddRectFilled( bb.Min + ImVec2( 0, 140 ), bb.Max, RarityGetter( 1 ), 3, ImDrawCornerFlags_Bot );
	window->DrawList->AddImage( Item->m_tTexture, bb.Min + ImVec2( 17, 37 - 10 ), bb.Min + ImVec2( 125, 105 - 10 ), ImVec2( 0, 0 ), ImVec2( 1, 1 ),
	                            ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ) ); // 108  68

	ImGui::GetWindowDrawList( )->PushClipRect( bb.Min, bb.Max, true );
	ImGui::GetWindowDrawList( )->AddText( bb.Min + ImVec2( 10, 142 - label_size.y - 10 ),
	                                      ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
	                                      LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ) );
	ImGui::GetWindowDrawList( )->PopClipRect( );

	window->DrawList->AddRectFilledMultiColor( bb.Min + ImVec2( 110, 142 - label_size.y - 10 ), bb.Max - ImVec2( 0, 6 ), ImColor( 58, 58, 58, 0 ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, 0 ) );

	return pressed;
}

bool InventoryItemAgent( AgentsItem_t* Item )
{
	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	ImGuiContext& g     = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id        = window->GetID( std::to_string( Item->m_nItemID ).c_str( ) );
	const ImVec2 label_size = ImGui::CalcTextSize( LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ), NULL, true );
	const ImVec2 pos        = window->DC.CursorPos;
	const ImVec2 size       = ImGui::CalcItemSize( { 140, 142 }, 142, 142 );
	ImRect bb( pos, pos + size );

	ImGui::ItemSize( size, style.FramePadding.y );
	ImGui::ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held, 0 );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ), 3 );
	window->DrawList->AddRectFilled( bb.Min + ImVec2( 0, 140 ), bb.Max, RarityGetter( 1 ), 3, ImDrawCornerFlags_Bot );
	window->DrawList->AddImage( Item->m_tTexture, bb.Min + ImVec2( 17, 37 - 10 ), bb.Min + ImVec2( 125, 105 - 10 ), ImVec2( 0, 0 ), ImVec2( 1, 1 ),
	                            ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ) ); // 108  68

	ImGui::GetWindowDrawList( )->PushClipRect( bb.Min, bb.Max, true );
	ImGui::GetWindowDrawList( )->AddText( bb.Min + ImVec2( 10, 142 - label_size.y - 10 ),
	                                      ImColor( 255, 255, 255,int( 255 * g_menu.m_animation_progress ) ),
	                                      LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ) );
	ImGui::GetWindowDrawList( )->PopClipRect( );

	window->DrawList->AddRectFilledMultiColor( bb.Min + ImVec2( 110, 142 - label_size.y - 10 ), bb.Max - ImVec2( 0, 6 ), ImColor( 58, 58, 58, 0 ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, 0 ) );

	return pressed;
}


bool InventoryItemGlove( GloveItem_t* Item )
{
	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	ImGuiContext& g     = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id        = window->GetID( std::to_string( Item->m_nKitID ).c_str( ) );
	const ImVec2 label_size = ImGui::CalcTextSize( LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ), NULL, true );
	const ImVec2 pos        = window->DC.CursorPos;
	const ImVec2 size       = ImGui::CalcItemSize( { 140, 142 }, 142, 142 );
	ImRect bb( pos, pos + size );

	ImGui::ItemSize( size, style.FramePadding.y );
	ImGui::ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held, 0 );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ), 3 );
	window->DrawList->AddRectFilled( bb.Min + ImVec2( 0, 140 ), bb.Max, RarityGetter( 1 ), 3, ImDrawCornerFlags_Bot );
	window->DrawList->AddImage( Item->m_tTexture, bb.Min + ImVec2( 17, 37 - 10 ), bb.Min + ImVec2( 125, 105 - 10 ), ImVec2( 0, 0 ), ImVec2( 1, 1 ),
	                            ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ) ); // 108  68

	ImGui::GetWindowDrawList( )->PushClipRect( bb.Min, bb.Max, true );
	ImGui::GetWindowDrawList( )->AddText( bb.Min + ImVec2( 10, 142 - label_size.y - 10 ),
	                                      ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
	                                      LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ) );
	ImGui::GetWindowDrawList( )->PopClipRect( );

	window->DrawList->AddRectFilledMultiColor( bb.Min + ImVec2( 110, 142 - label_size.y - 10 ), bb.Max - ImVec2( 0, 6 ), ImColor( 58, 58, 58, 0 ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, 0 ) );

	return pressed;
}

bool InventoryItem( WeaponItem_t* Item )
{
	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	ImGuiContext& g     = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id        = window->GetID( Item->m_tPaintKit->m_szName );
	const ImVec2 label_size = ImGui::CalcTextSize( LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ), NULL, true );
	const ImVec2 pos        = window->DC.CursorPos;
	const ImVec2 size       = ImGui::CalcItemSize( { 140, 142 }, 142, 142 );
	ImRect bb( pos, pos + size );

	ImGui::ItemSize( size, style.FramePadding.y );
	ImGui::ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held, 0 );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ), 3 );
	window->DrawList->AddRectFilled( bb.Min + ImVec2( 0, 140 ), bb.Max, RarityGetter( 1 ), 3, ImDrawCornerFlags_Bot );
	window->DrawList->AddImage( Item->m_tTexture, bb.Min + ImVec2( 17, 37 - 10 ), bb.Min + ImVec2( 125, 105 - 10 ), ImVec2( 0, 0 ), ImVec2( 1, 1 ),
	                            ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ) ); // 108  68

	ImGui::GetWindowDrawList( )->PushClipRect( bb.Min, bb.Max, true );
	ImGui::GetWindowDrawList( )->AddText( bb.Min + ImVec2( 10, 142 - label_size.y - 10 ),
	                                      ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
	                                      LocalizeTex22( Item->m_szName.c_str( ) ).c_str( ) );
	ImGui::GetWindowDrawList( )->PopClipRect( );

	window->DrawList->AddRectFilledMultiColor( bb.Min + ImVec2( 110, 142 - label_size.y - 10 ), bb.Max - ImVec2( 0, 6 ), ImColor( 58, 58, 58, 0 ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, int( 255 * g_menu.m_animation_progress ) ),
	                                           ImColor( 58, 58, 58, 0 ) );

	return pressed;
}

std::string ToUtf88( std::wstring_view wstr )
{
	if ( wstr.empty( ) )
		return std::string( );

	auto size_needed = WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int32_t >( wstr.size( ) ), NULL, 0, NULL, NULL );

	std::string out( size_needed, 0 );
	WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int >( wstr.size( ) ), &out[ 0 ], size_needed, NULL, NULL );

	return out;
}
std::string LocalizeTex2( const char* in )
{
	const auto wide_name = g_interfaces.m_localize->find_safe( in );

	if ( !wcslen( wide_name ) )
		return "";

	return ToUtf88( wide_name );
}



extern bool SaveInventory( const std::unordered_map< uint64_t, InventoryItem_t >& inventory, const char* filename );
extern bool LoadInventory( std::unordered_map< uint64_t, InventoryItem_t >& inventory, const char* filename );
void n_menu::impl_t::on_end_scene( )
{
	ImGui::GetStyle( ).Colors[ ImGuiCol_::ImGuiCol_Accent ] = GET_VARIABLE( g_variables.m_accent, c_color ).get_vec4( );

	// Обновление прогресса анимации (оставляем как есть)
	float delta_time = ImGui::GetIO( ).DeltaTime;
	if ( m_opened && m_animation_progress < 1.0f ) {
		m_is_opening = true;
		m_animation_progress += delta_time * m_animation_speed;
	} else if ( !m_opened && !point_menu_is_opened( ) && m_animation_progress > 0.0f ) {
		m_is_opening = false;
		m_animation_progress -= delta_time * m_animation_speed;
	}

	if ( point_menu_is_opened( ) ) {
		m_opened = true;
	}

	if ( point_menu_is_opened( ) && m_animation_progress < 1.0f ) {
		m_animation_progress += delta_time * m_animation_speed;
	}
	m_animation_progress = std::clamp( m_animation_progress, 0.0f, 1.0f );

	if ( m_animation_progress <= 0.0f || point_menu_is_opened( ) ) {
		return;
	}


	{
		auto& style                                   = ImGui::GetStyle( );
		style.Colors[ ImGuiCol_::ImGuiCol_WindowBg ]  = ImVec4( 10 / 255.f, 10 / 255.f, 10 / 255.f, float( 1.f * g_menu.m_animation_progress ) );
		style.Colors[ ImGuiCol_::ImGuiCol_ChildBg ]   = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, float( 1.f * g_menu.m_animation_progress ) );
		style.Colors[ ImGuiCol_::ImGuiCol_FrameBg ]   = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, float( 1.f * g_menu.m_animation_progress ) );
		style.Colors[ ImGuiCol_::ImGuiCol_PopupBg ]   = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, float( 1.f * g_menu.m_animation_progress ) );
		style.Colors[ ImGuiCol_::ImGuiCol_CheckMark ] = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, float( 1.f * g_menu.m_animation_progress ) );
		style.Colors[ ImGuiCol_::ImGuiCol_Button ]    = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, float( 1.f * g_menu.m_animation_progress ) );

		style.Colors[ ImGuiCol_::ImGuiCol_Border ]       = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 0.f );
		style.Colors[ ImGuiCol_::ImGuiCol_BorderShadow ] = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 0.f );
		style.Colors[ ImGuiCol_Text ]                    = ImVec4( 230 / 255.f, 230 / 255.f, 230 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TextDisabled ]            = ImVec4( 128 / 255.f, 128 / 255.f, 128 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_FrameBgHovered ]          = ImVec4( 30 / 255.f, 30 / 255.f, 30 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_FrameBgActive ]           = ImVec4( 35 / 255.f, 35 / 255.f, 35 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TitleBg ]                 = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TitleBgActive ]           = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TitleBgCollapsed ]        = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_MenuBarBg ]               = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ScrollbarBg ]             = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ScrollbarGrab ]           = ImVec4( 60 / 255.f, 60 / 255.f, 60 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ScrollbarGrabHovered ]    = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ScrollbarGrabActive ]     = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_SliderGrab ]              = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_SliderGrabActive ]        = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ButtonHovered ]           = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ButtonActive ]            = ImVec4( 30 / 255.f, 30 / 255.f, 30 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_Header ] =
			ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.f * g_menu.m_animation_progress ); // Можно задать так же, как для Tab
		style.Colors[ ImGuiCol_HeaderHovered ]        = ImVec4( 50 / 255.f, 50 / 255.f, 50 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_HeaderActive ]         = ImVec4( 45 / 255.f, 45 / 255.f, 45 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_Separator ]            = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_SeparatorHovered ]     = ImVec4( 60 / 255.f, 60 / 255.f, 60 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_SeparatorActive ]      = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ResizeGrip ]           = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ResizeGripHovered ]    = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ResizeGripActive ]     = ImVec4( 120 / 255.f, 120 / 255.f, 120 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_Tab ]                  = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TabHovered ]           = ImVec4( 40 / 255.f, 40 / 255.f, 40 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TabActive ]            = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TabUnfocused ]         = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TabUnfocusedActive ]   = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_PlotLines ]            = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_PlotLinesHovered ]     = ImVec4( 120 / 255.f, 120 / 255.f, 120 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_PlotHistogram ]        = ImVec4( 150 / 255.f, 150 / 255.f, 150 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_PlotHistogramHovered ] = ImVec4( 170 / 255.f, 170 / 255.f, 170 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TableHeaderBg ]        = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TableBorderStrong ]    = ImVec4( 30 / 255.f, 30 / 255.f, 30 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TableBorderLight ]     = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TableRowBg ]           = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TableRowBgAlt ]        = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_TextSelectedBg ]       = ImVec4( 40 / 255.f, 40 / 255.f, 40 / 255.f, 1.f * g_menu.m_animation_progress );
		// Нестандартный выбор цвета для эффекта DragDrop (можно менять по вкусу)
		style.Colors[ ImGuiCol_DragDropTarget ]        = ImVec4( 255 / 255.f, 255 / 255.f, 0 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_NavHighlight ]          = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_NavWindowingHighlight ] = ImVec4( 255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f * g_menu.m_animation_progress );
		// Для затемнения окон при навигации или модальных окнах можно задать меньшую альфу:
		style.Colors[ ImGuiCol_NavWindowingDimBg ] = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 0.5f * g_menu.m_animation_progress );
		style.Colors[ ImGuiCol_ModalWindowDimBg ]  = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 0.5f * g_menu.m_animation_progress );
	}

	const float menu_scale = 0.975f + 0.025f * ( m_animation_progress * m_animation_progress * ( 3.f - 2.f * m_animation_progress ) );
	ImGui::SetNextWindowSize( ImVec2( 650.f, 520.f ) * menu_scale, ImGuiCond_::ImGuiCond_Always );
	ImGui::Begin( n_branding::k_client_name, nullptr,
	              ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground |
	                  ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse |
	                  ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground );
	{
		/* Отрисовка фона, обводки и иконки (оставляем как есть) */
		ImGui::PushClipRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ), false );
		ImGui::GetBackgroundDrawList( )->AddRectFilled( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                                ImColor( 7, 7, 7, int(255 * m_animation_progress) ), 6.0f, ImDrawFlags_RoundCornersAll );
		ImGui::GetBackgroundDrawList( )->AddRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                          ImColor( 25, 25, 25, int( 255 * m_animation_progress ) ), 6.0f, ImDrawFlags_RoundCornersAll );
		if ( const auto logo_texture = get_menu_logo_texture( ) ) {
			const auto logo_min = ImGui::GetWindowPos( ) + ImVec2( 30.0f, 30.0f );
			const auto logo_max = logo_min + ImVec2( 92.0f, 42.0f );
			ImGui::GetBackgroundDrawList( )->AddImage( logo_texture, logo_min, logo_max, ImVec2( 0.f, 0.f ), ImVec2( 1.f, 1.f ),
			                                           ImColor( 255, 255, 255, int( 255 * m_animation_progress ) ) );
		}
		ImGui::PopClipRect( );

		// Определение вкладок
		std::vector< tab_t > tabs = { { "legit", "B", { "general" } },
			                          { "visuals", "A", { "entities", "game", "screen" } },
			                          { "misc", "C", { "general" } },
			                          { "movement", "O", { "general", "recorder" } },
			                          { "skins", "D", { "general" } },
			                          { "fonts", "I", { "general" } },
			                          { "config", "G", { "general" } },
			                          { "lua", "L", { "general" } } };

		if ( tab_number >= static_cast< int >( tabs.size( ) ) ) {
			tab_number = 0;
			subtab_number = 0;
		}

		custom_tabs( tabs );

		ImGui::PushFont( fonts_for_gui::regular );

		// Вкладка "legit" (aimbot, tab_number == 0)
		if ( tab_number == 0 ) {
			if ( subtab_number == 0 ) { // Под вкладка "legit"
				static int index = 0;
				ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
				child( "main", ImVec2( 230.0f, 310.0f ),
				       []( ) { 
						const char* weapons[] = { "sniper", "rifle", "shotgun", "machinegun", "pistol" };
						combo( "weapon", index, weapons, 5 );

						checkbox( "enable aimbot", &g_aimbot.m_aimbot_settings[ index ].m_enable );
						checkbox( "enable silent", &g_aimbot.m_aimbot_settings[ index ].m_silent );
						checkbox( "enable recoil control", &g_aimbot.m_aimbot_settings[ index ].m_recoil_control );
						checkbox( "enable visibility check", &g_aimbot.m_aimbot_settings[ index ].m_visibility_check );
						checkbox( "enable backtrack", &GET_VARIABLE( g_variables.m_backtrack_enable, bool ) );
						checkbox( "enable extended backtrack", &GET_VARIABLE( g_variables.m_backtrack_extend, bool ) );
						if ( GET_VARIABLE( g_variables.m_backtrack_enable,bool) )
							checkbox( "enable aim to backtrack", &g_aimbot.m_aimbot_settings[ index ].m_aim_to_backtrack );
						const char* hitboxes[] = { "head", "neck", "body", "legs" };
						multi_combo( "hitboxes", g_aimbot.m_aimbot_settings[ index ].m_hitboxes, hitboxes, 4 );
					
					
					} );

				ImGui::SetCursorPos( ImVec2( 150.0f+240.f, 10.0f ) );
				child( "configuration", ImVec2( 230.0f, 310.0f ), []( ) {
					// Пустое окно конфигурации (можно дополнить позже)
					slider_float( "fov", &g_aimbot.m_aimbot_settings[ index ].m_fov, 0.f, 180.f, "%.1f" );
					slider_float( "smooth", &g_aimbot.m_aimbot_settings[ index ].m_smooth, 1.f, 100.f, "%.1f" );
					slider_int( "backtrack time limit", &GET_VARIABLE( g_variables.m_backtrack_time_limit,int ), 0, 200, "%d" );
					slider_int( "ping adder", &GET_VARIABLE( g_variables.fakeLatencyAmount, int ), 0, 200, "%d" );
				} );
			}
		}


		// Вкладка "visuals" (tab_number == 1)
		if ( tab_number == 1 ) {
			// Players ESP
			if ( subtab_number == 0 ) {
				ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
				child( "players esp", ImVec2( 230.0f, 380.0f ), []( ) {
					checkbox( "enable visuals", &GET_VARIABLE( g_variables.esp_enable, bool ) );
					const char* entity_types[] = { "enemies", "teammates", "all" };
					combo( "entity type", GET_VARIABLE( g_variables.m_players_entity_type, int ), entity_types, IM_ARRAYSIZE( entity_types ) );
					checkbox( "only draw if audible", &GET_VARIABLE( g_variables.m_players_only_audible, bool ) );
					checkbox( "players", &GET_VARIABLE( g_variables.m_players, bool ) );
					

					if ( GET_VARIABLE( g_variables.m_players, bool ) ) {
						// Bounding Box popup
						checkbox( "bounding box", &GET_VARIABLE( g_variables.m_players_box, bool ), false, true, { 200, -1 }, []( ) {
							checkbox( "corner bounding box", &GET_VARIABLE( g_variables.m_players_box_corner, bool ) );

							checkbox( "bounding box outline", &GET_VARIABLE( g_variables.m_players_box_outline, bool ) );
							ImGui::Spacing( );
							ImGui::Text( "player bounding box color" );
							ImGui::ColorEdit4( "##player bounding box color", &GET_VARIABLE( g_variables.m_players_box_color, c_color ),
							                   color_picker_alpha_flags );
							if ( GET_VARIABLE( g_variables.m_players_box_outline, bool ) ) {
								ImGui::Spacing( );
								ImGui::Text( "player box outline color" );
								ImGui::ColorEdit4( "##player bounding box outline color",
								                   &GET_VARIABLE( g_variables.m_players_box_outline_color, c_color ), color_picker_alpha_flags );
							}
						} );

						// Name popup
						checkbox( "name", &GET_VARIABLE( g_variables.m_players_name, bool ), false, true, { 200, -1 }, []( ) {
							ImGui::Text( "player name color" );

							ImGui::ColorEdit4( "##player name color", &GET_VARIABLE( g_variables.m_players_name_color, c_color ),
							                   color_picker_alpha_flags );
						} );

						// Health bar popup
						checkbox( "health bar", &GET_VARIABLE( g_variables.health_bar_enable, bool ), false, true, { 200.f, -1.f }, []( ) {
							checkbox( "gradient color", &GET_VARIABLE( g_variables.health_bar_gradient, bool ) );

							checkbox( "color based on health", &GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) );
							checkbox( "fill outline background", &GET_VARIABLE( g_variables.health_bar_background, bool ) );
							checkbox( "text on bar", &GET_VARIABLE( g_variables.health_bar_text, bool ) );
							slider_int( "bar width", &GET_VARIABLE( g_variables.health_bar_size, int ), 1, 10, "%.d" );
							ImGui::Spacing( );
							ImGui::Text( "healthbar outline color" );
							ImGui::ColorEdit4( "##healthbar outline", &GET_VARIABLE( g_variables.colors_health_bar_outline, c_color ) );
							if ( GET_VARIABLE( g_variables.colors_custom, bool ) ) {
								ImGui::Spacing( );
								ImGui::Text( "healthbar color" );
								ImGui::ColorEdit4( "##healthbar color", &GET_VARIABLE( g_variables.colors_health_bar, c_color ) );
							}
							if ( GET_VARIABLE( g_variables.health_bar_gradient, bool ) ) {
								ImGui::Spacing( );
								ImGui::Text( "healthbar upper color" );
								ImGui::Spacing( );
								ImGui::ColorEdit4( "##healthbar upper color", &GET_VARIABLE( g_variables.colors_health_bar_upper, c_color ) );
								ImGui::Text( "healthbar lower color" );
								ImGui::ColorEdit4( "##healthbar lower color", &GET_VARIABLE( g_variables.colors_health_bar_lower, c_color ) );
							}
						} );

						checkbox( "player health text", &GET_VARIABLE( g_variables.m_players_health_text, bool ) );
						checkbox( "player flags", &GET_VARIABLE( g_variables.m_players_flags, bool ) );

					

						// Weapon name popup
						checkbox( "weapon name", &GET_VARIABLE( g_variables.m_weapon_name, bool ), false, true, { 200, -1 }, []( ) {
							ImGui::Text( "weapon name color" );

							ImGui::ColorEdit4( "##weapon name color", &GET_VARIABLE( g_variables.m_weapon_name_color, c_color ),
							                   color_picker_alpha_flags );
						} );

						// Weapon icon popup
						checkbox( "weapon icon", &GET_VARIABLE( g_variables.m_weapon_icon, bool ), false, true, { 200, -1 }, []( ) {
							ImGui::Text( "weapon icon color" );

							ImGui::ColorEdit4( "##weapon icon color", &GET_VARIABLE( g_variables.m_weapon_icon_color, c_color ),
							                   color_picker_alpha_flags );
						} );

						// Weapon ammo bar popup
						checkbox( "weapon ammo bar", &GET_VARIABLE( g_variables.m_player_ammo_bar, bool ), false, true, { 200, -1 }, []( ) {
							ImGui::Text( "weapon ammo bar color" );

							ImGui::ColorEdit4( "##weapon ammo bar color", &GET_VARIABLE( g_variables.m_player_ammo_bar_color, c_color ),
							                   color_picker_alpha_flags );
						} );
						checkbox( "weapon ammo text", &GET_VARIABLE( g_variables.m_player_ammo_text, bool ) );

						// Player skeleton popup
						checkbox( "player skeleton", &GET_VARIABLE( g_variables.m_players_skeleton, bool ), false, true, { 200, -1 }, []( ) {
							const char* skeleton_types[] = { "normal", "lag compensated" };
							combo( "skeleton type", GET_VARIABLE( g_variables.m_players_skeleton_type, int ), skeleton_types, 2 );
							ImGui::Spacing( );
							ImGui::Text( "player skeleton color" );
							ImGui::ColorEdit4( "##player skeleton color", &GET_VARIABLE( g_variables.m_players_skeleton_color, c_color ),
							                   color_picker_alpha_flags );
						} );

						// Player avatar (без popup)
						checkbox( "players avatar", &GET_VARIABLE( g_variables.m_players_avatar, bool ) );
						checkbox( "distance", &GET_VARIABLE( g_variables.m_players_distance, bool ) );
						checkbox( "emitted sounds", &GET_VARIABLE( g_variables.m_players_emitted_sounds, bool ) );
						checkbox( "damage indicators", &GET_VARIABLE( g_variables.m_players_damage_indicators, bool ) );
						checkbox( "backtrack eye trail", &GET_VARIABLE( g_variables.m_players_backtrack_eye_trail, bool ) );
						checkbox( "snaplines", &GET_VARIABLE( g_variables.m_players_snaplines, bool ) );
						checkbox( "radar reveal", &GET_VARIABLE( g_variables.m_players_radar, bool ) );

						// Out of FOV arrows popup
						checkbox(
							"out of fov arrows", &GET_VARIABLE( g_variables.m_out_of_fov_arrows, bool ), false, true, ImVec2( 200.f, -1.f ), []( ) {
								ImGui::SetCursorPosX( 25.f );
								ImGui::Text( "fov arrows color" );
								ImGui::ColorEdit4( "##out of fov arrows color", &GET_VARIABLE( g_variables.m_out_of_fov_arrows_color, c_color ),
							                       color_picker_alpha_flags );
								ImGui::Spacing( );
								slider_float( "arrows size", &GET_VARIABLE( g_variables.m_out_of_fov_arrows_size, float ), 0.1f, 50.f, "%.1f px" );
								slider_int( "arrows distance", &GET_VARIABLE( g_variables.m_out_of_fov_arrows_distance, int ), 10, 500, "%d px" );
							} );
						slider_int( "visible alpha", &GET_VARIABLE( g_variables.m_visible_alpha, int ), 0, 255, "%d" );
						slider_int( "in visible alpha", &GET_VARIABLE( g_variables.m_invisuals_alpha, int ), 0, 255, "%d" );

					}
				} );

				// Chams settings
				ImGui::SetCursorPos( ImVec2( 150.0f + 240.f, 10.0f ) );
				child( "chams", ImVec2( 230.0f, 310.0f ), []( ) {
					// visible chams
					checkbox( "visible model chams", &GET_VARIABLE( g_variables.m_player_visible_chams, bool ), false, true, { 200.f, -1.f }, []( ) {
						// Сначала выбор материала, затем цвет.
						const char* chams_materialss[] = { "default", "flat", "glow", "wireframe", "metallic" };

						combo( "visible chams material", GET_VARIABLE( g_variables.m_player_visible_chams_material, int ), chams_materialss, 5 );
						ImGui::Spacing( );
						ImGui::Text( "visible chams color" );
						ImGui::ColorEdit4( "##visible chams color", &GET_VARIABLE( g_variables.m_player_visible_chams_color, c_color ),
						                   color_picker_alpha_flags );
					} );

					// invisible chams
					checkbox( "invisible model chams", &GET_VARIABLE( g_variables.m_player_invisible_chams, bool ), false, true, { 200.f, -1.f }, []( ) {
						const char* chams_materialss[] = { "default", "flat", "glow", "wireframe", "metallic" };

						combo( "invisible chams material", GET_VARIABLE( g_variables.m_player_invisible_chams_material, int ), chams_materialss, 5 );
						ImGui::Spacing( );
						ImGui::Text( "invisible chams color" );
						ImGui::ColorEdit4( "##invisible chams color", &GET_VARIABLE( g_variables.m_player_invisible_chams_color, c_color ),
						                   color_picker_alpha_flags );
					} );
					checkbox( "visible attachment chams", &GET_VARIABLE( g_variables.m_player_visible_attachment_chams, bool ) );
					checkbox( "invisible attachment chams", &GET_VARIABLE( g_variables.m_player_invisible_attachment_chams, bool ) );

					// lag compensated chams
					checkbox( "backtrack chams", &GET_VARIABLE( g_variables.m_player_lag_chams, bool ), false, true, { 200.f, -1.f }, []( ) {
						const char* chams_materialss[] = { "default", "flat", "glow", "wireframe", "metallic" };

						checkbox( "lag compensated chams xqz", &GET_VARIABLE( g_variables.m_player_lag_chams_xqz, bool ) );
						combo( "lag compensated chams material", GET_VARIABLE( g_variables.m_player_lag_chams_material, int ), chams_materialss, 5 );
						const char* lag_chams_types[] = { "oldest record", "all records" };
					
						combo( "lag compensated chams type", GET_VARIABLE( g_variables.m_player_lag_chams_type, int ), lag_chams_types, 2 );
						ImGui::Spacing( );
						ImGui::Text( "lagcomp chams color" );
						ImGui::ColorEdit4( "##lagcomp chams color", &GET_VARIABLE( g_variables.m_player_lag_chams_color, c_color ),
						                   color_picker_alpha_flags );
					} );
				} );

				// Glow settings
				ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f + 380.f ) );
				child( "glow", ImVec2( 230.0f, 100.0f ), []( ) {
					// Перемещаем цвета в popup чекбокса "player glow"
					checkbox( "visible glow", &GET_VARIABLE( g_variables.m_visible_glow, bool ), false, true, { 200.f, -1.f }, []( ) {
						ImGui::Text( "player visible glow color" );

						ImGui::ColorEdit4( "##player vis glow color", &GET_VARIABLE( g_variables.m_glow_vis_color, c_color ),
						                   color_picker_alpha_flags );
					} );
					checkbox( "invisible glow", &GET_VARIABLE( g_variables.m_invisible_glow, bool ), false, true, { 200.f, -1.f }, []( ) {
						ImGui::Spacing( );
						ImGui::Text( "player invisible glow color" );
						ImGui::ColorEdit4( "##player invis glow color", &GET_VARIABLE( g_variables.m_glow_invis_color, c_color ),
						                   color_picker_alpha_flags );
					} );
					checkbox( "visible stencil glow", &GET_VARIABLE( g_variables.m_visible_stencil_glow, bool ) );
					checkbox( "invisible stencil glow", &GET_VARIABLE( g_variables.m_invisible_stencil_glow, bool ) );
					checkbox( "world glow (lights)", &GET_VARIABLE( g_variables.m_world_glow_lights, bool ) );
				} );

				ImGui::SetCursorPos( ImVec2( 150.0f + 240.f, 326.0f ) );
				child( "preview", ImVec2( 230.0f, 124.0f ), []( ) {
					checkbox( "player preview", &GET_VARIABLE( g_variables.m_player_preview, bool ) );
					if ( !GET_VARIABLE( g_variables.m_player_preview, bool ) )
						return;

					ImDrawList* draw = ImGui::GetWindowDrawList( );
					const ImVec2 pos = ImGui::GetCursorScreenPos( ) + ImVec2( 8.f, 4.f );
					const ImColor accent = GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 0.9f );
					draw->AddRectFilled( pos, pos + ImVec2( 78.f, 80.f ), ImColor( 8, 8, 9, 210 ), 4.f );
					draw->AddRect( pos, pos + ImVec2( 78.f, 80.f ), accent, 4.f );
					draw->AddCircle( pos + ImVec2( 37.f, 16.f ), 8.f, ImColor( 220, 220, 220, 210 ), 24, 1.2f );
					draw->AddLine( pos + ImVec2( 37.f, 25.f ), pos + ImVec2( 37.f, 50.f ), ImColor( 220, 220, 220, 210 ), 1.2f );
					draw->AddLine( pos + ImVec2( 22.f, 35.f ), pos + ImVec2( 52.f, 35.f ), ImColor( 220, 220, 220, 210 ), 1.2f );
					draw->AddLine( pos + ImVec2( 37.f, 50.f ), pos + ImVec2( 25.f, 70.f ), ImColor( 220, 220, 220, 210 ), 1.2f );
					draw->AddLine( pos + ImVec2( 37.f, 50.f ), pos + ImVec2( 50.f, 70.f ), ImColor( 220, 220, 220, 210 ), 1.2f );
					draw->AddText( pos + ImVec2( 92.f, 10.f ), ImColor( 230, 230, 230, 230 ), "enemy" );
					draw->AddText( pos + ImVec2( 92.f, 31.f ), accent, "100 hp" );
					draw->AddText( pos + ImVec2( 92.f, 52.f ), ImColor( 180, 180, 180, 230 ), "rifle" );
				} );
			} 
			else if ( subtab_number == 1 ) {
				ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
				child( "world", ImVec2( 230.0f, 430.0f ), []( ) {
					combo( "skybox changer", GET_VARIABLE( g_variables.m_sky_box, int ), (const char**)skyboxList.data( ), skyboxList.size( ) );
					if ( GET_VARIABLE( g_variables.m_sky_box, int ) == static_cast< int >( skyboxList.size( ) - 1 ) ) {
						static char buf[ 256 ];
						ImGui::Spacing( );
						ImGui::InputText( "skybox filename", buf, sizeof( buf ) );
						g_variables.m_custom_sky_box = buf;
						if ( ImGui::IsItemHovered( ) )
							ImGui::SetTooltip( "skybox files must be put in csgo/materials/skybox/ " );
					}
					checkbox( "world modulation", &GET_VARIABLE( g_variables.m_world_modulation, bool ), false, true, { 220, -1 }, []( ) {
						checkbox( "world color", &GET_VARIABLE( g_variables.m_world_modulate_world, bool ) );
						ImGui::ColorEdit4( "##world modulation color", &GET_VARIABLE( g_variables.m_world_modulation_world_color, c_color ),
						                   color_picker_alpha_flags );
						checkbox( "prop/static color", &GET_VARIABLE( g_variables.m_world_modulate_props, bool ) );
						ImGui::ColorEdit4( "##prop modulation color", &GET_VARIABLE( g_variables.m_world_modulation_prop_color, c_color ),
						                   color_picker_alpha_flags );
						checkbox( "sky color", &GET_VARIABLE( g_variables.m_world_modulate_sky, bool ) );
						ImGui::ColorEdit4( "##sky modulation color", &GET_VARIABLE( g_variables.m_world_modulation_sky_color, c_color ),
						                   color_picker_alpha_flags );
						slider_float( "intensity", &GET_VARIABLE( g_variables.m_world_modulation_intensity, float ), 0.f, 2.f, "%.2f" );
						slider_float( "brightness", &GET_VARIABLE( g_variables.m_world_modulation_brightness, float ), 0.1f, 2.f, "%.2f" );
						slider_float( "exposure", &GET_VARIABLE( g_variables.m_world_modulation_exposure, float ), 0.25f, 2.f, "%.2f" );
					} );
					checkbox( "shadows", &GET_VARIABLE( g_variables.m_shadows, bool ) );
					const char* world_materials[] = { "off", "fullbright", "flat tint" };
					combo( "world material changer", GET_VARIABLE( g_variables.m_world_material_changer, int ), world_materials,
					       IM_ARRAYSIZE( world_materials ) );
					checkbox( "ragdolls", &GET_VARIABLE( g_variables.m_ragdolls, bool ) );
					checkbox( "fullbright", &GET_VARIABLE( g_variables.m_fullbright, bool ) );
					checkbox( "world glow (lights)", &GET_VARIABLE( g_variables.m_world_glow_lights, bool ) );
					checkbox( "precipitation", &GET_VARIABLE( g_variables.m_precipitation, bool ), false, true, { 200, -1 }, []( ) {
						const char* precipitation_types[] = { "rain", "ash", "rain storm", "snow" };
						combo( "type", GET_VARIABLE( g_variables.m_precipitation_type, int ), precipitation_types, 4 );
					} );


						checkbox( "disable post processing", &GET_VARIABLE( g_variables.m_disable_post_processing, bool ) );
					checkbox( "remove panorama blur", &GET_VARIABLE( g_variables.m_remove_panorama_blur, bool ) );
#ifdef _DEBUG
					checkbox( "disable interp", &GET_VARIABLE( g_variables.m_disable_interp, bool ) );
#endif

					checkbox( "modulate fog", &GET_VARIABLE( g_variables.m_fog, bool ), false, true, { 200, -1 }, []( ) {
						ImGui::Text( "fog color" );
						ImGui::ColorEdit4( "##fog color picker", &GET_VARIABLE( g_variables.m_fog_color, c_color ), color_picker_alpha_flags );
						slider_float( "start ", &GET_VARIABLE( g_variables.m_fog_start, float ), 0.f, 5000.f, "%.0f u" );
						slider_float( "end ", &GET_VARIABLE( g_variables.m_fog_end, float ), 0.f, 5000.f, "%.0f u" );
					} );

					checkbox( "customize lighting", &GET_VARIABLE( g_variables.m_sun_set, bool ), false, true, { 200, -1 }, []( ) {
						if ( ImGui::IsItemHovered( ) ) {
							ImGui::SetTooltip( "if you want restore shadows setup all on zero" );
						}
						slider_float( "sunset rot x", &GET_VARIABLE( g_variables.m_sun_set_rot_x, float ), -1000.f, 1000.f, "%.2f" );
						slider_float( "sunset rot y", &GET_VARIABLE( g_variables.m_sun_set_rot_y, float ), -1000.f, 1000.f, "%.2f" );
						slider_float( "sunset rot z", &GET_VARIABLE( g_variables.m_sun_set_rot_z, float ), -1000.f, 1000.f, "%.2f" );
						slider_float( "sunset rotation speed", &GET_VARIABLE( g_variables.m_sun_set_rotation_speed, float ), 0.0f, 1.0f, "%.2f" );
					} );

					checkbox( "smoke color", &GET_VARIABLE( g_variables.m_custom_smoke, bool ), false, true, { 200, -1 }, []( ) {
						ImGui::Text( "smoke color" );
						ImGui::ColorEdit4( "##custom smoke color picker", &GET_VARIABLE( g_variables.m_custom_smoke_color, c_color ),
						                   color_picker_alpha_flags );
					} );

					checkbox( "fire color", &GET_VARIABLE( g_variables.m_custom_molotov, bool ), false, true, { 200, -1 }, []( ) {
						ImGui::Text( "fire color" );
						ImGui::ColorEdit4( "##custom molotov color picker", &GET_VARIABLE( g_variables.m_custom_molotov_color, c_color ),
						                   color_picker_alpha_flags );
					} );

					checkbox( "custom blood color", &GET_VARIABLE( g_variables.m_custom_blood, bool ), false, true, { 200, -1 }, []( ) {
						ImGui::Text( "custom blood color" );
						ImGui::ColorEdit4( "##custom blood color picker", &GET_VARIABLE( g_variables.m_custom_blood_color, c_color ),
						                   color_picker_alpha_flags );
					} );

					checkbox( "custom precipitation color", &GET_VARIABLE( g_variables.m_custom_precipitation, bool ), false, true, { 200, -1 },
					          []( ) {
								  ImGui::Text( "custom precipitation color" );
								  ImGui::ColorEdit4( "##custom precipitation color picker",
						                             &GET_VARIABLE( g_variables.m_custom_precipitation_color, c_color ), color_picker_alpha_flags );
							  } );

					checkbox( "motion blur", &GET_VARIABLE( g_variables.m_motion_blur, bool ), false, true, { 200, -1 }, []( ) {
						
						
								checkbox( "forward enabled", &GET_VARIABLE( g_variables.m_motion_forward_move_blur, bool ) );
								slider_float( "strength", &GET_VARIABLE( g_variables.m_motion_strength, float ), 0.0f, 10.0f, "%.2f" );
								slider_float( "falling intensity", &GET_VARIABLE( g_variables.m_motion_falling_intensity, float ), 0.0f, 20.0f,
							                        "%.2f" );
								slider_float( "rotation intensity", &GET_VARIABLE( g_variables.m_motion_rotate_intensity, float ), 0.0f, 20.0f,
							                        "%.2f" );
								slider_float( "falling min", &GET_VARIABLE( g_variables.m_motion_falling_minimum, float ), 0.0f, 50.0f,
							                        "%.2f" );
								slider_float( "falling max", &GET_VARIABLE( g_variables.m_motion_falling_maximum, float ), 0.0f, 50.0f,
							                        "%.2f" );
							
					} );

					ImGui::Spacing( );
					ImGui::SetCursorPosX( 10.f );
					ImGui::Text( "accent color" );

					ImGui::ColorEdit4( "##accent color", &GET_VARIABLE( g_variables.m_accent, c_color ), color_picker_no_alpha_flags );
			
				}
						);

				ImGui::SetCursorPos( ImVec2( 150.0f + 240.f, 10.0f ) );
				child( "view", ImVec2( 230.0f, 430.0f ), []( ) {
					checkbox( "thirdperson", &GET_VARIABLE( g_variables.m_thirdperson, bool ), false, true, { 200, -1 }, []( ) {
						slider_int( "thirdperson distance", &GET_VARIABLE( g_variables.m_thirdperson_distance, int ), 40, 240, "%d u" );
					} );
					checkbox( "change viewmodel offsets", &GET_VARIABLE( g_variables.m_viewmodel_offsets, bool ), false, true, { 200, -1 }, []( ) {
						slider_float( "offset x", &GET_VARIABLE( g_variables.m_viewmodel_offset_x, float ), -20.f, 20.f, "%.1f" );
						slider_float( "offset y", &GET_VARIABLE( g_variables.m_viewmodel_offset_y, float ), -20.f, 20.f, "%.1f" );
						slider_float( "offset z", &GET_VARIABLE( g_variables.m_viewmodel_offset_z, float ), -20.f, 20.f, "%.1f" );
					} );
					checkbox( "change viewmodel fov", &GET_VARIABLE( g_variables.m_viewmodel_fov, bool ), false, true, { 200, -1 }, []( ) {
						slider_int( "viewmodel fov", &GET_VARIABLE( g_variables.m_viewmodel_fov_value, int ), 54, 120, "%d" );
					} );
					checkbox( "change weapon sway scale", &GET_VARIABLE( g_variables.m_weapon_sway_scale, bool ), false, true, { 200, -1 }, []() {
						slider_float( "sway scale", &GET_VARIABLE( g_variables.m_weapon_sway_scale_value, float ), 0.f, 2.f, "%.2f" );
					} );

					checkbox( "force crosshair", &GET_VARIABLE( g_variables.m_force_crosshair, bool ) );
					checkbox( "recoil crosshair", &GET_VARIABLE( g_variables.m_recoil_crosshair, bool ) );
					checkbox( "sniper crosshair", &GET_VARIABLE( g_variables.m_sniper_crosshair, bool ) );
					checkbox( "penetration reticle", &GET_VARIABLE( g_variables.m_penetration_reticle, bool ) );
					checkbox( "spread circle", &GET_VARIABLE( g_variables.m_spread_circle, bool ), false, true, { 200, -1 }, []() {
						ImGui::Text( "spread circle color" );
						ImGui::ColorEdit4( "##spread circle color", &GET_VARIABLE( g_variables.m_spread_circle_color, c_color ),
						                   color_picker_alpha_flags );
					} );
					checkbox( "hitmarker", &GET_VARIABLE( g_variables.m_hit_marker, bool ) );

					checkbox( "jump trail", &GET_VARIABLE( g_variables.m_jump_trail, bool ) );
					checkbox( "bullet tracer", &GET_VARIABLE( g_variables.m_bullet_tracer, bool ), false, true, { 200, -1 }, []() {
						const char* tracer_types[] = { "engine lightning beam", "engine ring beam", "engine sparks", "weapon confetti",
							                       "weapon confetti sparks", "ambient sparks core", "confetti A", "dust devil swirls" };
						combo( "tracer type", GET_VARIABLE( g_variables.m_bullet_tracer_type, int ), tracer_types, IM_ARRAYSIZE( tracer_types ) );
						checkbox( "use accent", &GET_VARIABLE( g_variables.m_bullet_tracer_use_accent, bool ) );
						if ( !GET_VARIABLE( g_variables.m_bullet_tracer_use_accent, bool ) ) {
							ImGui::Text( "bullet tracer color" );
							ImGui::ColorEdit4( "##bullet tracer color", &GET_VARIABLE( g_variables.m_bullet_tracer_color, c_color ),
							                   color_picker_alpha_flags );
						}
						slider_float( "tracer intensity", &GET_VARIABLE( g_variables.m_bullet_tracer_intensity, float ), 0.25f, 4.f, "%.2fx" );
						slider_float( "tracer lifetime", &GET_VARIABLE( g_variables.m_bullet_tracer_lifetime, float ), 0.06f, 1.5f, "%.2fs" );
					} );
					checkbox( "predicted grenade path", &GET_VARIABLE( g_variables.m_predicted_grenade_path, bool ), false, true, { 200, -1 }, []() {
						ImGui::Text( "grenade path color" );
						ImGui::ColorEdit4( "##grenade path color", &GET_VARIABLE( g_variables.m_predicted_grenade_path_color, c_color ),
						                   color_picker_alpha_flags );
					} );
					const char* footstep_fx_modes[] = { "off", "surface-based", "blood impact", "blood mist", "feathers", "confetti A",
						                                "confetti balloons", "dust devil", "water splash", "baggage drip", "molotov fireball",
						                                "weapon confetti", "engine dust", "engine sparks", "engine smoke", "engine ring beam",
						                                "engine lightning beam", "ambient sparks core", "weapon confetti sparks", "dust devil smoke",
						                                "dust devil swirls", "baggage splash" };
					checkbox( "footstep fx", &GET_VARIABLE( g_variables.m_footstep_fx_enabled, bool ), false, true, { 200, -1 }, [&]() {
						combo( "particle mode", GET_VARIABLE( g_variables.m_footstep_fx_mode, int ), footstep_fx_modes, IM_ARRAYSIZE( footstep_fx_modes ) );
						if ( GET_VARIABLE( g_variables.m_footstep_fx_mode, int ) > 1 && GET_VARIABLE( g_variables.m_footstep_fx_mode, int ) < 12 )
							ImGui::TextDisabled( "Named CS:GO particles log precache/dispatch results." );
						slider_float( "footstep intensity", &GET_VARIABLE( g_variables.m_footstep_fx_intensity, float ), 0.25f, 3.f, "%.2fx" );
						slider_float( "footstep cooldown", &GET_VARIABLE( g_variables.m_footstep_fx_cooldown, float ), 0.08f, 0.5f, "%.2fs" );
					} );
					checkbox( "remove / reduce flash", &GET_VARIABLE( g_variables.m_reduce_flash, bool ), false, true, { 200, -1 }, []() {
						slider_float( "flash alpha", &GET_VARIABLE( g_variables.m_flash_alpha, float ), 0.f, 255.f, "%.0f" );
					} );
					checkbox( "remove fire smoke", &GET_VARIABLE( g_variables.m_remove_fire_smoke, bool ) );
					checkbox( "bloom", &GET_VARIABLE( g_variables.m_bloom, bool ), false, true, { 200, -1 }, []() {
						slider_float( "bloom intensity", &GET_VARIABLE( g_variables.m_bloom_intensity, float ), 0.f, 6.f, "%.2f" );
						slider_float( "exposure", &GET_VARIABLE( g_variables.m_bloom_exposure, float ), 0.05f, 3.f, "%.2f" );
					} );
					checkbox( "engine ambient light", &GET_VARIABLE( g_variables.m_engine_ambient_light, bool ), false, true, { 200, -1 }, []() {
						ImGui::Text( "ambient color" );
						ImGui::ColorEdit4( "##ambient light color", &GET_VARIABLE( g_variables.m_engine_ambient_light_color, c_color ),
						                   color_picker_no_alpha_flags );
						slider_float( "ambient intensity", &GET_VARIABLE( g_variables.m_engine_ambient_light_intensity, float ), 0.f, 4.f, "%.2f" );
						if ( button( "restore ambient defaults", ImVec2( 210, 0 ) ) )
							g_misc.request_ambient_light_restore( );
					} );
					checkbox( "depth of field", &GET_VARIABLE( g_variables.m_dof, bool ), false, true, { 200, -1 }, []() {
						slider_float( "focus distance", &GET_VARIABLE( g_variables.m_dof_focus_distance, float ), 16.f, 4096.f, "%.0f" );
						slider_float( "blur amount", &GET_VARIABLE( g_variables.m_dof_blur_amount, float ), 0.f, 12.f, "%.2f" );
						slider_float( "near depth", &GET_VARIABLE( g_variables.m_dof_near_depth, float ), 1.f, 2048.f, "%.0f" );
						slider_float( "far depth", &GET_VARIABLE( g_variables.m_dof_far_depth, float ), 1.f, 4096.f, "%.0f" );
						checkbox( "smooth dof", &GET_VARIABLE( g_variables.m_dof_smooth, bool ) );
					} );
				} );

			}
			else if ( subtab_number == 2 ) {
				ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
				child( "screen fx", ImVec2( 230.0f, 430.0f ), []( ) {
					checkbox( "pts meter", &GET_VARIABLE( g_variables.m_pts_meter, bool ), false, true, { 200, -1 }, []( ) {
						ImGui::Text( "pts color" );
						ImGui::ColorEdit4( "##pts meter color", &GET_VARIABLE( g_variables.m_pts_meter_color, c_color ), color_picker_alpha_flags );
						slider_int( "pts x", &GET_VARIABLE( g_variables.m_pts_meter_x, int ), 0, g_ctx.m_width, "%d px" );
						slider_int( "pts y", &GET_VARIABLE( g_variables.m_pts_meter_y, int ), 0, g_ctx.m_height, "%d px" );
					} );
					checkbox( "speedometer", &GET_VARIABLE( g_variables.m_speedometer, bool ), false, true, { 200, -1 }, []( ) {
						const char* speedometer_styles[] = { "clean", "drain", "haunted mound", "dg", "salem", "moreru" };
						combo( "style", GET_VARIABLE( g_variables.m_speedometer_style, int ), speedometer_styles, IM_ARRAYSIZE( speedometer_styles ) );
						const char* speedometer_units[] = { "units", "km/h", "mph" };
						combo( "units", GET_VARIABLE( g_variables.m_speedometer_units, int ), speedometer_units, IM_ARRAYSIZE( speedometer_units ) );
						checkbox( "peak speed", &GET_VARIABLE( g_variables.m_speedometer_peak, bool ) );
						checkbox( "ground/air state", &GET_VARIABLE( g_variables.m_speedometer_state, bool ) );
						checkbox( "jump quality", &GET_VARIABLE( g_variables.m_speedometer_jump_quality, bool ) );
						slider_int( "speedometer x", &GET_VARIABLE( g_variables.m_speedometer_x, int ), 0, g_ctx.m_width, "%d px" );
						slider_int( "speedometer y", &GET_VARIABLE( g_variables.m_speedometer_y, int ), 0, g_ctx.m_height, "%d px" );
					} );
					checkbox( "fumo spin", &GET_VARIABLE( g_variables.m_fumo_spin, bool ), false, true, { 200, -1 }, []( ) {
						ImGui::TextWrapped( "Use it as a tiny spinning logo/plush marker. Position it away from crosshair-heavy areas." );
						ImGui::TextDisabled( "Example path: C:\\fakeDrain\\effects\\fumo_spin.vpcf" );
						slider_int( "fumo x", &GET_VARIABLE( g_variables.m_fumo_spin_x, int ), 0, g_ctx.m_width, "%d px" );
						slider_int( "fumo y", &GET_VARIABLE( g_variables.m_fumo_spin_y, int ), 0, g_ctx.m_height, "%d px" );
						slider_float( "fumo size", &GET_VARIABLE( g_variables.m_fumo_spin_size, float ), 24.f, 180.f, "%.0f px" );
						slider_float( "fumo speed", &GET_VARIABLE( g_variables.m_fumo_spin_speed, float ), 0.1f, 5.f, "%.2fx" );
						slider_float( "intensity", &GET_VARIABLE( g_variables.m_fumo_spin_intensity, float ), 0.1f, 1.f, "%.2f" );
						const char* fumo_modes[] = { "clockwise", "counter", "wobble" };
						combo( "direction", GET_VARIABLE( g_variables.m_fumo_spin_direction, int ), fumo_modes, IM_ARRAYSIZE( fumo_modes ) );
					} );
					checkbox( "velocity trail", &GET_VARIABLE( g_variables.m_velocity_trail, bool ) );
					checkbox( "speed pulse overlay", &GET_VARIABLE( g_variables.m_speed_pulse_overlay, bool ), false, true, { 200, -1 }, []( ) {
						slider_float( "pulse threshold", &GET_VARIABLE( g_variables.m_speed_pulse_threshold, float ), 120.f, 520.f, "%.0f u/s" );
					} );
					checkbox( "strafe aura", &GET_VARIABLE( g_variables.m_strafe_aura, bool ) );
					checkbox( "velocity graph", &GET_VARIABLE( g_variables.m_velocity_graph, bool ) );
					checkbox( "local afterimage", &GET_VARIABLE( g_variables.m_local_afterimage, bool ) );
				} );

				ImGui::SetCursorPos( ImVec2( 150.0f + 240.f, 10.0f ) );
				child( "movement widgets", ImVec2( 230.0f, 430.0f ), []( ) {
					checkbox( "jump / strafe feedback", &GET_VARIABLE( g_variables.m_jump_strafe_feedback, bool ), false, true, { 200, -1 }, []() {
						checkbox( "quiet sound tick", &GET_VARIABLE( g_variables.m_jump_feedback_sound, bool ) );
					} );
					checkbox( "aura debt counter", &GET_VARIABLE( g_variables.m_aura_debt_counter, bool ), false, true, { 200, -1 }, []() {
						slider_int( "aura debt x", &GET_VARIABLE( g_variables.m_aura_debt_x, int ), 0, g_ctx.m_width, "%d px" );
						slider_int( "aura debt y", &GET_VARIABLE( g_variables.m_aura_debt_y, int ), 0, g_ctx.m_height, "%d px" );
					} );
					checkbox( "drain check widget", &GET_VARIABLE( g_variables.m_drain_check_widget, bool ), false, true, { 200, -1 }, []() {
						slider_int( "drain check x", &GET_VARIABLE( g_variables.m_drain_check_x, int ), 0, g_ctx.m_width, "%d px" );
						slider_int( "drain check y", &GET_VARIABLE( g_variables.m_drain_check_y, int ), 0, g_ctx.m_height, "%d px" );
					} );
					checkbox( "schizo vision", &GET_VARIABLE( g_variables.m_schizo_vision, bool ), false, true, { 200, -1 }, []() {
						slider_float( "glitch intensity", &GET_VARIABLE( g_variables.m_schizo_vision_intensity, float ), 0.05f, 1.f, "%.2f" );
					} );
					checkbox( "npc roast", &GET_VARIABLE( g_variables.m_npc_roast, bool ) );
					checkbox( "based radar", &GET_VARIABLE( g_variables.m_based_radar, bool ) );
					checkbox( "cringe detector", &GET_VARIABLE( g_variables.m_cringe_detector, bool ) );
					checkbox( "run analyzer", &GET_VARIABLE( g_variables.m_run_analyzer, bool ), false, true, { 200, -1 }, []() {
						ImGui::TextWrapped( "Prints finished run summaries to HUD chat with real binds seen during the run." );
						slider_float( "minimum speed", &GET_VARIABLE( g_variables.m_run_analyzer_min_speed, float ), 120.f, 520.f, "%.0f u/s" );
						slider_float( "end timeout", &GET_VARIABLE( g_variables.m_run_analyzer_end_timeout, float ), 0.2f, 2.5f, "%.2fs" );
						checkbox( "show failed attempts", &GET_VARIABLE( g_variables.m_run_analyzer_show_failed, bool ) );
						checkbox( "include raw bind list", &GET_VARIABLE( g_variables.m_run_analyzer_include_raw_binds, bool ) );
						checkbox( "use accent color", &GET_VARIABLE( g_variables.m_run_analyzer_use_accent, bool ) );
						if ( !GET_VARIABLE( g_variables.m_run_analyzer_use_accent, bool ) ) {
							ImGui::Text( "run analyzer color" );
							ImGui::ColorEdit4( "##run analyzer color", &GET_VARIABLE( g_variables.m_run_analyzer_color, c_color ), color_picker_alpha_flags );
						}
					} );
					checkbox( "old edit vibes", &GET_VARIABLE( g_variables.m_old_edit_vibes, bool ), false, true, { 200, -1 }, []() {
						slider_float( "trigger chance", &GET_VARIABLE( g_variables.m_old_edit_vibes_chance, float ), 0.f, 1.f, "%.2f" );
						slider_int( "minimum combo", &GET_VARIABLE( g_variables.m_old_edit_vibes_min_combo, int ), 1, 6, "%d" );
						checkbox( "text overlay", &GET_VARIABLE( g_variables.m_old_edit_vibes_text, bool ) );
						checkbox( "local sound", &GET_VARIABLE( g_variables.m_old_edit_vibes_sound, bool ) );
						ImGui::TextDisabled( "Example path: C:\\fakeDrain\\sounds\\old_edit.wav" );
						if ( GET_VARIABLE( g_variables.m_old_edit_vibes_sound, bool ) ) {
							static char old_edit_sound[ 128 ] = "";
							static std::string last_old_edit_sound;
							auto& sound = GET_VARIABLE( g_variables.m_old_edit_vibes_sound_path, std::string );
							if ( sound != last_old_edit_sound ) {
								strncpy_s( old_edit_sound, sizeof( old_edit_sound ), sound.c_str( ), _TRUNCATE );
								last_old_edit_sound = sound;
							}
							if ( input_text( "sound path", old_edit_sound, old_edit_sound, IM_ARRAYSIZE( old_edit_sound ), 210.f, 20.f, NULL ) ) {
								sound = old_edit_sound;
								last_old_edit_sound = sound;
							}
						}
					} );
				} );
			}
		}


		// Вкладка "Misc" (tab_number == 2)
		if ( tab_number == 2 ) {
			if ( subtab_number == 0 ) { // Под вкладка "general"
				ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
				child( "game", ImVec2( 230.0f, 310.0f ), []( ) {
					checkbox( "spectator list", &GET_VARIABLE( g_variables.m_spectators_list, bool ), false, true, ImVec2( 200.f, -1.f ), []( ) {
						ImGui::SetCursorPosX( 25.f );
						ImGui::Text( "spectating local color" );
						ImGui::ColorEdit4( "##spectator list text color one", &GET_VARIABLE( g_variables.m_spectators_list_text_color_one, c_color ),
						                   color_picker_alpha_flags );
						ImGui::SetCursorPosX( 25.f );
						ImGui::Text( "spectating other color" );
						ImGui::ColorEdit4( "##spectator list text color two", &GET_VARIABLE( g_variables.m_spectators_list_text_color_two, c_color ),
						                   color_picker_alpha_flags );
						const char* listed_players[] = { "all spectators", "local spectators" };
						combo( "listed players", GET_VARIABLE( g_variables.m_spectators_list_type, int ), listed_players, 2 );
						checkbox( "spectator avatars", &GET_VARIABLE( g_variables.m_spectators_avatar, bool ) );
					} );

					checkbox( "sniper crosshair", &GET_VARIABLE( g_variables.m_sniper_crosshair, bool ) );
					checkbox( "practice window", &GET_VARIABLE( g_variables.m_practice_window, bool ), false, true, ImVec2( 200, -1 ), []( ) {
						ImGui::Text( "practice checkpoint key" );
						keybind( "practice cp key", &GET_VARIABLE( g_variables.m_practice_cp_key, key_bind_t ) );
						ImGui::Text( "practice teleport key" );
						keybind( "practice tp key", &GET_VARIABLE( g_variables.m_practice_tp_key, key_bind_t ) );
					} );
					const char* displayed_logs[] = { "damage", "team damage", "purchase", "votes" };
					multi_combo( "displayed logs", GET_VARIABLE( g_variables.m_log_types, std::vector< bool > ), displayed_logs,
					             GET_VARIABLE( g_variables.m_log_types, std::vector< bool > ).size( ) );
					checkbox( "hit sound", &GET_VARIABLE( g_variables.m_hit_sound, bool ) );
					checkbox( "hit marker", &GET_VARIABLE( g_variables.m_hit_marker, bool ) );
					checkbox( "scaleform", &GET_VARIABLE( g_variables.m_scaleform, bool ) );
					const char* watermark_modes[] = { "off", "drainware", "clarity", "rgb / chroma" };
					combo( "watermark", GET_VARIABLE( g_variables.m_watermark_mode, int ), watermark_modes, IM_ARRAYSIZE( watermark_modes ) );
					const char* watermark_accent_modes[] = { "use menu accent", "custom" };
					combo( "watermark accent", GET_VARIABLE( g_variables.m_watermark_accent_mode, int ), watermark_accent_modes,
					       IM_ARRAYSIZE( watermark_accent_modes ) );
					if ( GET_VARIABLE( g_variables.m_watermark_accent_mode, int ) == 1 ) {
						ImGui::Text( "watermark accent color" );
						ImGui::ColorEdit4( "##watermark custom accent", &GET_VARIABLE( g_variables.m_watermark_custom_accent, c_color ),
						                   color_picker_alpha_flags );
					}
					slider_float( "watermark alpha", &GET_VARIABLE( g_variables.m_watermark_background_alpha, float ), 0.15f, 1.f, "%.2f" );
					if ( GET_VARIABLE( g_variables.m_watermark_mode, int ) == 3 ) {
						slider_float( "chroma speed", &GET_VARIABLE( g_variables.m_watermark_chroma_speed, float ), 0.15f, 4.f, "%.2fx" );
						slider_float( "chroma strength", &GET_VARIABLE( g_variables.m_watermark_chroma_strength, float ), 0.f, 1.f, "%.2f" );
					}
					{
						static char watermark_nickname[ 64 ] = "verionfe";
						static std::string last_watermark_nickname = "verionfe";
						auto& nickname = GET_VARIABLE( g_variables.m_watermark_nickname, std::string );
						if ( nickname != last_watermark_nickname ) {
							strncpy_s( watermark_nickname, sizeof( watermark_nickname ), nickname.c_str( ), _TRUNCATE );
							last_watermark_nickname = nickname;
						}
						if ( input_text( "watermark nickname", watermark_nickname, watermark_nickname, IM_ARRAYSIZE( watermark_nickname ), 210.f, 20.f, NULL ) ) {
							nickname = watermark_nickname;
							last_watermark_nickname = nickname;
						}
					}
					checkbox( "media player", &GET_VARIABLE( g_variables.trackdispay, bool ) );
					const char* clantag_modes[] = { "disabled", "default", "animated", "custom", "spotify" };
					combo( "clantag", GET_VARIABLE( g_variables.m_clantag_mode, int ), clantag_modes, IM_ARRAYSIZE( clantag_modes ) );
					if ( GET_VARIABLE( g_variables.m_clantag_mode, int ) == 2 )
						slider_float( "clantag speed", &GET_VARIABLE( g_variables.m_clantag_speed, float ), 0.25f, 4.f, "%.2fx" );
					if ( GET_VARIABLE( g_variables.m_clantag_mode, int ) == 3 ) {
						static char custom_clantag[ 128 ] = "drainware";
						static std::string last_custom_clantag = "drainware";
						auto& custom = GET_VARIABLE( g_variables.m_custom_clantag, std::string );
						if ( custom != last_custom_clantag ) {
							strncpy_s( custom_clantag, sizeof( custom_clantag ), custom.c_str( ), _TRUNCATE );
							last_custom_clantag = custom;
						}

						if ( input_text( "custom clantag", custom_clantag, custom_clantag, IM_ARRAYSIZE( custom_clantag ), 210.f, 20.f, NULL ) ) {
							custom = custom_clantag;
							last_custom_clantag = custom;
						}
					}
					checkbox( "movement chat prints", &GET_VARIABLE( g_variables.m_movement_chat_prints, bool ), false, true, { 200, -1 }, []() {
						checkbox( "edgebug", &GET_VARIABLE( g_variables.m_chat_print_edgebug, bool ) );
						checkbox( "pixelsurf", &GET_VARIABLE( g_variables.m_chat_print_pixelsurf, bool ) );
						checkbox( "new px found", &GET_VARIABLE( g_variables.m_chat_print_new_px, bool ) );
						checkbox( "jumpbug", &GET_VARIABLE( g_variables.m_chat_print_jumpbug, bool ) );
						static char chat_prefix[ 64 ] = "dgware";
						static std::string last_chat_prefix = "dgware";
						auto& prefix = GET_VARIABLE( g_variables.m_chat_print_prefix, std::string );
						if ( prefix != last_chat_prefix ) {
							strncpy_s( chat_prefix, sizeof( chat_prefix ), prefix.c_str( ), _TRUNCATE );
							last_chat_prefix = prefix;
						}
						if ( input_text( "print prefix", chat_prefix, chat_prefix, IM_ARRAYSIZE( chat_prefix ), 210.f, 20.f, NULL ) ) {
							prefix = chat_prefix;
							last_chat_prefix = prefix;
						}
						ImGui::Text( "print color" );
						ImGui::ColorEdit4( "##movement chat print color", &GET_VARIABLE( g_variables.m_chat_print_color, c_color ),
						                   color_picker_alpha_flags );
						const char* chat_color_modes[] = { "normal", "accent mapped", "custom mapped" };
						combo( "chat color mode", GET_VARIABLE( g_variables.m_chat_print_color_mode, int ), chat_color_modes,
						       IM_ARRAYSIZE( chat_color_modes ) );
						checkbox( "mix with accent", &GET_VARIABLE( g_variables.m_chat_print_combine_accent, bool ) );
						slider_float( "print cooldown", &GET_VARIABLE( g_variables.m_chat_print_cooldown, float ), 0.15f, 4.f, "%.2fs" );
					} );
					checkbox( "debug logging", &GET_VARIABLE( g_variables.m_debug_logging, bool ), false, true, { 200, -1 }, []() {
						checkbox( "log particles", &GET_VARIABLE( g_variables.m_debug_log_particles, bool ) );
						checkbox( "log chat prints", &GET_VARIABLE( g_variables.m_debug_log_chat, bool ) );
						checkbox( "log run analyzer", &GET_VARIABLE( g_variables.m_debug_log_run, bool ) );
						checkbox( "log sound suppression", &GET_VARIABLE( g_variables.m_debug_log_sound, bool ) );
						if ( button( "clear debug log", ImVec2( 210, 0 ) ) )
							g_misc.clear_debug_log( );
					} );
					checkbox( "particle debug", &GET_VARIABLE( g_variables.m_debug_logging, bool ), false, true, { 200, -1 }, []() {
						const char* features[] = { "footstep", "edgebug", "bullet tracer" };
						const char* particles[] = { "off", "surface-based", "blood impact", "blood mist", "feathers", "confetti A",
							                     "confetti balloons", "dust devil", "water splash", "baggage drip", "molotov fireball",
							                     "weapon confetti", "engine dust", "engine sparks", "engine smoke", "engine ring beam",
							                     "engine lightning beam", "ambient sparks core", "weapon confetti sparks", "dust devil smoke",
							                     "dust devil swirls", "baggage splash" };
						combo( "debug feature", GET_VARIABLE( g_variables.m_particle_debug_feature, int ), features, IM_ARRAYSIZE( features ) );
						combo( "debug particle", GET_VARIABLE( g_variables.m_particle_debug_particle, int ), particles, IM_ARRAYSIZE( particles ) );
						static char debug_particle_name[ 96 ]{ };
						static std::string last_debug_particle_name;
						auto& custom_particle_name = GET_VARIABLE( g_variables.m_particle_debug_name, std::string );
						if ( custom_particle_name != last_debug_particle_name ) {
							strncpy_s( debug_particle_name, sizeof( debug_particle_name ), custom_particle_name.c_str( ), _TRUNCATE );
							last_debug_particle_name = custom_particle_name;
						}
						if ( input_text( "custom particle", debug_particle_name, debug_particle_name, IM_ARRAYSIZE( debug_particle_name ), 210.f, 20.f, NULL ) ) {
							custom_particle_name = debug_particle_name;
							last_debug_particle_name = custom_particle_name;
						}
						ImGui::TextDisabled( "leave custom empty to use dropdown" );
						if ( button( "test particle at feet", ImVec2( 210, 0 ) ) )
							g_misc.request_particle_debug_test( 0 );
						if ( button( "test particle at crosshair", ImVec2( 210, 0 ) ) )
							g_misc.request_particle_debug_test( 1 );
						if ( button( "test tracer", ImVec2( 210, 0 ) ) )
							g_misc.request_particle_debug_test( 2 );
						if ( button( "validate common CS:GO particles", ImVec2( 210, 0 ) ) )
							g_misc.request_particle_debug_test( 3 );
					} );
					if ( button( "force hud update" ) )
						g_ctx.force_full_update = true;
#ifdef _DEBUG
					static int ctx_panel = 0;
					static char buffer[ 1024 * 16 ];
					input_text( "panorama script text", buffer, buffer, IM_ARRAYSIZE( buffer ), -1, 20, NULL );
					const char* ctx_panel_items[] = { "CSGOHud", "CSGOMainMenu" };
					combo( "context panel", ctx_panel, ctx_panel_items, 1 );
					if ( ImGui::Button( "run panorama script" ) )
						g_scaleform.m_uiengine->run_script( ctx_panel == 0 ? g_scaleform.m_hud_panel : g_scaleform.m_menu_panel, buffer,
						                                    ctx_panel == 0 ? "panorama/layout/hud/hud.xml" : "panorama/layout/mainmenu.xml", 8, 10,
						                                    false, false );
#endif
					
				} );
			
				
			}
		}

		// Вкладка "Movement" (tab_number == 3)
		if (tab_number == 3)
		{
			ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
			child( "main", ImVec2( 230.0f, 375.0f ), []( ) { 
				checkbox( "bunny hop", &GET_VARIABLE( g_variables.m_bunny_hop, bool ) );
				checkbox( "edge jump", &GET_VARIABLE( g_variables.m_edge_jump, bool ), false, true, { 200, -1 }, [](){
					
					checkbox( "on ladders", &GET_VARIABLE( g_variables.m_edge_jump_ladder, bool ) );
					ImGui::Text( "edge jump key" );
					keybind( "edge jump key ", &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) );
					} );
				
				
				checkbox( "long jump", &GET_VARIABLE( g_variables.m_long_jump, bool ), false, true, { 200, -1 },[](){
					ImGui::Text( "long jump key" );
					keybind( "long jump key ", &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) );
					} );
				
				checkbox( "fire man", &GET_VARIABLE( g_variables.m_fire_man, bool ), false, true, { 200, -1 },
				          []( ) 
					{
					ImGui::Text( "fire man key" );
					keybind( "fire man key ", &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) );
					} );
				
				checkbox( "mini jump", &GET_VARIABLE( g_variables.m_mini_jump, bool ), false, true, { 200, -1 },
				          []( ) { 
						
						ImGui::Text( "mini jump key" );
						keybind( "mini jump key ", &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) );
						checkbox( "hold crouch", &GET_VARIABLE( g_variables.m_mini_jump_hold_duck, bool ) );
					
					
					} );
			
				
				
				checkbox( "jump bug", &GET_VARIABLE( g_variables.m_jump_bug, bool ), false, true, { 200, -1 }, [](){ 
					ImGui::Text( "jump bug key" );
					keybind( "jump bug key ", &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) );
					} );
			
				checkbox("edge bug", &GET_VARIABLE(g_variables.edge_bug, bool), false, true, { 200, -1 }, []() {
					
					ImGui::Text( "edge bug key" );
					keybind( "edge bug key", &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) );
					checkbox( "advanced search", &GET_VARIABLE( g_variables.EdgeBugAdvanceSearch, bool ) );
					checkbox( "silent edgebug", &GET_VARIABLE( g_variables.SiletEdgeBug, bool ) );
					checkbox( "extra advanced search", &GET_VARIABLE( g_variables.MegaEdgeBug, bool ) );
					if ( GET_VARIABLE( g_variables.EdgeBugAdvanceSearch, bool ) ) {
						checkbox( "autostrafe to edge", &GET_VARIABLE( g_variables.AutoStrafeEdgeBug, bool ) );
						if ( !GET_VARIABLE( g_variables.AutoStrafeEdgeBug, bool ) ) {
							slider_float( "strafe angle", &GET_VARIABLE( g_variables.deltaScaler, float ), 1.f, 179.f, "%.0f" );
							const char* delta_types[] = { "full", "two-fifths", "half", "quarter" };
							combo( "delta scale", GET_VARIABLE( g_variables.DeltaType, int ), delta_types,4 );
						}
					}

					slider_int( "edgebug ticks", &GET_VARIABLE( g_variables.EdgeBugTicks, int ), 1, 64, "%d" );
					slider_float( "edgebug lock", &GET_VARIABLE( g_variables.EdgeBugMouseLock, float ), 0.0f, 10.0f, "%.2f" );
			

					} );
				
				checkbox( "pixel surf", &GET_VARIABLE( g_variables.m_pixel_surf, bool ), false, true, { 200, -1 },
				          []( ) { 
						ImGui::Text( "pixel surf key" );	
						keybind( "pixel surf key ", &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) );
			
						checkbox( "pixel surf fix", &GET_VARIABLE( g_variables.m_pixel_surf_fix, bool ) ); } );
				
				checkbox( "pixelsurf assistant", &GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ), false, true, { 200, -1 }, []( ) { 
						ImGui::Text( "pixelsurf assist" );
					checkbox( "broke hop", &GET_VARIABLE( g_variables.m_pixel_surf_assist_brokehop, bool ) );
					checkbox( "render pixelsurf assist", &GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) );
					slider_float( "unlock factor", &GET_VARIABLE( g_variables.m_pixel_surf_assist_lockfactor, float ), 0.f, 1.f, "%.0f" );
					slider_int( "predicting ticks", &GET_VARIABLE( g_variables.m_pixel_surf_assist_ticks, int ), 12, 64, "%d" );
					slider_float( "pixelsurf assistant radius", &GET_VARIABLE( g_variables.m_pixel_surf_assist_radius, float ), 40.f, 300.f, "%.0f" );

					ImGui::Text( "pixelsurf assistant: " );
					keybind( "pixelsurf assistant key", &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) );
					ImGui::Text( "pixelsurf assistant point: " );
					keybind( "pixelsurf assistant point key", &GET_VARIABLE( g_variables.m_pixel_surf_assist_point_key, key_bind_t ) );
					ImGui::Text( "pixel finder" );
					keybind( "pixel finder key", &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) );
					const char* pixel_surf_assist_types[] = { "standard", "advanced" };
					combo( "type", GET_VARIABLE( g_variables.m_pixel_surf_assist_type, int ), pixel_surf_assist_types,2 );
					if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_type, int ) > 0 ) {
						slider_float( "delta scale", &GET_VARIABLE( g_variables.m_pixelsurf_assist_delta_scale, float ), 0.f, 1.f, "%.0f" );
					}
					
					} );

				checkbox( "pixelsurf assistant point", &GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ), false, true, { 200, -1 },
					[]( ) { 
						ImGui::Text( "pixelsurf assistant point key" );
						keybind( "pixelsurf assistant point key", &GET_VARIABLE( g_variables.m_pixel_jump_point_key, key_bind_t ) );
					} );
				
				checkbox("bounce assistant", &GET_VARIABLE(g_variables.m_bouncee_assist, bool), false, true, { 200, -1 }, []() {
					ImGui::Text( "bounce assist" );
					checkbox( "broke hop", &GET_VARIABLE( g_variables.m_bouncee_assist_brokehop, bool ) );
					checkbox( "render bounce assist", &GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) );
					ImGui::Text( "bounce assistant: " );
					keybind( "bounce assistant key", &GET_VARIABLE( g_variables.m_bounce_assist_key, key_bind_t ) );
					ImGui::Text( "bounce assistant point: " );
					keybind( "bounce assistant point key", &GET_VARIABLE( g_variables.m_bounce_assist_point_key, key_bind_t ) );
					slider_float( "bounce assistant radius", &GET_VARIABLE( g_variables.m_bouncee_assist_radius, float ), 40.f, 300.f,  "%.0f" );
			

					} );
				

				checkbox( "auto align", &GET_VARIABLE( g_variables.m_auto_align, bool ) );
				checkbox( "no crouch cooldown", &GET_VARIABLE( g_variables.m_no_crouch_cooldown, bool ) );
				checkbox( "auto duck", &GET_VARIABLE( g_variables.m_auto_duck, bool ) );
				checkbox( "ladder bug", &GET_VARIABLE( g_variables.m_ladder_bug, bool ), false, true, { 200, -1 }, []( ) {
					ImGui::Text( "ladder bug key" );
					keybind( "ladder bug key ", &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) );
					} );
				checkbox( "airstuck", &GET_VARIABLE( g_variables.m_air_stuck, bool ), false, true, { 200, -1 }, []( ) { ImGui::Text( "airstuck key" );
				keybind( "##airstuck key ", &GET_VARIABLE( g_variables.m_air_stuck_key, key_bind_t ) );
					} );
				checkbox( "deagle spinner", &GET_VARIABLE( g_variables.m_deagle_spinner, bool ), false, true, { 200, -1 }, []( ) {
					ImGui::Text( "spinner key" );
					keybind( "deagle spinner key", &GET_VARIABLE( g_variables.m_deagle_spinner_key, key_bind_t ) );
					const char* spinner_modes[] = { "hold", "toggle" };
					combo( "spinner mode", GET_VARIABLE( g_variables.m_deagle_spinner_mode, int ), spinner_modes, IM_ARRAYSIZE( spinner_modes ) );
					slider_float( "spinner speed", &GET_VARIABLE( g_variables.m_deagle_spinner_speed, float ), 0.35f, 2.5f, "%.2fx" );
				} );
				checkbox( "blockbot", &GET_VARIABLE( g_variables.m_blockbot, bool ), false, true, { 200, -1 }, []( ) {
					ImGui::Text( "blockbot key" );
					keybind( "blockbot key", &GET_VARIABLE( g_variables.m_blockbot_key, key_bind_t ) );
					const char* target_modes[] = { "nearest", "crosshair" };
					combo( "target mode", GET_VARIABLE( g_variables.m_blockbot_target_mode, int ), target_modes, IM_ARRAYSIZE( target_modes ) );
					const char* team_filters[] = { "all players", "enemies", "teammates" };
					combo( "team filter", GET_VARIABLE( g_variables.m_blockbot_team_filter, int ), team_filters, IM_ARRAYSIZE( team_filters ) );
					slider_float( "strength", &GET_VARIABLE( g_variables.m_blockbot_strength, float ), 0.1f, 1.f, "%.2f" );
					slider_float( "smoothing", &GET_VARIABLE( g_variables.m_blockbot_smoothing, float ), 0.f, 1.f, "%.2f" );
				} );
				checkbox( "edgebug particles", &GET_VARIABLE( g_variables.m_edgebug_particles, bool ), false, true, { 200, -1 }, []() {
					const char* particle_modes[] = { "off", "surface-based", "blood impact", "blood mist", "feathers", "confetti A",
						                             "confetti balloons", "dust devil", "water splash", "baggage drip", "molotov fireball",
						                             "weapon confetti", "engine dust", "engine sparks", "engine smoke", "engine ring beam",
						                             "engine lightning beam", "ambient sparks core", "weapon confetti sparks", "dust devil smoke",
						                             "dust devil swirls", "baggage splash" };
					combo( "edgebug particle", GET_VARIABLE( g_variables.m_edgebug_particle_type, int ), particle_modes, IM_ARRAYSIZE( particle_modes ) );
					if ( GET_VARIABLE( g_variables.m_edgebug_particle_type, int ) > 1 && GET_VARIABLE( g_variables.m_edgebug_particle_type, int ) < 12 )
						ImGui::TextDisabled( "Named CS:GO particles log precache/dispatch results." );
					slider_float( "edgebug intensity", &GET_VARIABLE( g_variables.m_edgebug_particle_intensity, float ), 0.25f, 3.f, "%.2fx" );
					slider_float( "edgebug cooldown", &GET_VARIABLE( g_variables.m_edgebug_particle_cooldown, float ), 0.18f, 2.f, "%.2fs" );
				} );
				checkbox( "px database", &GET_VARIABLE( g_variables.m_px_database, bool ), false, true, { 200, -1 }, []( ) {
					ImGui::TextWrapped( "Auto-records real pixelsurf runs and merges same-edge straight lines per map." );
					checkbox( "auto-record", &GET_VARIABLE( g_variables.m_px_database_auto_record, bool ) );
					checkbox( "current map only", &GET_VARIABLE( g_variables.m_px_database_current_map_only, bool ) );
					checkbox( "use menu accent", &GET_VARIABLE( g_variables.m_px_database_use_accent, bool ) );
					if ( !GET_VARIABLE( g_variables.m_px_database_use_accent, bool ) ) {
						ImGui::Text( "px line color" );
						ImGui::ColorEdit4( "##px database color", &GET_VARIABLE( g_variables.m_px_database_color, c_color ), color_picker_alpha_flags );
					}
					slider_float( "line thickness", &GET_VARIABLE( g_variables.m_px_database_thickness, float ), 1.f, 6.f, "%.1f" );
					slider_float( "distance limit", &GET_VARIABLE( g_variables.m_px_database_distance, float ), 120.f, 8000.f, "%.0f u" );
					checkbox( "draw through walls", &GET_VARIABLE( g_variables.m_px_database_draw_through_walls, bool ) );
					if ( button( "delete nearest px line", ImVec2( 210, 0 ) ) )
						g_misc.request_px_delete_nearest( );
					if ( button( "clear current map px", ImVec2( 210, 0 ) ) )
						g_misc.request_px_clear_current_map( );
				} );
				
				} );

			ImGui::SetCursorPos( ImVec2( 150.0f + 240.f, 10.0f ) );
			child( "indicators", ImVec2( 230.0f, 375.0f ), []( ) {
				
				checkbox("velocity indicator", &GET_VARIABLE(g_variables.m_velocity_indicator, bool), false, true, { 200, -1 }, []() {
					
					checkbox( "show pre speed ", &GET_VARIABLE( g_variables.m_velocity_indicator_show_pre_speed, bool ) );
					checkbox( "fade alpha ", &GET_VARIABLE( g_variables.m_velocity_indicator_fade_alpha, bool ) );
					checkbox( "custom color   ", &GET_VARIABLE( g_variables.m_velocity_indicator_custom_color, bool ) );
					checkbox( "rgb velocity", &GET_VARIABLE( g_variables.m_velocity_indicator_rgb, bool ) );
					if ( GET_VARIABLE( g_variables.m_velocity_indicator_rgb, bool ) )
						slider_float( "rgb speed", &GET_VARIABLE( g_variables.m_velocity_indicator_rgb_speed, float ), 0.05f, 2.5f, "%.2fx" );
					slider_int( "padding  ", &GET_VARIABLE( g_variables.m_velocity_indicator_padding, int ), 5, 100, "%d%%" );
					ImGui::Spacing( );
					
					if ( GET_VARIABLE( g_variables.m_velocity_indicator_custom_color, bool ) ) {
						ImGui::Text( "velocity colors start" );
						ImGui::ColorEdit4( "##velocity indicator color 1", &GET_VARIABLE( g_variables.m_velocity_indicator_color1, c_color ),
						                   color_picker_alpha_flags );
					
						ImGui::Text( "velocity colors end" );
						ImGui::ColorEdit4( "##velocity indicator color 2", &GET_VARIABLE( g_variables.m_velocity_indicator_color2, c_color ),
						                   color_picker_alpha_flags );
					} else {
						ImGui::Text( "velocity colors up" );
						ImGui::ColorEdit4( "##velocity indicator color 3", &GET_VARIABLE( g_variables.m_velocity_indicator_color3, c_color ),
						                   color_picker_alpha_flags );
					
						ImGui::Text( "velocity colors down" );
						ImGui::ColorEdit4( "##velocity indicator color 4", &GET_VARIABLE( g_variables.m_velocity_indicator_color4, c_color ),
						                   color_picker_alpha_flags );
					
						ImGui::Text( "velocity colors equeal" );
						ImGui::ColorEdit4( "##velocity indicator color 5", &GET_VARIABLE( g_variables.m_velocity_indicator_color5, c_color ),
						                   color_picker_alpha_flags );
					}
					} );

				checkbox("stamina indicator", &GET_VARIABLE(g_variables.m_stamina_indicator, bool), false, true, { 200, -1 }, []() {
					
					checkbox( "show pre speed	", &GET_VARIABLE( g_variables.m_stamina_indicator_show_pre_speed, bool ) );
					checkbox( "fade alpha	", &GET_VARIABLE( g_variables.m_stamina_indicator_fade_alpha, bool ) );
					slider_int( "padding  ", &GET_VARIABLE( g_variables.m_stamina_indicator_padding, int ), 5, 100, "%d%%" );
					ImGui::Spacing( );
					ImGui::Text( "stamina colors start" );
					
					ImGui::ColorEdit4( "##stamina indicator color 1", &GET_VARIABLE( g_variables.m_stamina_indicator_color1, c_color ),
					                   color_picker_alpha_flags );
				
					ImGui::Text( "stamina colors end" );
					ImGui::ColorEdit4( "##stamina indicator color 2", &GET_VARIABLE( g_variables.m_stamina_indicator_color2, c_color ),
					                   color_picker_alpha_flags );
					} );
				
				checkbox( "keybind indicators", &GET_VARIABLE( g_variables.m_key_indicators_enable, bool ), false, true, { 200, -1 }, [](){
					
					
					slider_int( "position  ", &GET_VARIABLE( g_variables.m_key_indicators_position, int ), 30, g_ctx.m_height, "%d" );
					const char* keybind_indicators[] = { "edgebug",         "pixelsurf",      "edgejump", "longjump",  "minijump",
						                                 "jumpbug", "pixelsurf assist", "bounce assist", "fireman",  "ladderbug" };
					multi_combo( "displayed keybinds", GET_VARIABLE( g_variables.m_key_indicators, std::vector< bool > ), keybind_indicators,
					                   10 );
					const char* ind_type[] = { "vertical", "horisontal" };
					combo( "keybind indicator type", GET_VARIABLE( g_variables.m_key_indicators_type, int ), ind_type, 2 );
					checkbox( "success color toggle", &GET_VARIABLE( g_variables.m_key_indicators_success_toggle, bool ) );
					ImGui::Spacing( );
					ImGui::Text( "keybind colors" );
					ImGui::ColorEdit4( "##keybind color", &GET_VARIABLE( g_variables.m_key_color, c_color ), color_picker_alpha_flags );
					ImGui::Text( "keybind success colors" );
					ImGui::ColorEdit4( "##keybind success color", &GET_VARIABLE( g_variables.m_key_color_success, c_color ),
					                   color_picker_alpha_flags );
					} );
				checkbox( "s0beit bind overlay", &GET_VARIABLE( g_variables.m_bind_overlay, bool ), false, true, { 200, -1 }, []() {
					const char* overlay_positions[] = { "bottom center", "bottom left", "bottom right" };
					combo( "overlay position", GET_VARIABLE( g_variables.m_bind_overlay_position, int ), overlay_positions,
					       IM_ARRAYSIZE( overlay_positions ) );
					const char* overlay_styles[] = { "s0beit", "fakedrain", "minimal" };
					combo( "overlay style", GET_VARIABLE( g_variables.m_bind_overlay_style, int ), overlay_styles, IM_ARRAYSIZE( overlay_styles ) );
					checkbox( "use accent", &GET_VARIABLE( g_variables.m_bind_overlay_use_accent, bool ) );
					if ( !GET_VARIABLE( g_variables.m_bind_overlay_use_accent, bool ) ) {
						ImGui::Text( "overlay color" );
						ImGui::ColorEdit4( "##bind overlay color", &GET_VARIABLE( g_variables.m_bind_overlay_color, c_color ),
						                   color_picker_alpha_flags );
					}
					slider_float( "background alpha", &GET_VARIABLE( g_variables.m_bind_overlay_background_alpha, float ), 0.f, 1.f, "%.2f" );
					slider_float( "animation speed", &GET_VARIABLE( g_variables.m_bind_overlay_animation_speed, float ), 0.2f, 4.f, "%.2fx" );
					slider_float( "font scale", &GET_VARIABLE( g_variables.m_bind_overlay_font_scale, float ), 0.75f, 1.5f, "%.2fx" );
				} );
				

			} );
		}

		// Вкладка "Inventory" (tab_number == 4)

		if ( tab_number == 4 ) {
			ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
			child( "main", ImVec2( 460.0f, 450.0f ), []( ) {
				static int selectedAgentTeam  = 0;       // Выбранная команда агента
				static int16_t selectedWeapon = 0;       // Выбранное оружие
				static WeaponItem_t* selectedWeaponSkin = nullptr; // Выбранный скин оружия
				static InventoryItem_t inventory_list;   // Текущий предмет для настройки
				static int selectedPage = 0;             // Текущая страница

				// Страница 0: Список инвентаря
				if ( selectedPage == 0 ) {
					

					ImGui::SetCursorPosX( 10.f );
					if ( button( "Add New Item",ImVec2(440,20) )) {
						selectedPage = 1; // Переход на страницу добавления
					}
					
					if ( button( "Remove all items", ImVec2( 440, 20 ) ) ) {
						IPlayerInventory* m_pInventory = g_interfaces.m_inventory_manager ? g_interfaces.m_inventory_manager->GetLocalPlayerInventory( ) : nullptr;
						if ( m_pInventory ) {
							std::vector< uint64_t > itemsToDelete;
							for ( const auto& [ itemID, itemData ] : g_invetory_changer.m_aInventoryItemList ) {
								itemsToDelete.push_back( itemID );
							}
							for ( auto itemID : itemsToDelete ) {
								C_EconItemView* pEconItemView = m_pInventory->GetInventoryItemByItemID( itemID );
								if ( pEconItemView && pEconItemView->GetSocData( ) ) {
									m_pInventory->RemoveItem( pEconItemView->GetSocData( ) );
									g_invetory_changer.m_aInventoryItemList.erase( itemID );
								}
							}
							g_ctx.force_full_update = true;
						}
					}

					ImGui::SetCursorPosY( 43.f );
					ImGui::BeginChild( "inventory_list", ImVec2( 440.f, 380.f ) );{
						ImGui::SetCursorPosX( 1.f );
						int next_space = 0;
						for ( const auto& [ itemID, itemData ] : g_invetory_changer.m_aInventoryItemList ) {
							if ( next_space == 0 )
								ImGui::SetCursorPosX( 1.f );
							InventoryItem( itemData );

							if ( ImGui::IsItemHovered( ) && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) ) {
								IPlayerInventory* m_pInventory = g_interfaces.m_inventory_manager ? g_interfaces.m_inventory_manager->GetLocalPlayerInventory( ) : nullptr;
								if ( m_pInventory ) {
									C_EconItemView* pEconItemView = m_pInventory->GetInventoryItemByItemID( itemID );
									if ( pEconItemView && pEconItemView->GetSocData( ) ) {
										m_pInventory->RemoveItem( pEconItemView->GetSocData( ) );
										g_invetory_changer.m_aInventoryItemList.erase( itemID );

										g_ctx.force_full_update = true;
										break;
									}
								}
							}

							float lastItemX2 = ImGui::GetItemRectMax( ).x;
							float nextItemX2 = lastItemX2 + 8.0f + 140.0f;
							if ( next_space < 2 ) {
								ImGui::SameLine( );
								next_space++;
							} else {
								next_space = 0;
							}
						}
					}
					ImGui::EndChild( );
				}

				// Страница 1: Добавление нового предмета
				if ( selectedPage == 1 ) {
					int next_space = 0;
					ImGui::BeginChild( "##secpage", { -1, -1 } );
					{
						ImGui::Text( "weapons" );

						{
							int item_index         = 0;
							const float item_width = 140.f;
							const float spacing    = 8.f;
							int items_per_row =
								std::max( 1, static_cast< int >( ( ImGui::GetContentRegionAvail( ).x - spacing ) / ( item_width + spacing ) ) );
							ImGui::SetCursorPosX( 1.f );

							for ( const auto& weapon : g_items_manager.m_vDefaults ) {
								if ( next_space == 0 )
									ImGui::SetCursorPosX( 1.f );
								if ( InventoryItemDefault( weapon ) ) {
									selectedWeapon = weapon->m_nItemID;
									selectedPage   = 2; // Переход к выбору скина
								}
								item_index++;
								if ( next_space < 2 ) {
									ImGui::SameLine( );
									next_space++;
								} else {
									next_space = 0;
								}
							}
						}

						ImGui::Text( "gloves" );

						{
							const float item_width = 140.f;
							const float spacing    = 8.f;
							int items_per_row =
								std::max( 1, static_cast< int >( ( ImGui::GetContentRegionAvail( ).x - spacing ) / ( item_width + spacing ) ) );
							next_space     = 0;
							int item_index = 0;
							ImGui::SetCursorPosX( 1.f );
							for ( auto i = 0; i < g_items_manager.m_vGloves.size( ); i++ ) {
								if ( next_space == 0 )
									ImGui::SetCursorPosX( 1.f );
								if ( InventoryItemGlove( g_items_manager.m_vGloves[ i ] ) ) {
									inventory_list.m_nItemID   = g_items_manager.m_vGloves[ i ]->m_nItemID;
									inventory_list.m_iPaintKit = g_items_manager.m_vGloves[ i ]->m_nKitID;
									inventory_list.SkinName    = LocalizeTex2( g_items_manager.m_vGloves[ i ]->m_szName.c_str( ) );
									if ( g_interfaces.m_inventory_manager )
										g_invetory_changer.AddItemToInventory( g_interfaces.m_inventory_manager->GetLocalPlayerInventory( ),
										                                       inventory_list );
									inventory_list = InventoryItem_t( );
									selectedPage   = 0; // Возврат к списку
								}
								item_index++;
								if ( next_space < 2 ) {
									ImGui::SameLine( );
									next_space++;
								} else {
									next_space = 0;
								}
							}
						}
						{
							ImGui::Text( "agents" );

							int item_index         = 0;
							const float item_width = 140.f;
							const float spacing    = 8.f;
							int items_per_row =
								std::max( 1, static_cast< int >( ( ImGui::GetContentRegionAvail( ).x - spacing ) / ( item_width + spacing ) ) );
							next_space = 0;
							ImGui::SetCursorPosX( 1.f );
							for ( auto i = 0; i < g_items_manager.m_vAgents.size( ); i++ ) {
								
								if ( i < 2 ) {
									if ( InventoryItemAgent( g_items_manager.m_vAgents[ i ] ) ) {
										selectedPage      = 5; // Переход к выбору агента
										selectedAgentTeam = i;
									}
									ImGui::SameLine( );
									item_index++;
								}
							}
						}
					}
					ImGui::EndChild( );
				}

				// Страница 5: Выбор агента
				if ( selectedPage == 5 ) {
					

				
					ImGui::BeginChild( "##fithpage", { -1, -1 } );
					{
						int item_index         = 0;
						const float item_width = 140.f;
						const float spacing    = 8.f;
						int items_per_row =
							std::max( 1, static_cast< int >( ( ImGui::GetContentRegionAvail( ).x - spacing ) / ( item_width + spacing ) ) );
						ImGui::SetCursorPosX( 1.f );
						int next_space = 0;
						for ( auto i = 2; i < g_items_manager.m_vAgents.size( ); i++ ) {
							if ( g_items_manager.m_vAgents[ i ]->m_iTeamID == selectedAgentTeam ) {
								if ( next_space == 0 )
									ImGui::SetCursorPosX( 1.f );
								if ( InventoryItemAgent( g_items_manager.m_vAgents[ i ] ) ) {
									inventory_list.m_nItemID       = g_items_manager.m_vAgents[ i ]->m_nItemID;
									inventory_list.m_iPaintKit     = 0;
									inventory_list.m_iRarity       = 0;
									inventory_list.m_flPearlescent = 0;
									inventory_list.ct_team         = selectedAgentTeam;
									inventory_list.SkinName        = LocalizeTex2( g_items_manager.m_vAgents[ i ]->m_szName.c_str( ) );
									if ( g_interfaces.m_inventory_manager )
										g_invetory_changer.AddItemToInventory( g_interfaces.m_inventory_manager->GetLocalPlayerInventory( ),
										                                       inventory_list );
									inventory_list = InventoryItem_t( );
									selectedPage   = 0; // Возврат к списку
								}
								item_index++;
								if ( next_space < 2 ) {
									ImGui::SameLine( );
									next_space++;
								} else {
									next_space = 0;
								}
							}
						}
					} 
					ImGui::EndChild( );
				}

				// Страница 2: Выбор скина для оружия
				if ( selectedPage == 2 ) {
					ImGui::BeginChild( "##secondpage", { -1, -1 } );
					int next_space = 0;
					 {
						int item_index         = 0;
						const float item_width = 140.f;
						const float spacing    = 8.f;
						int items_per_row =
							std::max( 1, static_cast< int >( ( ImGui::GetContentRegionAvail( ).x - spacing ) / ( item_width + spacing ) ) );
						ImGui::SetCursorPosX( 1.f );
						for ( const auto& weapon : g_items_manager.m_vWeapons ) {
							if ( weapon->m_tItem->m_nID != selectedWeapon )
								continue;
							if ( next_space == 0 )
								ImGui::SetCursorPosX( 1.f );
							if ( InventoryItem( weapon ) ) {
								selectedWeaponSkin             = weapon;
								selectedPage                   = 3; // Переход к настройке
								inventory_list.m_flPearlescent = weapon->m_tPaintKit->m_flPearcelent;
								for ( size_t i = 0; i < 4; i++ ) {
									inventory_list.color[ i ][ 0 ] = float_t( weapon->m_tPaintKit->m_cColor[ i ].get_colors( ).at( 0 ) ) / 255.f;
									inventory_list.color[ i ][ 1 ] = float_t( weapon->m_tPaintKit->m_cColor[ i ].get_colors( ).at( 1 ) ) / 255.f;
									inventory_list.color[ i ][ 2 ] = float_t( weapon->m_tPaintKit->m_cColor[ i ].get_colors( ).at( 2 ) ) / 255.f;
								}
							}
							item_index++;
							if ( next_space < 2 ) {
								ImGui::SameLine( );
								next_space++;
							} else {
								next_space = 0;
							}
						}
					} 
					 ImGui::EndChild( );
				}

				// Страница 3: Настройка скина
				if ( selectedPage == 3 ) {
					if ( !selectedWeaponSkin ) {
						ImGui::TextWrapped( "No skin selected. Go back and pick a paint kit." );
						if ( button( "back", ImVec2( 210, 0 ) ) )
							selectedPage = 2;
						return;
					}
					ImGui::SetCursorPos( ImVec2( 10.0f, 10.0f ) );
					child( "preview", ImVec2( 200.f, 380.f ), []( ) {
						ImGuiWindow* window = ImGui::GetCurrentWindow( );
						ImVec2 pos          = window->DC.CursorPos;
						ImVec2 size         = ImVec2( 200.f, 167.f );
						if ( selectedWeaponSkin && selectedWeaponSkin->m_tTexture )
							window->DrawList->AddImage( selectedWeaponSkin->m_tTexture, pos, pos + size );
					} );

					ImGui::SetCursorPos( ImVec2( 215.0f, 10.0f ) );
					child( "settings", ImVec2( 230.f, 380.f ), []( ) {
						

						
						checkbox( "stattrak", &inventory_list.m_StatTrack.enabled );
						if ( inventory_list.m_StatTrack.enabled ) {
							slider_int( "stattrak Count", &inventory_list.m_StatTrack.counter, 0, 10000, "%d" );
						}
						slider_float( "wear", &inventory_list.m_flWear, 0.0f, 1.0f, "%.2f" );
						slider_float( "seed", &inventory_list.m_flSeed, 0, 1000, "%.2f" );
						slider_int( "quality", &inventory_list.m_iQuality, 0, 12, "%d" );

						checkbox( "custom rarity", &inventory_list.m_CustomRarity.enabled );
						if ( inventory_list.m_CustomRarity.enabled ) {
							slider_int( "rarity", &inventory_list.m_CustomRarity.counter, 0, 7, "%d" );
						}

						

						checkbox( "auto equip ct", &inventory_list.auto_equipment_ct );
						checkbox( "auto equip t", &inventory_list.auto_equipment_t );
						checkbox( "custom tag", &inventory_list.m_Customtag.enabled );
						if ( inventory_list.m_Customtag.enabled ) {
							ImGui::Spacing( );
							ImGui::Spacing( );
							input_text( "tag", inventory_list.m_Customtag.aTagName, inventory_list.m_Customtag.aTagName, 64, -1, 20, NULL );
							inventory_list.SkinName = std::string( inventory_list.m_Customtag.aTagName ).c_str( );
						} else {
							inventory_list.SkinName = LocalizeTex2( selectedWeaponSkin->m_tPaintKit->m_nDescriptionTag ).c_str( );
						}
						if ( button( "add to inventory" ) ) {
							inventory_list.m_nItemID   = selectedWeaponSkin->m_tItem->m_nID;
							inventory_list.m_iPaintKit = selectedWeaponSkin->m_tPaintKit->m_nID;
							inventory_list.m_iRarity   = selectedWeaponSkin->m_tPaintKit->m_nRarity;
							if ( g_interfaces.m_inventory_manager )
								g_invetory_changer.AddItemToInventory( g_interfaces.m_inventory_manager->GetLocalPlayerInventory( ), inventory_list );
							inventory_list                            = InventoryItem_t( );
							selectedWeaponSkin                        = nullptr;
							selectedPage                              = 0; // Возврат к списку
							g_ctx.force_full_update                   = true;
						}
					} );
				}
			} );
		}

		// Вкладка "Font" (tab_number == 5)

		if ( tab_number == 5 ) {
			ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
			child( "fonts", ImVec2(230.0f, 410.0f ), []( ) {
				
				
				
				const char* font_flags[] = { "no hinting",  "no autohint", "force autohint", "light hinting",
					                         "monohinting", "bold",        "oblique",        "monochrome" };
			
				
				ImGui::Spacing( );
				ImGui::Spacing( );
				ImGui::Spacing( );
				static ImGuiTextFilter indicator_font_search_filter{ };
				indicator_font_search_filter.Draw( "search for a font", 210.f );
				float item_height = ImGui::GetTextLineHeightWithSpacing( );
				ImVec2 size       = ImVec2( 237, item_height * 5 ); // Ширина 0 означает использование доступной ширины
				ImGui::SetCursorPosX( -8.f );
				if ( ImGui::BeginListBox( "##font list", ImVec2( size ) ) ) {
					for ( const auto iterator : g_fonts.m_font_file_names ) {
						if ( indicator_font_search_filter.PassFilter( iterator.c_str( ) ) )
							if ( ImGui::Selectable( iterator.c_str( ), HASH_RT( GET_VARIABLE( g_variables.m_indicator_font_settings, font_setting_t )
							                                                        .m_name.c_str( ) ) == HASH_RT( iterator.c_str( ) ) ) )
								GET_VARIABLE( g_variables.m_indicator_font_settings, font_setting_t ).m_name = iterator;
					}
					ImGui::EndListBox( );
				}
			
				ImGui::SetCursorPosX( 10.f );
				if ( button( "reload fonts" ,ImVec2(210,0) ))
					g_render.m_reload_fonts = true;
				ImGui::SetCursorPosX( 10.f );
				if ( button( "reset to default  ", { 210, 0 } ) ) {
					g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ] =
						g_render.m_fonts[ e_font_names::font_name_indicator_29 ];
				}
				
				
				multi_combo( "font flags ", GET_VARIABLE( g_variables.m_indicator_font_flags, std::vector< bool > ), font_flags,
				             GET_VARIABLE( g_variables.m_indicator_font_flags, std::vector< bool > ).size( ), ImVec2( 210, 40 ) );
			

				slider_int( "size	", &GET_VARIABLE( g_variables.m_indicator_font_settings, font_setting_t ).m_size, 0, 50, "%d",210.f );

			} );
		}

		// Вкладка "Config" (tab_number == 6)
		if (tab_number == 6)
		{
			ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
			child( "settings", ImVec2( 230.0f, 410.0f ), []( ) { 
				ImGui::Spacing( );
				ImGui::Spacing( );
				input_text( "config file name", g_menu.m_config_file, g_menu.m_config_file, sizeof( g_menu.m_config_file ),210.f,20.f,NULL );
				std::string converted_file_name         = g_menu.m_config_file;
				static std::string selected_config_name = { };
			
			
				float item_height = ImGui::GetTextLineHeightWithSpacing( );
				ImVec2 size       = ImVec2( 237, item_height * 5 ); // Ширина 0 означает использование доступной ширины
				ImGui::SetCursorPosX( -8.f );
				if ( ImGui::BeginListBox( "##config list", size ) ) {
					for ( size_t i = 0; i < g_config.m_file_names.size( ); i++ ) {
						bool is_selected = ( g_menu.m_selected_config == i );
						if ( ImGui::Selectable( g_config.m_file_names[ i ].c_str( ), is_selected ) )
							g_menu.m_selected_config = i;
					}
					ImGui::EndListBox( );
				}
				selected_config_name = !g_config.m_file_names.empty( ) ? g_config.m_file_names[ g_menu.m_selected_config ] : "";
				ImGui::SetCursorPosX( 10.f );
				if ( button( "create",ImVec2(210,0)) ) {
					if ( !g_config.save( converted_file_name ) )
						g_console.print( std::vformat( "failed to create {:s}", std::make_format_args( converted_file_name ) ).c_str( ) );
					else
						g_logger.print( std::vformat( "created config {:s}", std::make_format_args( converted_file_name ) ).c_str( ) );
					converted_file_name.clear( );
					g_config.refresh( );
				}
				static bool open_save_popup = false;
				ImGui::SetCursorPosX( 10.f );
				if ( button( "save", ImVec2( 210, 0 ) ) )
					open_save_popup = true;
				if ( open_save_popup ) {
					save_popup( "save confirmation", ImVec2( 220.f, -1.f ), [ & ]( ) {
						if ( button( "yes" ) ) {
							saveConfig( n_branding::k_points_file );
							saveConfig2( n_branding::k_bpoints_file );
							SaveInventory( g_invetory_changer.m_aInventoryItemList, n_branding::k_inventory_file );
							g_aimbot.save( );
							if ( !g_config.save( selected_config_name ) )
								g_console.print( std::vformat( "failed to save {:s}", std::make_format_args( selected_config_name ) ).c_str( ) );
							else
								g_logger.print( std::vformat( "saved config {:s}", std::make_format_args( selected_config_name ) ).c_str( ) );
							open_save_popup = false;
						}
						
						if ( button( "no" ) ) {
							g_logger.print( std::vformat( "canceled saving config {:s}", std::make_format_args( selected_config_name ) ).c_str( ) );
							open_save_popup = false;
						}
					} );
				}
				ImGui::SetCursorPosX( 10.f );
				if ( button( "load", ImVec2( 210, 0 ) ) ) {
					if ( !g_config.load( selected_config_name ) )
						g_console.print( std::vformat( "failed to load {:s}", std::make_format_args( selected_config_name ) ).c_str( ) );
					else {
						g_logger.print( std::vformat( "loaded config {:s}", std::make_format_args( selected_config_name ) ).c_str( ) );
						g_render.m_reload_fonts = true;
					}
					loadConfig( n_branding::k_points_file );
					loadConfig2( n_branding::k_bpoints_file );
					g_aimbot.load( );
					std::unordered_map< uint64_t, InventoryItem_t > inventoryConfig;
					// Например, загрузка конфигурации
					if ( g_interfaces.m_inventory_manager && LoadInventory( inventoryConfig, n_branding::k_inventory_file ) ) {
						g_invetory_changer.m_aInventoryItemList.clear( );
						// Проходим по всем элементам конфигурации
						for ( auto& [ key, item ] : inventoryConfig ) {
							// Добавляем скин в инвентарь
							g_invetory_changer.AddItemToInventory( g_interfaces.m_inventory_manager->GetLocalPlayerInventory( ), item );

							// Обнуляем переменную для скина, если требуется сброс
							item = InventoryItem_t( );
						}
					}
				}
				ImGui::SetCursorPosX( 10.f );
				if ( button( "remove", ImVec2( 210, 0 ) ) ) {
					g_logger.print( std::vformat( "removed config {:s}", std::make_format_args( selected_config_name ) ).c_str( ) );
					g_config.remove( g_menu.m_selected_config );
					g_menu.m_selected_config = 0;
				}
				ImGui::SetCursorPosX( 10.f );
				if ( button( "refresh", ImVec2( 210, 0 ) ) )
					g_config.refresh( );
				ImGui::SetCursorPosX( 10.f );
				if ( button( "connect to server", ImVec2( 210, 0 ) ) )
					g_menu.connect_to_server = true;
					
				
				
				} );
		}

		if ( tab_number == 7 ) {
			ImGui::SetCursorPos( ImVec2( 150.0f, 10.0f ) );
			child( "lua", ImVec2( 230.0f, 410.0f ), []( ) {
				const char* watermark_modes[] = { "off", "drainware", "clarity", "rgb / chroma" };
				combo( "watermark", GET_VARIABLE( g_variables.m_watermark_mode, int ), watermark_modes, IM_ARRAYSIZE( watermark_modes ) );
				const char* watermark_accent_modes[] = { "use menu accent", "custom" };
				combo( "watermark accent##lua", GET_VARIABLE( g_variables.m_watermark_accent_mode, int ), watermark_accent_modes,
				       IM_ARRAYSIZE( watermark_accent_modes ) );
				if ( GET_VARIABLE( g_variables.m_watermark_accent_mode, int ) == 1 ) {
					ImGui::Text( "watermark accent color" );
					ImGui::ColorEdit4( "##watermark custom accent lua", &GET_VARIABLE( g_variables.m_watermark_custom_accent, c_color ),
					                   color_picker_alpha_flags );
				}
				slider_float( "watermark alpha##lua", &GET_VARIABLE( g_variables.m_watermark_background_alpha, float ), 0.15f, 1.f, "%.2f" );
				if ( GET_VARIABLE( g_variables.m_watermark_mode, int ) == 3 ) {
					slider_float( "chroma speed##lua", &GET_VARIABLE( g_variables.m_watermark_chroma_speed, float ), 0.15f, 4.f, "%.2fx" );
					slider_float( "chroma strength##lua", &GET_VARIABLE( g_variables.m_watermark_chroma_strength, float ), 0.f, 1.f, "%.2f" );
				}
				static char watermark_nickname_lua[ 64 ] = "verionfe";
				static std::string last_watermark_nickname_lua = "verionfe";
				auto& nickname = GET_VARIABLE( g_variables.m_watermark_nickname, std::string );
				if ( nickname != last_watermark_nickname_lua ) {
					strncpy_s( watermark_nickname_lua, sizeof( watermark_nickname_lua ), nickname.c_str( ), _TRUNCATE );
					last_watermark_nickname_lua = nickname;
				}
				if ( input_text( "watermark nickname##lua", watermark_nickname_lua, watermark_nickname_lua, IM_ARRAYSIZE( watermark_nickname_lua ),
				                 210.f, 20.f, NULL ) ) {
					nickname = watermark_nickname_lua;
					last_watermark_nickname_lua = nickname;
				}
				checkbox( "media player", &GET_VARIABLE( g_variables.trackdispay, bool ) );
			} );
		}

		ImGui::PopFont( );
	}
	ImGui::End( );
}
