#pragma once
#include "players.h"
#include "../../../game/sdk/classes/c_csgo_hud_radar.h"
#include "../../../game/sdk/includes/includes.h"
#include "../../../globals/includes/includes.h"
#include "../../avatar_cache/avatar_cache.h"
#include "../../entity_cache/entity_cache.h"
#include "dormancy/dormancy.h"
#include "../../../globals/render/draw.h"
#include "../../../instalisation fonts/fonts2.h"
#include <algorithm>
#include <deque>
#include <vector>

namespace
{
	struct damage_indicator_t {
		int index = 0;
		int damage = 0;
		float time = 0.f;
	};

	std::deque< damage_indicator_t > g_damage_indicators;
}

void n_players::impl_t::on_paint_traverse( )
{
	if ( GET_VARIABLE( g_variables.m_players, bool ) || GET_VARIABLE( g_variables.esp_enable, bool ) )
		this->players( );
	
}

void n_players::impl_t::add_damage_indicator( int index, int damage )
{
	if ( index <= 0 || index >= 65 || damage <= 0 || !g_interfaces.m_global_vars_base )
		return;

	g_damage_indicators.push_back( { index, damage, g_interfaces.m_global_vars_base->m_real_time } );
	while ( g_damage_indicators.size( ) > 24U )
		g_damage_indicators.pop_front( );
}

constexpr float anim_time = 255 / 2.0f;
void n_players::impl_t::players( )
{
	// dormancy-related
	static unsigned long radar_base{ };
	static c_csgo_hud_radar* hud_radar{ };

	[]( ) {
		g_dormancy.think( );

		// gonna be used later on.

		// static auto find_hud_element_ptr = reinterpret_cast< DWORD( __thiscall* )( void*, const char* ) >(
		// 	g_modules[ CLIENT_DLL ].find_pattern( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) );
		//
		// static auto hud_ptr = *reinterpret_cast< DWORD** >( g_modules[ CLIENT_DLL ].find_pattern( "81 25 ? ? ? ? ? ? ? ? 8B 01" ) + 0x2 );
		//
		// radar_base = find_hud_element_ptr( hud_ptr, "CCSGO_HudRadar" );
		// hud_radar  = reinterpret_cast< c_csgo_hud_radar* >( radar_base - 0x14 );
	}( );

	const float delta_time = 1.f * g_interfaces.m_global_vars_base->m_frame_time;

	c_angle view_angles = { };
	g_interfaces.m_engine_client->get_view_angles( view_angles );

	const float yaw = deg2rad( view_angles.m_y );

	const auto display_size = ImGui::GetIO( ).DisplaySize;

	g_damage_indicators.erase( std::remove_if( g_damage_indicators.begin( ), g_damage_indicators.end( ),
	                                           [ & ]( const damage_indicator_t& indicator ) {
		                                           return g_interfaces.m_global_vars_base->m_real_time - indicator.time > 1.35f;
	                                           } ),
	                           g_damage_indicators.end( ) );

	const int entity_type = std::clamp( GET_VARIABLE( g_variables.m_players_entity_type, int ), 0, 2 );

	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || entity == g_ctx.m_local || !entity->is_alive( ) )
			return;

		const bool is_enemy = g_ctx.m_local->is_enemy( entity );
		if ( ( entity_type == 0 && !is_enemy ) || ( entity_type == 1 && is_enemy ) )
			return;

		if ( GET_VARIABLE( g_variables.m_players_radar, bool ) && !entity->is_dormant( ) )
			entity->is_spotted( ) = true;

		const auto index = entity->get_index( );

		bool valid_dormant       = false;
		const auto backup_flags  = entity->get_flags( );
		const auto backup_origin = entity->get_abs_origin( );

		if ( entity->is_dormant( ) )
			valid_dormant = g_dormancy.adjust_player_sound( entity );
		else
			g_dormancy.m_sound_players[ index ].reset( true, entity->get_abs_origin( ), entity->get_flags( ) );

		if ( GET_VARIABLE( g_variables.m_players_only_audible, bool ) && entity->is_dormant( ) && !valid_dormant )
			return;

		if ( entity->is_dormant( ) ) {
			this->m_fading_alpha[ index ] -= delta_time;
		} else if ( this->m_fading_alpha[ index ] < 1.f ) {
			this->m_fading_alpha[ index ] += delta_time;
		}

		const auto client_renderable = entity->get_client_renderable( );
		if ( !client_renderable )
			return;

		const auto client_unknown = client_renderable->get_client_unknown( );
		if ( !client_unknown )
			return;

		const auto collideable = client_unknown->get_collideable( );
		if ( !collideable )
			return;

		const auto client_entity = client_unknown->get_client_entity( );
		if ( !client_entity )
			return;

		if ( this->m_fading_alpha[ index ] >= 1.0f )
			this->m_fading_alpha[ index ] = 1.0f;

		if ( this->m_fading_alpha[ index ] <= 0.0f ) {
			this->m_fading_alpha[ index ] = 0.0f;
			return;
		}

		const bool in_view_frustrum = !g_interfaces.m_engine_client->cull_box( collideable->get_obb_mins( ) + client_entity->get_abs_origin( ),
		                                                                       collideable->get_obb_maxs( ) + client_entity->get_abs_origin( ) );

		const auto client_networkable = client_unknown->get_client_networkable( );
		if ( !client_networkable )
			return;

		[ & ]( ) {
			if ( !GET_VARIABLE( g_variables.m_out_of_fov_arrows, bool ) || in_view_frustrum )
				return;

			const c_vector position_difference = g_ctx.m_local->get_abs_origin( ) - client_entity->get_abs_origin( );

			float x = std::cos( yaw ) * position_difference.m_y - std::sin( yaw ) * position_difference.m_x;
			float y = std::cos( yaw ) * position_difference.m_x + std::sin( yaw ) * position_difference.m_y;
			if ( const auto len = std::sqrt( x * x + y * y ); len ) {
				x /= len;
				y /= len;
			}

			const float triangle_size          = GET_VARIABLE( g_variables.m_out_of_fov_arrows_size, float );
			const int triangle_screen_position = GET_VARIABLE( g_variables.m_out_of_fov_arrows_distance, int );

			const c_vector_2d position =
				c_vector_2d( display_size.x / 2.f + ( x * triangle_screen_position ), display_size.y / 2.f + ( y * triangle_screen_position ) );

			const c_vector_2d triangle_position        = c_vector_2d( position.m_x + x * 3.f, position.m_y + y * 3.f );
			const c_vector_2d triangle_render_points[] = {
				c_vector_2d( 0.4f * y * triangle_size + triangle_position.m_x, triangle_position.m_y + -0.4f * x * triangle_size ),
				c_vector_2d( triangle_position.m_x + 1.0f * x * triangle_size, triangle_position.m_y + 1.0f * y * triangle_size ),
				c_vector_2d( triangle_position.m_x + -0.4f * y * triangle_size, triangle_position.m_y + 0.4f * x * triangle_size )
			};

			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_triangle,
				std::make_any< triangle_draw_object_t >(
					triangle_render_points[ 0 ], triangle_render_points[ 1 ], triangle_render_points[ 2 ],
					GET_VARIABLE( g_variables.m_out_of_fov_arrows_color, c_color ).get_u32( this->m_fading_alpha[ index ] ),
					e_triangle_flags::triangle_flag_filled ) );
		}( );

		bounding_box_t box{ };
		if ( !entity->get_bounding_box( &box ) )
			return;

		player_info_t player_info{ };
		if ( !g_interfaces.m_engine_client->get_player_info( index, &player_info ) )
			return;

		// fuck you
		std::string hp_text = std::to_string( entity->get_health( ) );
		if ( GET_VARIABLE( g_variables.m_players_health_suffix, bool ) )
			hp_text.append( "hp" );

		const int team = entity->get_team( );

		float alpha  = 0.f;
		bool can_see = g_ctx.m_local->can_see( entity->get_eye_position( false) );
		if (can_see)
		{
			alpha = std::clamp( this->m_fading_alpha[ index ], 0.f, GET_VARIABLE( g_variables.m_visible_alpha, int ) / 255.f );
		}
		else
		{
			alpha = std::clamp( this->m_fading_alpha[ index ], 0.f, GET_VARIABLE( g_variables.m_invisuals_alpha, int ) / 255.f );
		}

		if ( this->m_backup_player_data[ index ].m_animated_health > entity->get_health( ) )
			this->m_backup_player_data[ index ].m_animated_health -=
				( 100.f * g_interfaces.m_global_vars_base->m_frame_time ) /* NOTE ~ float ~ cannot use the variable delta_time as it is too slow. */;
		else
			this->m_backup_player_data[ index ].m_animated_health = entity->get_health( );

		const float factor = static_cast< float >( this->m_backup_player_data[ index ].m_animated_health ) / entity->get_max_health( );
		const float hue    = ( factor * 120.f ) / 360.f;

		float padding[ e_padding_direction::padding_direction_max ] = { 0.f, 0.f, 0.f, 0.f };

		if ( GET_VARIABLE( g_variables.m_players_box, bool ) ) {
			if ( !GET_VARIABLE( g_variables.m_players_box_corner, bool ) )
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_rect,
					std::make_any< rect_draw_object_t >(
						c_vector_2d( box.m_left, box.m_top ), c_vector_2d( box.m_right, box.m_bottom ),
						GET_VARIABLE( g_variables.m_players_box_color, c_color ).get_u32( alpha ),
						GET_VARIABLE( g_variables.m_players_box_outline_color, c_color ).get_u32( alpha ), false, 0.f,
						ImDrawFlags_::ImDrawFlags_None, 1.f,
						GET_VARIABLE( g_variables.m_players_box_outline, bool )
							? e_rect_flags::rect_flag_inner_outline | e_rect_flags::rect_flag_outer_outline
							: e_rect_flags::rect_flag_none ) );
			else {
				if ( GET_VARIABLE( g_variables.m_players_box_outline, bool ) ) {
					g_render.corner_rect(
						box.m_left, box.m_top, box.m_right, box.m_bottom,
						c_color( GET_VARIABLE( g_variables.m_players_box_outline_color, c_color ) ).get_u32( alpha ), 2.f );
				}

				g_render.corner_rect( box.m_left, box.m_top, box.m_right, box.m_bottom,
				                      c_color( GET_VARIABLE( g_variables.m_players_box_color, c_color ) ).get_u32( alpha ) );
			}
		}

		if ( GET_VARIABLE( g_variables.m_players_name, bool ) ) {
			std::string converted_name = std::string( player_info.m_name ).substr( 0, 24 );

			if ( converted_name.length( ) > 23U )
				converted_name.append( "..." );

			if ( player_info.m_fake_player )
				converted_name.insert( 0, "[bot] " );

			const auto text_size = g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp]->CalcTextSizeA(
				g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp]->FontSize, FLT_MAX, 0.f, converted_name.c_str());

			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_text,
				std::make_any<text_draw_object_t>(
					g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp],
					c_vector_2d(box.m_left + box.m_width * 0.5f - text_size.x * 0.5f, box.m_top - 3 - text_size.y), converted_name,
					GET_VARIABLE(g_variables.m_players_name_color, c_color).get_u32(alpha),
					c_color(0.f, 0.f, 0.f, alpha).get_u32(alpha), e_text_flags::text_flag_dropshadow));

			padding[ e_padding_direction::padding_direction_top ] -= text_size.y - 20;
		}

		const auto active_weapon = reinterpret_cast< c_base_entity* >(
			g_interfaces.m_client_entity_list->get_client_entity_from_handle( entity->get_active_weapon_handle( ) ) );
		if ( active_weapon )
			this->m_backup_player_data[ index ].m_active_weapon = active_weapon;

		if ( this->m_backup_player_data[ index ].m_active_weapon ) {
			const auto item_definition_index = this->m_backup_player_data[ index ].m_active_weapon->get_item_definition_index( );
			const auto weapon_data           = g_interfaces.m_weapon_system->get_weapon_data( item_definition_index );

			if ( weapon_data )
				this->m_backup_player_data[ index ].m_weapon_data = weapon_data;

			if ( this->m_backup_player_data[ index ].m_weapon_data ) {
				[ & ]( ) {
					if ( GET_VARIABLE( g_variables.m_player_ammo_bar, bool ) ) {
						if ( !this->m_backup_player_data[ index ].m_weapon_data->is_gun( ) )
							return;

						const float ammo = static_cast< float >( this->m_backup_player_data[ index ].m_active_weapon->get_ammo( ) );

						if ( this->m_backup_player_data[ index ].m_ammo > ammo )
							this->m_backup_player_data[ index ].m_ammo -= ( 2.f * g_interfaces.m_global_vars_base->m_frame_time );
						else
							this->m_backup_player_data[ index ].m_ammo = ammo;

						const int max_ammo = this->m_backup_player_data[ index ].m_weapon_data->m_max_clip1;

						float factor = this->m_backup_player_data[ index ].m_ammo / static_cast< float >( max_ammo );
						factor       = std::clamp< float >( factor, 0.f, 1.f );

						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_rect,
							std::make_any< rect_draw_object_t >(
								c_vector_2d( box.m_left, box.m_bottom + 3.f ), c_vector_2d( box.m_left + box.m_width, box.m_bottom + 5.f ),
								ImColor( 0.f, 0.f, 0.f, alpha ), ImColor( 0.f, 0.f, 0.f, alpha ),
								false, 0.f, ImDrawFlags_::ImDrawFlags_None, 1.f, rect_flag_outer_outline ) );

						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_rect,
							std::make_any< rect_draw_object_t >(
								c_vector_2d( box.m_left, box.m_bottom + 3.f ), c_vector_2d( box.m_left + box.m_width * factor, box.m_bottom + 5.f ),
								GET_VARIABLE( g_variables.m_player_ammo_bar_color, c_color ).get_u32( alpha ),
								ImColor( 0.f, 0.f, 0.f, alpha ), false, 0.f, ImDrawFlags_::ImDrawFlags_None, 1.f,
								rect_flag_outer_outline ) );

						padding[ e_padding_direction::padding_direction_bottom ] += 6.f;
					};
				}( );

				[ & ]( ) {
					if ( GET_VARIABLE( g_variables.m_weapon_name, bool ) ) {
						const auto localized_name = g_interfaces.m_localize->find( this->m_backup_player_data[ index ].m_weapon_data->m_hud_name );

						std::wstring w = localized_name;
						if ( w.empty( ) )
							return;

						std::transform( w.begin( ), w.end( ), w.begin( ), ::towlower );

						const std::string converted_name( w.begin( ), w.end( ) );
						if ( converted_name.empty( ) )
							return;

						const auto text_size = g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp]->CalcTextSizeA(
							g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp]->FontSize, FLT_MAX, 0.f, converted_name.c_str());

						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_text,
							std::make_any<text_draw_object_t>(
								g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp],
								c_vector_2d(box.m_left + (box.m_width - text_size.x) / 2,
						             box.m_bottom + 2 + padding[e_padding_direction::padding_direction_bottom]),
								converted_name, GET_VARIABLE(g_variables.m_weapon_name_color, c_color).get_u32(alpha),
								ImColor(0.f, 0.f, 0.f, alpha), text_flag_dropshadow));

						padding[ e_padding_direction::padding_direction_bottom ] += ( text_size.y + 1.f );
					}
				}( );

				if ( GET_VARIABLE( g_variables.m_weapon_icon, bool ) ) {
					if ( !( item_definition_index == e_item_definition_index::weapon_shield ||
					        item_definition_index == e_item_definition_index::weapon_breachcharge ||
					        item_definition_index == e_item_definition_index::weapon_bumpmine ) ) {
						const auto text      = reinterpret_cast< const char* >( g_utilities.get_weapon_icon( item_definition_index ) );
						const auto text_size = g_render.m_fonts[ e_font_names::font_name_icon_12 ]->CalcTextSizeA(
							g_render.m_fonts[ e_font_names::font_name_icon_12 ]->FontSize, FLT_MAX, 0.f, text );

						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_text,
							std::make_any< text_draw_object_t >(
								g_render.m_fonts[ e_font_names::font_name_icon_12 ],
								c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2,
						                     box.m_bottom + 2 + padding[ e_padding_direction::padding_direction_bottom ] ),
								text, GET_VARIABLE( g_variables.m_weapon_icon_color, c_color ).get_u32( alpha ),
								ImColor( 0.f, 0.f, 0.f, alpha ), text_flag_dropshadow ) );

						padding[ e_padding_direction::padding_direction_bottom ] += text_size.y;
					}
				}

				if ( GET_VARIABLE( g_variables.m_player_ammo_text, bool ) && this->m_backup_player_data[ index ].m_weapon_data->is_gun( ) ) {
					const std::string ammo_text = std::format( "{}/{}", this->m_backup_player_data[ index ].m_active_weapon->get_ammo( ),
					                                           this->m_backup_player_data[ index ].m_active_weapon->get_ammo_reserve( ) );
					const auto text_size = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->CalcTextSizeA(
						g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->FontSize, FLT_MAX, 0.f, ammo_text.c_str( ) );

					g_render.m_draw_data.emplace_back(
						e_draw_type::draw_type_text,
						std::make_any< text_draw_object_t >(
							g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ],
							c_vector_2d( box.m_left + ( box.m_width - text_size.x ) * 0.5f,
							             box.m_bottom + 2 + padding[ e_padding_direction::padding_direction_bottom ] ),
							ammo_text, GET_VARIABLE( g_variables.m_player_ammo_bar_color, c_color ).get_u32( alpha ),
							ImColor( 0.f, 0.f, 0.f, alpha ), text_flag_dropshadow ) );

					padding[ e_padding_direction::padding_direction_bottom ] += text_size.y + 1.f;
				}
			}
		}

		if ( GET_VARIABLE( g_variables.m_players_health_text, bool ) ) {
			const auto text_size = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->CalcTextSizeA(
				g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->FontSize, FLT_MAX, 0.f, hp_text.c_str( ) );
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_text,
				std::make_any< text_draw_object_t >(
					g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ],
					c_vector_2d( box.m_right + 4.f, box.m_top + padding[ e_padding_direction::padding_direction_right ] ), hp_text,
					c_color( 1.f, 1.f, 1.f, 1.f ).get_u32( alpha ), ImColor( 0.f, 0.f, 0.f, alpha ), text_flag_dropshadow ) );
			padding[ e_padding_direction::padding_direction_right ] += text_size.y + 1.f;
		}

		if ( GET_VARIABLE( g_variables.m_players_flags, bool ) || GET_VARIABLE( g_variables.m_players_emitted_sounds, bool ) ) {
			std::vector< std::string > flags{ };
			if ( player_info.m_fake_player )
				flags.emplace_back( "BOT" );
			if ( entity->is_scoped( ) )
				flags.emplace_back( "ZOOM" );
			if ( entity->get_armor( ) > 0 )
				flags.emplace_back( entity->has_helmet( ) ? "HK" : "K" );
			if ( this->m_backup_player_data[ index ].m_active_weapon && this->m_backup_player_data[ index ].m_active_weapon->is_reloading( ) )
				flags.emplace_back( "RELOAD" );
			if ( GET_VARIABLE( g_variables.m_players_emitted_sounds, bool ) && entity->is_dormant( ) && valid_dormant )
				flags.emplace_back( "SOUND" );

			for ( const auto& flag : flags ) {
				const auto text_size = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->CalcTextSizeA(
					g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->FontSize, FLT_MAX, 0.f, flag.c_str( ) );
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_text,
					std::make_any< text_draw_object_t >(
						g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ],
						c_vector_2d( box.m_right + 4.f, box.m_top + padding[ e_padding_direction::padding_direction_right ] ), flag,
						GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( alpha ), ImColor( 0.f, 0.f, 0.f, alpha ), text_flag_dropshadow ) );
				padding[ e_padding_direction::padding_direction_right ] += text_size.y + 1.f;
			}
		}

		if ( GET_VARIABLE( g_variables.m_players_distance, bool ) ) {
			const int distance = static_cast< int >( g_ctx.m_local->get_abs_origin( ).dist_to( entity->get_abs_origin( ) ) * 0.0254f );
			const std::string distance_text = std::format( "{}m", distance );
			const auto text_size = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->CalcTextSizeA(
				g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->FontSize, FLT_MAX, 0.f, distance_text.c_str( ) );
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_text,
				std::make_any< text_draw_object_t >(
					g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ],
					c_vector_2d( box.m_left + ( box.m_width - text_size.x ) * 0.5f,
					             box.m_bottom + 2 + padding[ e_padding_direction::padding_direction_bottom ] ),
					distance_text, c_color( 1.f, 1.f, 1.f, 1.f ).get_u32( alpha ), ImColor( 0.f, 0.f, 0.f, alpha ), text_flag_dropshadow ) );
			padding[ e_padding_direction::padding_direction_bottom ] += text_size.y + 1.f;
		}

		if ( GET_VARIABLE( g_variables.m_players_snaplines, bool ) ) {
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line,
			                                   std::make_any< line_draw_object_t >(
												   c_vector_2d( g_ctx.m_width * 0.5f, static_cast< float >( g_ctx.m_height ) ),
			                                                   c_vector_2d( box.m_left + box.m_width * 0.5f, box.m_bottom ),
			                                                   GET_VARIABLE( g_variables.m_players_box_color, c_color ).get_u32( alpha * 0.65f ),
			                                                   1.f ) );
		}

		if ( GET_VARIABLE( g_variables.m_players_backtrack_eye_trail, bool ) && GET_VARIABLE( g_variables.m_backtrack_enable, bool ) ) {
			auto record = &Backtrack::records[ entity->get_index( ) ];
			if ( record && record->size( ) > 1U ) {
				c_vector_2d last{ };
				bool has_last = false;
				const std::size_t max_records = std::min< std::size_t >( record->size( ), 10U );
				for ( std::size_t i = 0U; i < max_records; ++i ) {
					if ( !Backtrack::valid( record->at( i ).simulationTime ) )
						continue;

					c_vector_2d current{ };
					if ( !g_render.world_to_screen( record->at( i ).head, current ) )
						continue;

					if ( has_last ) {
						const float trail_alpha = alpha * std::clamp( 1.f - static_cast< float >( i ) / 10.f, 0.15f, 0.85f );
						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_line,
							std::make_any< line_draw_object_t >( last, current, GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( trail_alpha ),
							                                     1.2f ) );
					}
					last = current;
					has_last = true;
				}
			}
		}

		if ( GET_VARIABLE( g_variables.m_players_damage_indicators, bool ) ) {
			for ( const auto& indicator : g_damage_indicators ) {
				if ( indicator.index != index )
					continue;

				const float lifetime = g_interfaces.m_global_vars_base->m_real_time - indicator.time;
				const float indicator_alpha = std::clamp( 1.f - lifetime / 1.35f, 0.f, 1.f ) * alpha;
				const std::string damage_text = std::format( "-{}", indicator.damage );
				const auto text_size = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->CalcTextSizeA(
					g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ]->FontSize, FLT_MAX, 0.f, damage_text.c_str( ) );
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_text,
					std::make_any< text_draw_object_t >(
						g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_esp ],
						c_vector_2d( box.m_left + box.m_width * 0.5f - text_size.x * 0.5f,
						             box.m_top - 17.f - lifetime * 18.f - padding[ e_padding_direction::padding_direction_top ] ),
						damage_text, c_color( 1.f, 0.15f, 0.15f, 1.f ).get_u32( indicator_alpha ), ImColor( 0.f, 0.f, 0.f, indicator_alpha ),
						text_flag_dropshadow ) );
			}
		}

		if ( GET_VARIABLE( g_variables.m_players_avatar, bool ) ) {
			constexpr const static auto avatar_size = 14.f;

			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_texture,
			                                   std::make_any< texture_draw_object_t >(
												   c_vector_2d( box.m_left + ( box.m_width - avatar_size ) * 0.5f,
			                                                    box.m_top - 5 - avatar_size - padding[ e_padding_direction::padding_direction_top ] ),
												   c_vector_2d( avatar_size, avatar_size ), ImColor( 1.f, 1.f, 1.f, alpha ),
												   player_info.m_fake_player ? team == e_team_id::team_tt   ? g_render.m_terrorist_avatar
			                                                                   : team == e_team_id::team_ct ? g_render.m_counter_terrorist_avatar
			                                                                                                : nullptr
																			 : g_avatar_cache[ index ],
												   0.f, ImDrawFlags_::ImDrawFlags_None ) );

			padding[ e_padding_direction::padding_direction_top ] -= avatar_size;
		}

		if ( GET_VARIABLE( g_variables.health_bar_enable, bool ) ) {
			static float prev_player_hp[ 65 ];

			int idx = entity->get_index( ); // используем единый индекс
			if ( prev_player_hp[ idx ] > entity->get_health( ) ) {
				prev_player_hp[ idx ] -= anim_time * g_interfaces.m_global_vars_base->m_frame_time;
			} else {
				prev_player_hp[ idx ] = entity->get_health( );
			}

			int g = static_cast< int >( entity->get_health( ) * 2.55f );
			int r = 255 - g;

			// Теперь вычисляем цвет так же, как в оригинале: (r, g, 0, 255)
			unsigned int rgb_health_color = c_color::from_hsb( hue, 1.f, 1.f, 1.f ).get_u32( alpha );

			unsigned int health_bar         = GET_VARIABLE( g_variables.colors_health_bar, c_color ).get_u32( alpha );
			unsigned int health_bar_upper   = GET_VARIABLE( g_variables.colors_health_bar_upper, c_color ).get_u32( alpha );
			unsigned int health_bar_lower   = GET_VARIABLE( g_variables.colors_health_bar_lower, c_color ).get_u32( alpha );
			unsigned int health_bar_outline = GET_VARIABLE( g_variables.colors_health_bar_outline, c_color ).get_u32( alpha );

			if ( GET_VARIABLE( g_variables.health_bar_background, bool ) ) {
				g_draw.rect_filled( box.m_left - 2, box.m_top - 1, ( -GET_VARIABLE( g_variables.health_bar_size, int ) - 2 ), box.m_height + 2,
				                    health_bar_outline );
			} else {
				g_draw.rect_outlined( box.m_left - 3, box.m_top - 1, ( -GET_VARIABLE( g_variables.health_bar_size, int ) ), box.m_height + 2,
				                      health_bar_outline );
			}

			// Если не включены режимы baseonhealth и gradient – отрисовываем базовый прямоугольник
			if ( !GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) && !GET_VARIABLE( g_variables.health_bar_gradient, bool ) ) {
				g_draw.rect_filled( box.m_left - 3, box.m_top + ( box.m_height - box.m_height * prev_player_hp[ idx ] / 100.f ),
				                    ( -GET_VARIABLE( g_variables.health_bar_size, int ) ), ( box.m_height * prev_player_hp[ idx ] / 100.f ),
				                    health_bar );
			}

			// Если включен режим baseonhealth – отрисовываем прямоугольник, основанный на здоровье
			if ( GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) ) {
				g_draw.rect_filled( box.m_left - 3, box.m_top + ( box.m_height - box.m_height * prev_player_hp[ idx ] / 100.f ),
				                    ( -GET_VARIABLE( g_variables.health_bar_size, int ) ), ( box.m_height * prev_player_hp[ idx ] / 100.f ),
				                    rgb_health_color );
			}

			// Если включен градиент – отрисовываем градиентный прямоугольник
			if ( GET_VARIABLE( g_variables.health_bar_gradient, bool ) ) {
				g_draw.rect_gradient( box.m_left - 3, box.m_top + ( box.m_height - box.m_height * prev_player_hp[ idx ] / 100.f ),
				                      ( -GET_VARIABLE( g_variables.health_bar_size, int ) ), ( box.m_height * prev_player_hp[ idx ] / 100.f ),
				                      health_bar_upper, health_bar_upper, health_bar_lower, health_bar_lower );
			}

			// Если включён вывод текста и здоровье меньше 99, отрисовываем его
			if ( GET_VARIABLE( g_variables.health_bar_text, bool ) && entity->get_health( ) < 99 ) {
				ImVec2 size = g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp]->CalcTextSizeA(
					g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp]->FontSize, FLT_MAX,
					NULL, std::to_string(entity->get_health()).c_str());

				text_draw_object_t txt{};
				txt.m_position = c_vector_2d((box.m_left - (size.x + (float)GET_VARIABLE(g_variables.health_bar_size, int))) - size.x / 2.f,
				                          box.m_top + (box.m_height - box.m_height * prev_player_hp[idx] / 100.f));
				txt.m_color = c_color(1.f, 1.f, 1.f).get_u32(alpha);
				txt.m_font = g_render.m_custom_fonts[e_custom_font_names::custom_font_name_esp];
				txt.m_outline_color = c_color(0.f, 0.f, 0.f).get_u32(alpha);
				txt.m_draw_flags = text_flag_dropshadow;
				txt.m_text = std::to_string(entity->get_health());

				g_render.m_draw_data.emplace_back(e_draw_type::draw_type_text, std::make_any<text_draw_object_t>(txt));
			}
		}

		[ & ]( const bool draw_skeleton ) {
			if ( !draw_skeleton )
				return;

			auto draw_skeleton_matrix = [ & ]( matrix3x4_t* bone_matrix ) {
				if ( !bone_matrix )
					return;

				const auto model = client_renderable->get_model( );
				if ( model ) {
					const auto studio_model = g_interfaces.m_model_info->get_studio_model( model );

					if ( studio_model ) {
						c_vector child_position = { }, parent_position = { };
						c_vector_2d child_screen_position = { }, parent_screen_position = { };

						c_vector upper_direction = c_vector( bone_matrix[ 7 ][ 0 ][ 3 ], bone_matrix[ 7 ][ 1 ][ 3 ], bone_matrix[ 7 ][ 2 ][ 3 ] ) -
						                           c_vector( bone_matrix[ 6 ][ 0 ][ 3 ], bone_matrix[ 6 ][ 1 ][ 3 ], bone_matrix[ 6 ][ 2 ][ 3 ] );
						c_vector breast_bone =
							c_vector( bone_matrix[ 6 ][ 0 ][ 3 ], bone_matrix[ 6 ][ 1 ][ 3 ], bone_matrix[ 6 ][ 2 ][ 3 ] ) + upper_direction * 0.5f;

						for ( int i = 0; i < studio_model->n_bones; i++ ) {
							mstudiobone_t* bone = studio_model->get_bone( i );
							if ( !bone )
								continue;

							if ( bone->m_parent == -1 )
								continue;

							if ( !( bone->m_flags & 0x00000100 /* BONE_USED_BY_HITBOX */ ) )
								continue;

							child_position  = c_vector( bone_matrix[ i ][ 0 ][ 3 ], bone_matrix[ i ][ 1 ][ 3 ], bone_matrix[ i ][ 2 ][ 3 ] );
							parent_position = c_vector( bone_matrix[ bone->m_parent ][ 0 ][ 3 ], bone_matrix[ bone->m_parent ][ 1 ][ 3 ],
							                            bone_matrix[ bone->m_parent ][ 2 ][ 3 ] );

							c_vector delta_child  = child_position - breast_bone;
							c_vector delta_parent = parent_position - breast_bone;

							if ( delta_parent.length( ) < 9.0f && delta_child.length( ) < 9.0f )
								parent_position = breast_bone;

							if ( i == 5 )
								child_position = breast_bone;

							if ( fabs( delta_child.m_z ) < 5.0f && delta_parent.length( ) < 5.0f && delta_child.length( ) < 5.0f || i == 6 )
								continue;

							if ( g_render.world_to_screen( child_position, child_screen_position ) &&
							     g_render.world_to_screen( parent_position, parent_screen_position ) ) {
								g_render.m_draw_data.emplace_back(
									e_draw_type::draw_type_line,
									std::make_any< line_draw_object_t >(
										child_screen_position, parent_screen_position,
										GET_VARIABLE( g_variables.m_players_skeleton_color, c_color ).get_u32( alpha ),
										1.f ) );
							}
						}
					}
				}
			};

			if ( GET_VARIABLE( g_variables.m_players_skeleton_type, int ) == 1 && GET_VARIABLE( g_variables.m_backtrack_enable, bool ) ) {
				 auto record = &Backtrack::records[ entity->get_index( ) ];
				
				 if ( !Backtrack::valid(record->front().simulationTime) )
					return;
				
				 auto bone_matrix = record->front( ).matrix;
				
				 draw_skeleton_matrix( bone_matrix );

			} else {
				matrix3x4_t bone_matrix[ 128 ]{ };

				memcpy( bone_matrix, entity->get_cached_bone_data( ).get_elements( ),
				        entity->get_cached_bone_data( ).count( ) * sizeof( matrix3x4_t ) );

				draw_skeleton_matrix( bone_matrix );
			}
		}( GET_VARIABLE( g_variables.m_players_skeleton, bool ) );

		

		// restore

		if ( entity->is_dormant( ) ) {
			entity->get_flags( ) = backup_flags;
			entity->set_abs_origin( backup_origin );
		}
	} );
}
