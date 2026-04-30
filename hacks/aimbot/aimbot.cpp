#include "aimbot.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/branding/branding.h"
#include "../../globals/includes/includes.h"
#include "../auto_wall/auto_wall.h"
#include "../entity_cache/entity_cache.h"
#include "../prediction/prediction.h"
#include "../../dependencies/json/json.hpp"
#include <fstream>
bool n_aimbot::impl_t::save( )
{
	// Путь сохранения файла
	const std::string filePath = n_branding::k_aimbot_config;

	// Формируем JSON-массив для настроек aimbot
	nlohmann::json aimbotArray = nlohmann::json::array( );

	for ( const auto& settings : m_aimbot_settings ) {
		nlohmann::json jsonSettings;
		jsonSettings[ "m_enable" ]           = settings.m_enable;
		jsonSettings[ "m_silent" ]           = settings.m_silent;
		jsonSettings[ "m_visibility_check" ] = settings.m_visibility_check;
		jsonSettings[ "m_friendly_fire" ]    = settings.m_friendly_fire;
		jsonSettings[ "m_recoil_control" ]   = settings.m_recoil_control;
		jsonSettings[ "m_aim_to_backtrack" ]   = settings.m_aim_to_backtrack;

		// Массив hitboxes
		nlohmann::json hitboxes = nlohmann::json::array( );
		for ( size_t i = 0; i < hitboxex_form_t::max_hitboxes; ++i ) {
			hitboxes.push_back( settings.m_hitboxes[ i ] );
		}
		jsonSettings[ "m_hitboxes" ] = hitboxes;
		jsonSettings[ "m_fov" ]      = settings.m_fov;
		jsonSettings[ "m_smooth" ]   = settings.m_smooth;

		aimbotArray.push_back( jsonSettings );
	}

	// Записываем JSON в файл
	try {
		// Создаём директорию, если её нет
		std::filesystem::create_directories( n_branding::k_config_directory );

		std::ofstream outFile( filePath, std::ios::out | std::ios::trunc );
		if ( !outFile.good( ) )
			return false;

		outFile << aimbotArray.dump( 4 );
		outFile.close( );
	} catch ( const std::exception& ex ) {
		// Можно добавить вывод ошибки в консоль или лог
		return false;
	}

	return true;
}


bool n_aimbot::impl_t::load( )
{
	const std::string filePath = n_branding::k_aimbot_config;
	nlohmann::json aimbotArray;

	std::ifstream inFile( filePath, std::ios::in );
	if ( !inFile.good( ) )
		return false;

	try {
		inFile >> aimbotArray;
		inFile.close( );
	} catch ( const std::exception& ex ) {
		return false;
	}

	if ( !aimbotArray.is_array( ) )
		return false;

	// Загружаем данные из JSON
	for ( size_t i = 0; i < m_aimbot_settings.size( ) && i < aimbotArray.size( ); ++i ) {
		const auto& jsonSettings                  = aimbotArray[ i ];
		m_aimbot_settings[ i ].m_enable           = jsonSettings.value( "m_enable", false );
		m_aimbot_settings[ i ].m_silent           = jsonSettings.value( "m_silent", false );
		m_aimbot_settings[ i ].m_visibility_check = jsonSettings.value( "m_visibility_check", false );
		m_aimbot_settings[ i ].m_friendly_fire    = jsonSettings.value( "m_friendly_fire", false );
		m_aimbot_settings[ i ].m_recoil_control   = jsonSettings.value( "m_recoil_control", false );
		m_aimbot_settings[ i ].m_aim_to_backtrack   = jsonSettings.value( "m_aim_to_backtrack", false );

		if ( jsonSettings.contains( "m_hitboxes" ) && jsonSettings[ "m_hitboxes" ].is_array( ) ) {
			const auto& hitboxes = jsonSettings[ "m_hitboxes" ];
			for ( size_t j = 0; j < hitboxes.size( ) && j < hitboxex_form_t::max_hitboxes; ++j ) {
				m_aimbot_settings[ i ].m_hitboxes[ j ] = hitboxes[ j ].get< bool >( );
			}
		}

		m_aimbot_settings[ i ].m_fov    = jsonSettings.value( "m_fov", 0.f );
		m_aimbot_settings[ i ].m_smooth = jsonSettings.value( "m_smooth", 1.f );
	}

	return true;
}

c_base_entity* n_aimbot::impl_t::find_closest_player( )
{
	
	float closest_fov             = 180.f;
	c_base_entity* closest_player = nullptr;

	const auto eye_position = g_ctx.m_local->get_eye_position( false );

	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || !entity->is_alive( ) || entity->is_dormant( ) || !g_ctx.m_local->is_enemy( entity ) || entity == g_ctx.m_local )
			return;

		const auto hitbox_position = entity->get_hitbox_position( hitbox_head );
		auto save_tick_count       = g_ctx.m_cmd->m_tick_count;
	   {

			
			fire_bullet_data_t data = { };
			if ( g_auto_wall.get_damage( hitbox_position, &data ) < 0.f )
				return;
		}

		auto angle = g_math.calculate_angle( eye_position, hitbox_position );
		angle -= ( g_ctx.m_local->get_punch( ) * g_convars[ HASH_BT( "weapon_recoil_scale" ) ]->get_float( ) );

		if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle ); calculated_fov < closest_fov ) {
			closest_fov    = calculated_fov;
			closest_player = entity;
		}
	} );

	return closest_player;
}

n_aimbot::impl_t::aimbot_settings_t n_aimbot::impl_t::get_aimbot_settings( )
{
	if ( !g_ctx.m_local )
		return { };
	if ( !g_ctx.m_local->is_alive( ) )
		return g_aimbot.m_aimbot_settings[ 0 ];

	const auto weapon = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_active_weapon_handle( ) );
	if ( !weapon )
		return g_aimbot.m_aimbot_settings[ 0 ];

	const auto weapon_data = g_interfaces.m_weapon_system->get_weapon_data( weapon->get_item_definition_index( ) );
	if ( !weapon_data )
		return g_aimbot.m_aimbot_settings[ 0 ];
	if ( weapon_data->m_weapon_type == weapontype_sniper )
		return g_aimbot.m_aimbot_settings[ 0 ];
	else if ( weapon_data->m_weapon_type == weapontype_rifle )
		return g_aimbot.m_aimbot_settings[ 1 ];
	else if ( weapon_data->m_weapon_type == weapontype_shotgun )
		return g_aimbot.m_aimbot_settings[ 2 ];
	else if ( weapon_data->m_weapon_type == weapontype_machinegun )
		return g_aimbot.m_aimbot_settings[ 3 ];
	else if ( weapon_data->m_weapon_type == weapontype_pistol )
		return g_aimbot.m_aimbot_settings[ 4 ];
	else
		return g_aimbot.m_aimbot_settings[ 0 ];
	return g_aimbot.m_aimbot_settings[ 0 ];
}

bool n_aimbot::impl_t::can_aimbot( )
{
	return g_ctx.m_cmd->m_tick_count != 0 && g_ctx.m_cmd->m_buttons & in_attack;
}

void n_aimbot::impl_t::on_create_move_post( )
{
	if ( !can_aimbot( ) )
		return;

	



	const auto weapon = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_active_weapon_handle( ) );
	if ( !weapon )
		return;

	const auto weapon_data = g_interfaces.m_weapon_system->get_weapon_data( weapon->get_item_definition_index( ) );
	if ( !weapon_data )
		return;
	//weapon_data->m_weapon_type
	if ( !weapon_data->is_gun( ) )
		return;

	if ( !( g_ctx.m_local->can_shoot( weapon ) ) )
		return;
	aimbot_settings_t settings    = get_aimbot_settings( );
	if ( settings.m_enable == false )
		return;
	float closest_fov             = settings.m_fov;
	c_base_entity* closest_player = nullptr;
	c_angle angles_to_pos{ };
	const auto eye_position = g_ctx.m_local->get_eye_position( false );
	int ticks_to_simulate   = -1;
	g_entity_cache.enumerate(
		e_enumeration_type::type_players,
		[ & ]( c_base_entity* entity ) {
			if ( !entity || !entity->is_alive( ) || entity->is_dormant( ) || !g_ctx.m_local->is_enemy( entity ) || entity == g_ctx.m_local )
				return;

			for ( int i = 0; i < hitboxex_form_t::max_hitboxes; i++ ) {
				if ( !settings.m_hitboxes[ i ] )
					continue;

				c_vector hitbox_position{ };
				if ( i == hitboxex_form_t::hb_head ) {
					if ( settings.m_hitboxes[ i ] )
						hitbox_position = entity->get_hitbox_position( hitbox_head );
				} else if ( i == hitboxex_form_t::hb_neck ) {
					if ( settings.m_hitboxes[ i ] )
						hitbox_position = entity->get_hitbox_position( hitbox_neck );
				} else if ( i == hitboxex_form_t::hb_body ) {
					if ( settings.m_hitboxes[ i ] )
						hitbox_position = entity->get_hitbox_position( hitbox_chest );
				} else if ( i == hitboxex_form_t::hb_legs ) {
					if ( settings.m_hitboxes[ i ] )
						hitbox_position = entity->get_hitbox_position( hitbox_left_foot );
				}
				auto save_tick_count = g_ctx.m_cmd->m_tick_count;
				if ( settings.m_visibility_check ) {
					if (!g_ctx.m_local->can_see(hitbox_position))
					{
						if ( settings.m_aim_to_backtrack ) {
							auto records = &Backtrack::records[ entity->get_index( ) ];
							if ( !records || records->empty( ) || !Backtrack::valid( records->front( ).simulationTime ) || records->size( ) <= 3 ||
						         !GET_VARIABLE( g_variables.m_backtrack_enable, bool ) ) {
							} else {
								int bestRecord{ };
								int ticks_to_backtrack = std::clamp( ( int )Backtrack::records[ entity->get_index( ) ].size( ) - 2, 0,
							                                         ( int )Backtrack::records[ entity->get_index( ) ].size( ) - 2 );
								{
									auto& record = Backtrack::records[ entity->get_index( ) ][ ticks_to_backtrack ];
									if ( Backtrack::valid( record.simulationTime ) ) {
										if ( !g_ctx.m_local->can_see( record.head ) )
											return;
										c_angle angle{ };
										if ( settings.m_hitboxes[ hb_head ] )
											angle = g_math.calculate_angle( eye_position, record.head );
										else if ( settings.m_hitboxes[ hb_neck ] )
											angle = g_math.calculate_angle( eye_position, record.neck );
										else if ( settings.m_hitboxes[ hb_body ] )
											angle = g_math.calculate_angle( eye_position, record.body );
										else if ( settings.m_hitboxes[ hb_legs ] )
											angle = g_math.calculate_angle( eye_position, record.legs );

										if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle );
									         calculated_fov < closest_fov ) {
											closest_fov       = calculated_fov;
											closest_player    = entity;
											angles_to_pos     = angle - g_ctx.m_cmd->m_view_point;
											ticks_to_simulate = Backtrack::timeToTicks( record.simulationTime + Backtrack::getLerp( ) );
											return;
										}
									}
								}
							}
						}
						return;
					}
				} else {
					fire_bullet_data_t data = { };
					if (g_auto_wall.get_damage(hitbox_position, &data) < 0.f)
					{
						if ( settings.m_aim_to_backtrack ) {
							auto records = &Backtrack::records[ entity->get_index( ) ];
							if ( !records || records->empty( ) || !Backtrack::valid( records->front( ).simulationTime ) || records->size( ) <= 3 ||
						         !GET_VARIABLE( g_variables.m_backtrack_enable, bool ) ) {
							} else {
								int bestRecord{ };
								int ticks_to_backtrack = std::clamp( ( int )Backtrack::records[ entity->get_index( ) ].size( ) - 2, 0,
							                                         ( int )Backtrack::records[ entity->get_index( ) ].size( ) - 2 );
								{
									auto& record = Backtrack::records[ entity->get_index( ) ][ ticks_to_backtrack ];
									if ( Backtrack::valid( record.simulationTime ) ) {
										fire_bullet_data_t data = { };
										if ( g_auto_wall.get_damage( settings.m_hitboxes[ hb_head ], &data ) < 0.f )
											return;
										c_angle angle{ };
										if ( settings.m_hitboxes[ hb_head ] )
											angle = g_math.calculate_angle( eye_position, record.head );
										else if ( settings.m_hitboxes[ hb_neck ] )
											angle = g_math.calculate_angle( eye_position, record.neck );
										else if ( settings.m_hitboxes[ hb_body ] )
											angle = g_math.calculate_angle( eye_position, record.body );
										else if ( settings.m_hitboxes[ hb_legs ] )
											angle = g_math.calculate_angle( eye_position, record.legs );

										if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle );
									         calculated_fov < closest_fov ) {
											closest_fov       = calculated_fov;
											closest_player    = entity;
											angles_to_pos     = angle - g_ctx.m_cmd->m_view_point;
											ticks_to_simulate = Backtrack::timeToTicks( record.simulationTime + Backtrack::getLerp( ) );
											return;
										}
									}
								}
							}
						}
						return;
					}
					
				}

			if ( settings.m_aim_to_backtrack ) {
					auto records = &Backtrack::records[ entity->get_index( ) ];
					if ( !records || records->empty( ) || !Backtrack::valid( records->front( ).simulationTime ) || records->size( ) <= 3 ||
				         !GET_VARIABLE( g_variables.m_backtrack_enable, bool ) ) {
					} else {
						int bestRecord{ };
						int ticks_to_backtrack = std::clamp( ( int )Backtrack::records[ entity->get_index( ) ].size( ) - 2, 0,
					                                         ( int )Backtrack::records[ entity->get_index( ) ].size( ) - 2 );
						{
							auto& record = Backtrack::records[ entity->get_index( ) ][ ticks_to_backtrack ];
							if ( Backtrack::valid( record.simulationTime ) ) {
								c_angle angle{ };
								if ( settings.m_hitboxes[ hb_head ] )
									angle = g_math.calculate_angle( eye_position, record.head );
								else if ( settings.m_hitboxes[ hb_neck ] )
									angle = g_math.calculate_angle( eye_position, record.neck );
								else if ( settings.m_hitboxes[ hb_body ] )
									angle = g_math.calculate_angle( eye_position, record.body );
								else if ( settings.m_hitboxes[ hb_legs ] )
									angle = g_math.calculate_angle( eye_position, record.legs );

								if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle ); calculated_fov < closest_fov ) {
									closest_fov       = calculated_fov;
									closest_player    = entity;
									angles_to_pos     = angle - g_ctx.m_cmd->m_view_point;
									ticks_to_simulate = Backtrack::timeToTicks( record.simulationTime + Backtrack::getLerp( ) );
									return;
								}
							}
						}
					}
				}
					
				auto angle = g_math.calculate_angle( eye_position, hitbox_position );
				if ( ticks_to_simulate == -1 ) {
					if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle ); calculated_fov < closest_fov ) {
						closest_fov    = calculated_fov;
						closest_player = entity;
						angles_to_pos  = angle - g_ctx.m_cmd->m_view_point;
					}
				}
			}
		}
		
	 );

	if ( !closest_player )
		return;
	

	


	if ( settings.m_recoil_control )
		angles_to_pos -= ( g_ctx.m_local->get_punch( ) * g_convars[ HASH_BT( "weapon_recoil_scale" ) ]->get_float( ) );
	angles_to_pos.normalize( );

	if ( ticks_to_simulate != -1 )
		g_ctx.m_cmd->m_tick_count = ticks_to_simulate;

	g_ctx.m_cmd->m_view_point += angles_to_pos * settings.m_smooth;
	if ( !settings.m_silent )
		g_interfaces.m_engine_client->set_view_angles( g_ctx.m_cmd->m_view_point );
}
