#pragma once

#include "../../game/sdk/includes/includes.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

inline bool g_in_movement_assist_simulation = false;
inline int g_movement_assist_simulation_depth = 0;
inline const char* g_movement_assist_simulation_reason = "none";
inline int g_drainware_suppressed_sound_count = 0;
inline std::string g_drainware_last_suppressed_sound;
inline bool g_drainware_custom_movement_sounds_muted = false;
inline bool g_drainware_sound_spam_detected = false;
inline bool g_drainware_panic_restore_requested = false;
inline std::string g_drainware_last_stability_action = "boot";
inline std::string g_drainware_last_error_line = "none";
inline std::string g_drainware_particle_health = "unknown";
inline std::string g_drainware_config_health = "unknown";
inline std::string g_drainware_inventory_health = "unknown";
inline std::string g_drainware_world_health = "unknown";

inline void movement_assist_debug_log( const char* feature, const std::string& message, const float rate_limit = 0.f,
                                       const char* rate_key = nullptr )
{
	if ( rate_limit > 0.f && rate_key ) {
		static std::unordered_map< std::string, float > last_log_time;
		const auto now = std::chrono::steady_clock::now( );
		const float now_seconds = std::chrono::duration< float >( now.time_since_epoch( ) ).count( );
		const std::string key = std::string( feature ? feature : "assist" ) + ':' + rate_key;
		const auto it = last_log_time.find( key );
		if ( it != last_log_time.end( ) && now_seconds - it->second < rate_limit )
			return;
		last_log_time[ key ] = now_seconds;
	}

	std::filesystem::create_directories( "C:\\drainware" );
	std::ofstream file( "C:\\drainware\\debuglog.txt", std::ios::app );
	if ( !file.good( ) )
		return;

	const auto wall_now = std::chrono::system_clock::now( );
	const auto time = std::chrono::system_clock::to_time_t( wall_now );
	std::tm local_time{ };
	localtime_s( &local_time, &time );

	file << '[' << std::put_time( &local_time, "%H:%M:%S" ) << "][" << ( feature ? feature : "assist" ) << "] " << message << '\n';
}

inline void drainware_stability_breadcrumb( const char* action )
{
	g_drainware_last_stability_action = action && action[ 0 ] ? action : "unknown";
	movement_assist_debug_log( "stability", std::string( "entering=" ) + g_drainware_last_stability_action, 0.08f,
	                           g_drainware_last_stability_action.c_str( ) );
}

inline void drainware_stability_error( const char* feature, const std::string& message )
{
	g_drainware_last_error_line = std::string( "[" ) + ( feature ? feature : "stability" ) + "] " + message;
	movement_assist_debug_log( feature ? feature : "stability", message, 0.15f, feature ? feature : "stability_error" );
}

inline void drainware_note_suppressed_sound( const char* reason, const char* sample )
{
	++g_drainware_suppressed_sound_count;
	g_drainware_last_suppressed_sound = std::string( reason ? reason : "unknown" ) + " sample=" + ( sample ? sample : "<null>" );
	if ( g_drainware_suppressed_sound_count > 48 ) {
		g_drainware_sound_spam_detected = true;
		g_drainware_custom_movement_sounds_muted = true;
	}
}

class ScopedMovementAssistSimulation
{
public:
	explicit ScopedMovementAssistSimulation( const char* reason ) :
		m_previous_reason( g_movement_assist_simulation_reason )
	{
		++g_movement_assist_simulation_depth;
		g_in_movement_assist_simulation = true;
		g_movement_assist_simulation_reason = reason && reason[ 0 ] ? reason : "movement_assist";
	}

	~ScopedMovementAssistSimulation( )
	{
		if ( g_movement_assist_simulation_depth > 0 )
			--g_movement_assist_simulation_depth;

		if ( g_movement_assist_simulation_depth <= 0 ) {
			g_movement_assist_simulation_depth = 0;
			g_in_movement_assist_simulation = false;
			g_movement_assist_simulation_reason = m_previous_reason ? m_previous_reason : "none";
		} else {
			g_movement_assist_simulation_reason = m_previous_reason ? m_previous_reason : "movement_assist";
		}
	}

	ScopedMovementAssistSimulation( const ScopedMovementAssistSimulation& ) = delete;
	ScopedMovementAssistSimulation& operator=( const ScopedMovementAssistSimulation& ) = delete;

private:
	const char* m_previous_reason = "none";
};

class ScopedMovementAssistState
{
public:
	ScopedMovementAssistState( c_base_entity* entity, c_user_cmd* cmd, const bool restore_cmd = false ) :
		m_entity( entity ), m_cmd( cmd ), m_restore_cmd( restore_cmd )
	{
		if ( !m_entity )
			return;

		m_valid = true;
		m_origin = m_entity->get_origin( );
		m_abs_origin = m_entity->get_abs_origin( );
		m_velocity = m_entity->get_velocity( );
		m_base_velocity = m_entity->get_base_velocity( );
		m_flags = m_entity->get_flags( );
		m_eflags = m_entity->get_eflags( );
		m_move_type = m_entity->get_move_type( );
		m_tick_base = m_entity->get_tick_base( );
		m_ground_entity = m_entity->get_ground_entity_handle( );
		m_fall_velocity = m_entity->get_fall_velocity( ) ? *m_entity->get_fall_velocity( ) : 0.f;
		m_stamina = m_entity->get_stamina( );
		m_duck_amount = m_entity->get_duck_amount( );
		m_duck_speed = m_entity->get_duck_speed( );
		m_buttons = m_entity->get_buttons( ) ? *m_entity->get_buttons( ) : 0;
		m_button_last = m_entity->get_button_last( );
		m_button_pressed = m_entity->get_button_pressed( );
		m_button_released = m_entity->get_button_released( );

		if ( g_interfaces.m_global_vars_base ) {
			m_has_globals = true;
			m_current_time = g_interfaces.m_global_vars_base->m_current_time;
			m_frame_time = g_interfaces.m_global_vars_base->m_frame_time;
			m_tick_count = g_interfaces.m_global_vars_base->m_tick_count;
		}

		if ( g_interfaces.m_prediction ) {
			m_has_prediction = true;
			m_first_time_predicted = g_interfaces.m_prediction->m_is_first_time_predicted;
			m_in_prediction = g_interfaces.m_prediction->m_in_prediction;
			m_commands_predicted = g_interfaces.m_prediction->m_commands_predicted;
		}

		if ( m_cmd ) {
			m_cmd_view = m_cmd->m_view_point;
			m_cmd_forward = m_cmd->m_forward_move;
			m_cmd_side = m_cmd->m_side_move;
			m_cmd_up = m_cmd->m_up_move;
			m_cmd_buttons = m_cmd->m_buttons;
			m_cmd_impulse = m_cmd->m_impulse;
			m_cmd_mouse_x = m_cmd->m_mouse_delta_x;
			m_cmd_mouse_y = m_cmd->m_mouse_delta_y;
		}
	}

	~ScopedMovementAssistState( )
	{
		restore( );
	}

	void restore( )
	{
		if ( !m_valid || !m_entity )
			return;

		m_entity->get_origin( ) = m_origin;
		m_entity->set_abs_origin( m_abs_origin );
		m_entity->get_velocity( ) = m_velocity;
		m_entity->get_base_velocity( ) = m_base_velocity;
		m_entity->get_flags( ) = m_flags;
		m_entity->get_eflags( ) = m_eflags;
		m_entity->get_move_type( ) = m_move_type;
		m_entity->get_tick_base( ) = m_tick_base;
		m_entity->get_ground_entity_handle( ) = m_ground_entity;
		if ( m_entity->get_fall_velocity( ) )
			*m_entity->get_fall_velocity( ) = m_fall_velocity;
		m_entity->get_stamina( ) = m_stamina;
		m_entity->get_duck_amount( ) = m_duck_amount;
		m_entity->get_duck_speed( ) = m_duck_speed;
		if ( m_entity->get_buttons( ) )
			*m_entity->get_buttons( ) = m_buttons;
		m_entity->get_button_last( ) = m_button_last;
		m_entity->get_button_pressed( ) = m_button_pressed;
		m_entity->get_button_released( ) = m_button_released;

		if ( m_has_globals && g_interfaces.m_global_vars_base ) {
			g_interfaces.m_global_vars_base->m_current_time = m_current_time;
			g_interfaces.m_global_vars_base->m_frame_time = m_frame_time;
			g_interfaces.m_global_vars_base->m_tick_count = m_tick_count;
		}

		if ( m_has_prediction && g_interfaces.m_prediction ) {
			g_interfaces.m_prediction->m_is_first_time_predicted = m_first_time_predicted;
			g_interfaces.m_prediction->m_in_prediction = m_in_prediction;
			g_interfaces.m_prediction->m_commands_predicted = m_commands_predicted;
		}

		if ( m_restore_cmd && m_cmd ) {
			m_cmd->m_view_point = m_cmd_view;
			m_cmd->m_forward_move = m_cmd_forward;
			m_cmd->m_side_move = m_cmd_side;
			m_cmd->m_up_move = m_cmd_up;
			m_cmd->m_buttons = m_cmd_buttons;
			m_cmd->m_impulse = m_cmd_impulse;
			m_cmd->m_mouse_delta_x = m_cmd_mouse_x;
			m_cmd->m_mouse_delta_y = m_cmd_mouse_y;
		}

		m_valid = false;
	}

	ScopedMovementAssistState( const ScopedMovementAssistState& ) = delete;
	ScopedMovementAssistState& operator=( const ScopedMovementAssistState& ) = delete;

private:
	bool m_valid = false;
	c_base_entity* m_entity = nullptr;
	c_user_cmd* m_cmd = nullptr;
	bool m_restore_cmd = false;

	c_vector m_origin{ };
	c_vector m_abs_origin{ };
	c_vector m_velocity{ };
	c_vector m_base_velocity{ };
	int m_flags = 0;
	int m_eflags = 0;
	int m_move_type = 0;
	int m_tick_base = 0;
	unsigned int m_ground_entity = 0;
	float m_fall_velocity = 0.f;
	float m_stamina = 0.f;
	float m_duck_amount = 0.f;
	float m_duck_speed = 0.f;
	int m_buttons = 0;
	int m_button_last = 0;
	int m_button_pressed = 0;
	int m_button_released = 0;

	bool m_has_globals = false;
	float m_current_time = 0.f;
	float m_frame_time = 0.f;
	int m_tick_count = 0;

	bool m_has_prediction = false;
	bool m_first_time_predicted = false;
	bool m_in_prediction = false;
	int m_commands_predicted = 0;

	c_angle m_cmd_view{ };
	float m_cmd_forward = 0.f;
	float m_cmd_side = 0.f;
	float m_cmd_up = 0.f;
	int m_cmd_buttons = 0;
	unsigned char m_cmd_impulse = 0;
	short m_cmd_mouse_x = 0;
	short m_cmd_mouse_y = 0;
};
