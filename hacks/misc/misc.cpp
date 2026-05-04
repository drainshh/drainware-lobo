#include "misc.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/branding/branding.h"
#include "../../globals/includes/includes.h"
#include "../../globals/logger/logger.h"
#include "../../resources/embedded/watermark_assets.h"
#ifndef LOBO_USE_IMPLOT
#define LOBO_USE_IMPLOT 1
#endif
#if LOBO_USE_IMPLOT
#include "../../third_party/implot/implot.h"
#endif
#include "../auto_wall/auto_wall.h"
#include "../avatar_cache/avatar_cache.h"
#include "../entity_cache/entity_cache.h"
#include "../movement/movement_assist_simulation.h"
#include "../movement/movement.h"
#include "../instalisation fonts/fonts2.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	using set_clantag_fn = int( __fastcall* )( const char*, const char* );
	constexpr const char* k_debug_log_file = "C:\\drainware\\debuglog.txt";

	std::string timestamp_string( )
	{
		const auto now = std::chrono::system_clock::now( );
		const auto time = std::chrono::system_clock::to_time_t( now );
		std::tm local_time{ };
		localtime_s( &local_time, &time );

		std::ostringstream out;
		out << std::put_time( &local_time, "%H:%M:%S" );
		return out.str( );
	}

	float current_real_time( )
	{
		return g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_real_time : 0.f;
	}

	void debug_log( const char* feature, const std::string& message, const bool category_enabled, const float rate_limit = 0.f,
	                const char* rate_key = nullptr )
	{
		if ( !GET_VARIABLE( g_variables.m_debug_logging, bool ) || !category_enabled )
			return;

		if ( rate_limit > 0.f && rate_key ) {
			static std::unordered_map< std::string, float > last_log_time;
			const std::string key = std::string( feature ) + ':' + rate_key;
			const float now = current_real_time( );
			const auto it = last_log_time.find( key );
			if ( it != last_log_time.end( ) && now - it->second < rate_limit )
				return;
			last_log_time[ key ] = now;
		}

		std::filesystem::create_directories( "C:\\drainware" );
		std::ofstream file( k_debug_log_file, std::ios::app );
		if ( !file.good( ) )
			return;

		file << '[' << timestamp_string( ) << "][" << feature << "] " << message << '\n';
	}

	std::string vector_string( const c_vector& value )
	{
		std::ostringstream out;
		out << '(' << std::fixed << std::setprecision( 1 ) << value.m_x << ' ' << value.m_y << ' ' << value.m_z << ')';
		return out.str( );
	}

	void clear_debug_file( )
	{
		std::filesystem::create_directories( "C:\\drainware" );
		std::ofstream file( k_debug_log_file, std::ios::trunc );
		if ( file.good( ) )
			file << '[' << timestamp_string( ) << "][debug] log cleared\n";
	}

	set_clantag_fn get_clantag_fn( )
	{
		static auto fn = reinterpret_cast< set_clantag_fn >(
			g_modules.m_modules[ ENGINE_DLL ].find_pattern( "53 56 57 8B DA 8B F9 FF 15" ) );
		return fn;
	}

	void apply_clantag( const std::string& tag )
	{
		if ( const auto fn = get_clantag_fn( ) )
			fn( tag.c_str( ), tag.c_str( ) );
	}

	c_color resolved_watermark_accent( )
	{
		if ( GET_VARIABLE( g_variables.m_watermark_accent_mode, int ) == 1 )
			return GET_VARIABLE( g_variables.m_watermark_custom_accent, c_color );

		return GET_VARIABLE( g_variables.m_accent, c_color );
	}

	c_color movement_print_color( )
	{
		auto color = GET_VARIABLE( g_variables.m_chat_print_color, c_color );
		if ( !GET_VARIABLE( g_variables.m_chat_print_combine_accent, bool ) )
			return color;

		const auto accent = GET_VARIABLE( g_variables.m_accent, c_color );
		return c_color( ( static_cast< int >( color[ 0 ] ) + static_cast< int >( accent[ 0 ] ) ) / 2,
		                ( static_cast< int >( color[ 1 ] ) + static_cast< int >( accent[ 1 ] ) ) / 2,
		                ( static_cast< int >( color[ 2 ] ) + static_cast< int >( accent[ 2 ] ) ) / 2, static_cast< int >( color[ 3 ] ) );
	}

	std::string chat_color_token( const c_color& color, const int mode )
	{
		if ( mode <= 0 )
			return "\x01";

		const int red = color[ 0 ];
		const int green = color[ 1 ];
		const int blue = color[ 2 ];
		if ( green >= red && green >= blue )
			return "\x04";
		if ( blue >= red && blue >= green )
			return "\x0C";
		if ( red >= green && red >= blue )
			return "\x02";

		return "\x01";
	}

	std::string styled_metric_text( const char* label, const std::string& value, int style )
	{
		switch ( std::clamp( style, 0, 3 ) ) {
		case 1:
			return std::string( label ) + "  " + value;
		case 2:
			return std::string( label ) + " | " + value;
		case 3:
			return std::string( label ) + " -> " + value;
		default:
			return std::string( label ) + ": " + value;
		}
	}

	const char* bhop_direction_label( float delta_deg )
	{
		delta_deg = std::remainder( delta_deg, 360.f );
		const float a = std::fabs( delta_deg );
		if ( a < 35.f )
			return "forward";
		if ( a > 145.f )
			return "backwards";
		if ( delta_deg > 0.f )
			return "sideways right";
		return "sideways left";
	}

	void movement_print( const char* event_name, bool respect_master = true )
	{
		if ( respect_master && !GET_VARIABLE( g_variables.m_movement_chat_prints, bool ) )
			return;

		auto prefix = GET_VARIABLE( g_variables.m_chat_print_prefix, std::string );
		if ( prefix.empty( ) )
			prefix = "dgware";

		const auto color = movement_print_color( );
		const int color_mode = std::clamp( GET_VARIABLE( g_variables.m_chat_print_color_mode, int ), 0, 2 );
		if ( g_interfaces.m_hud_chat ) {
			const std::string token = chat_color_token( color, color_mode );
			const std::string message = token + prefix + " \x01| " + token + event_name;
			g_interfaces.m_hud_chat->chat_printf( "%s", message.c_str( ) );
			debug_log( "chat", std::string( "printed via SayText2 color_mode=" ) +
			                       ( color_mode == 0 ? "normal" : color_mode == 1 ? "accent_mapped" : "custom_mapped" ) +
			                       " text=" + prefix + " | " + event_name,
			           GET_VARIABLE( g_variables.m_debug_log_chat, bool ), 0.25f, "chat_print" );
			return;
		}

		if ( g_interfaces.m_convar ) {
			const c_unsigned_char_color print_color( color[ 0 ], color[ 1 ], color[ 2 ], color[ 3 ] );
			g_interfaces.m_convar->console_color_printf( print_color, "%s | %s\n", prefix.c_str( ), event_name );
			debug_log( "chat", "fallback=console text=" + prefix + " | " + event_name, GET_VARIABLE( g_variables.m_debug_log_chat, bool ), 0.25f,
			           "chat_console" );
		}
	}

	bool safe_play_ui_sound( const std::string& sound_path, const char* feature, const float cooldown )
	{
		drainware_stability_breadcrumb( feature );

		if ( g_in_movement_assist_simulation ) {
			drainware_note_suppressed_sound( "custom_sound_in_assist_simulation", sound_path.c_str( ) );
			debug_log( "sound", std::string( "blocked reason=in_assist_simulation feature=" ) + feature,
			           GET_VARIABLE( g_variables.m_debug_log_sound, bool ), 0.25f, feature );
			return false;
		}

		if ( g_drainware_custom_movement_sounds_muted ) {
			drainware_note_suppressed_sound( "custom_sounds_muted", sound_path.c_str( ) );
			debug_log( "sound", std::string( "blocked reason=custom_sounds_muted feature=" ) + feature,
			           GET_VARIABLE( g_variables.m_debug_log_sound, bool ), 0.25f, feature );
			return false;
		}

		if ( sound_path.empty( ) || !g_interfaces.m_surface ) {
			drainware_note_suppressed_sound( "empty_or_no_surface", sound_path.c_str( ) );
			debug_log( "sound", std::string( "blocked empty_or_no_surface feature=" ) + feature, GET_VARIABLE( g_variables.m_debug_log_sound, bool ), 0.4f,
			           feature );
			return false;
		}

		std::string lowered = sound_path;
		std::transform( lowered.begin( ), lowered.end( ), lowered.begin( ), []( unsigned char c ) { return static_cast< char >( std::tolower( c ) ); } );
		if ( lowered.find( "player\\land" ) != std::string::npos || lowered.find( "player/land" ) != std::string::npos ) {
			drainware_note_suppressed_sound( "blocked_default_land_sound", sound_path.c_str( ) );
			debug_log( "sound", std::string( "blocked default land sound path=" ) + sound_path + " feature=" + feature,
			           GET_VARIABLE( g_variables.m_debug_log_sound, bool ), 0.3f, "blocked_land" );
			return false;
		}

		static std::unordered_map< std::string, float > last_sound_time;
		static float last_global_movement_sound = 0.f;
		const float now = current_real_time( );
		if ( now - last_global_movement_sound < 0.06f ) {
			drainware_note_suppressed_sound( "global_sound_rate_limit", sound_path.c_str( ) );
			debug_log( "sound", std::string( "blocked global_rate_limit sound=" ) + sound_path + " feature=" + feature,
			           GET_VARIABLE( g_variables.m_debug_log_sound, bool ), 0.15f, "global_rate_limit" );
			return false;
		}
		const auto it = last_sound_time.find( lowered );
		if ( it != last_sound_time.end( ) && now - it->second < cooldown ) {
			drainware_note_suppressed_sound( "per_sound_cooldown", sound_path.c_str( ) );
			std::ostringstream message;
			message << "blocked duplicate sound=" << sound_path << " feature=" << feature << " cooldown=" << std::fixed << std::setprecision( 2 )
			        << cooldown << " remaining=" << std::max( 0.f, cooldown - ( now - it->second ) );
			debug_log( "sound", message.str( ), GET_VARIABLE( g_variables.m_debug_log_sound, bool ), 0.15f, lowered.c_str( ) );
			return false;
		}

		last_sound_time[ lowered ] = now;
		last_global_movement_sound = now;
		g_interfaces.m_surface->play_sound( sound_path.c_str( ) );
		debug_log( "sound", std::string( "played sound=" ) + sound_path + " feature=" + feature, GET_VARIABLE( g_variables.m_debug_log_sound, bool ),
		           0.15f, lowered.c_str( ) );
		return true;
	}

	struct particle_option_t {
		const char* label;
		const char* name;
		bool local_effect;
	};

	static constexpr std::array< particle_option_t, 22 > k_particle_options{
		particle_option_t{ "off", "", false },
		particle_option_t{ "surface-based", "surface-based", false },
		particle_option_t{ "blood impact", "blood_impact_basic", false },
		particle_option_t{ "blood mist", "blood_impact_mist1", false },
		particle_option_t{ "feathers", "chicken_gone_feathers", false },
		particle_option_t{ "confetti A", "confetti_A", false },
		particle_option_t{ "confetti balloons", "confetti_balloons", false },
		particle_option_t{ "dust devil", "dust_devil", false },
		particle_option_t{ "water splash", "water_splash_leakypipe_vertical", false },
		particle_option_t{ "baggage drip", "baggage_leak_drip", false },
		particle_option_t{ "molotov fireball", "molotov_explosion_child_fireball1", false },
		particle_option_t{ "weapon confetti", "weapon_confetti", false },
		particle_option_t{ "engine dust", "engine_dust", true },
		particle_option_t{ "engine sparks", "engine_sparks", true },
		particle_option_t{ "engine smoke", "engine_smoke", true },
		particle_option_t{ "engine ring beam", "engine_ring_beam", true },
		particle_option_t{ "engine lightning beam", "engine_lightning_beam", true },
		particle_option_t{ "ambient sparks core", "ambient_sparks_core", false },
		particle_option_t{ "weapon confetti sparks", "weapon_confetti_sparks", false },
		particle_option_t{ "dust devil smoke", "dust_devil_smoke", false },
		particle_option_t{ "dust devil swirls", "dust_devil_swirls", false },
		particle_option_t{ "baggage splash", "baggage_leak_splash", false },
	};

	static constexpr std::array< const char*, 8 > k_tracer_particle_names{
		"engine_lightning_beam", "engine_ring_beam", "engine_sparks", "weapon_confetti", "weapon_confetti_sparks",
		"ambient_sparks_core",  "confetti_A",       "dust_devil_swirls",
	};

	static constexpr std::array< const char*, 10 > k_known_particle_validation_names{
		"blood_impact_basic",   "blood_impact_mist1", "confetti_A",         "confetti_balloons", "ambient_sparks_core",
		"baggage_leak_drip",   "chicken_gone_feathers", "dust_devil_smoke", "weapon_confetti",   "weapon_confetti_sparks",
	};

	const particle_option_t& particle_option( int mode )
	{
		mode = std::clamp( mode, 0, static_cast< int >( k_particle_options.size( ) ) - 1 );
		return k_particle_options[ mode ];
	}

	const particle_option_t* find_particle_option_by_name( const char* name )
	{
		if ( !name || !name[ 0 ] )
			return nullptr;

		for ( const auto& option : k_particle_options ) {
			if ( option.name[ 0 ] && std::strcmp( option.name, name ) == 0 )
				return &option;
		}

		return nullptr;
	}

	const char* particle_reference_name( int mode )
	{
		return particle_option( mode ).name;
	}

	const char* tracer_particle_name( int mode )
	{
		mode = std::clamp( mode, 0, static_cast< int >( k_tracer_particle_names.size( ) ) - 1 );
		return k_tracer_particle_names[ mode ];
	}

	int resolve_surface_particle_mode( const c_vector& origin );

	bool invalid_string_table_index( const int index )
	{
		return index < 0 || index == 0xffff;
	}

	bool valid_particle_system_name( const char* particle_name )
	{
		if ( !particle_name || !particle_name[ 0 ] )
			return false;

		const std::size_t length = std::strlen( particle_name );
		if ( length > 96 || std::strstr( particle_name, ".pcf" ) )
			return false;

		for ( std::size_t i = 0; i < length; ++i ) {
			const unsigned char c = static_cast< unsigned char >( particle_name[ i ] );
			if ( !std::isalnum( c ) && c != '_' && c != '-' && c != '.' )
				return false;
		}

		return true;
	}

	struct csgo_particle_precache_t {
		bool ok = false;
		bool from_cache = false;
		int index = 0xffff;
		std::string reason;
	};

	std::string current_map_name( );

	c_network_string_table* particle_effect_names_table( )
	{
		static c_network_string_table* table = nullptr;
		static bool searched = false;

		if ( searched && table )
			return table;

		if ( !g_interfaces.m_network_string_table_container )
			return nullptr;

		searched = true;
		table = g_interfaces.m_network_string_table_container->find_table( "ParticleEffectNames" );
		return table;
	}

	using precache_particle_system_fn = int( __fastcall* )( const char*, void* );

	precache_particle_system_fn precache_particle_system( )
	{
		static auto fn = reinterpret_cast< precache_particle_system_fn >( g_modules[ CLIENT_DLL ].find_pattern(
			"56 57 8B F9 8B 0D ? ? ? ? 6A 00 6A FF 57 8B 01 6A 00 FF 50 20 8B F0 57 56" ) );
		return fn;
	}

	csgo_particle_precache_t precache_csgo_particle( const char* particle_name )
	{
		static std::unordered_map< std::string, csgo_particle_precache_t > cache;

		if ( !valid_particle_system_name( particle_name ) ) {
			return csgo_particle_precache_t{ false, false, 0xffff, "invalid_particle_system_name" };
		}

		const std::string cache_key = particle_name;
		const auto cached = cache.find( cache_key );
		if ( cached != cache.end( ) ) {
			auto result = cached->second;
			result.from_cache = true;
			return result;
		}

		auto finish = [ & ]( csgo_particle_precache_t result ) {
			cache[ cache_key ] = result;
			return result;
		};

		const auto table = particle_effect_names_table( );
		const std::string map_name = current_map_name( );
		if ( !g_interfaces.m_network_string_table_container ) {
			debug_log( "particles][precache", std::string( "selected=" ) + particle_name +
			                                      " table=ParticleEffectNames failed=no_string_table_container map=" + map_name,
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.5f, particle_name );
			return finish( { false, false, 0xffff, "no_string_table_container" } );
		}
		if ( !table ) {
			debug_log( "particles][precache", std::string( "selected=" ) + particle_name +
			                                      " table=ParticleEffectNames failed=particle_effect_names_table_missing map=" + map_name,
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.5f, particle_name );
			return finish( { false, false, 0xffff, "particle_effect_names_table_missing" } );
		}

		const int table_count_before = table->get_num_strings( );
		const int index_before = table->find_string_index( particle_name );
		const bool found_before = !invalid_string_table_index( index_before );

		int manual_add_index = index_before;
		std::string add_status = found_before ? "skipped_present" : "not_needed";
		if ( !found_before ) {
			manual_add_index = table->add_string( false, particle_name );
			add_status = invalid_string_table_index( manual_add_index ) ? "manual_failed" : "ok";
		}

		const auto particle_mgr = precache_particle_system( );
		if ( !particle_mgr ) {
			int index_without_manager = table->find_string_index( particle_name );
			if ( invalid_string_table_index( index_without_manager ) )
				index_without_manager = manual_add_index;

			const int table_count_after_missing_manager = table->get_num_strings( );
			debug_log( "particles][precache",
			           std::string( "selected=" ) + particle_name + " table=ParticleEffectNames found=" + ( found_before ? "true" : "false" ) +
			               " table_count_before=" + std::to_string( table_count_before ) + " table_count_after=" +
			               std::to_string( table_count_after_missing_manager ) + " add_string=" + add_status +
			               " index=" + std::to_string( index_without_manager ) + " particle_mgr=missing result=" +
			               ( invalid_string_table_index( index_without_manager ) ? "failed" : "ok_string_table_only" ) + " map=" + map_name,
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.5f, particle_name );

			if ( invalid_string_table_index( index_without_manager ) )
				return finish( { false, false, 0xffff, "particle_mgr_missing" } );

			return finish( { true, false, index_without_manager, "ok_string_table_only" } );
		}

		const int manager_index = particle_mgr( particle_name, nullptr );
		int index_after = table->find_string_index( particle_name );
		if ( invalid_string_table_index( index_after ) && !invalid_string_table_index( manager_index ) )
			index_after = manager_index;

		const int table_count_after = table->get_num_strings( );
		if ( !found_before && invalid_string_table_index( manual_add_index ) && !invalid_string_table_index( index_after ) )
			add_status = "ok_via_particle_mgr";

		const bool manager_ok = !invalid_string_table_index( manager_index );
		const bool result_ok = !invalid_string_table_index( index_after );
		std::string failure_stage;
		if ( !result_ok ) {
			if ( add_status == "manual_failed" )
				failure_stage = "add_string_failed";
			else if ( !manager_ok )
				failure_stage = "particle_mgr_precache_failed";
			else
				failure_stage = "unknown_precache_failure";
		}

		debug_log( "particles][precache",
		           std::string( "selected=" ) + particle_name + " table=ParticleEffectNames found=" + ( found_before ? "true" : "false" ) +
		               " table_count_before=" + std::to_string( table_count_before ) + " table_count_after=" + std::to_string( table_count_after ) +
		               " add_string=" + add_status + " index=" + std::to_string( index_after ) +
		               " particle_mgr=" + ( manager_ok ? "ok" : "failed" ) + " manager_index=" + std::to_string( manager_index ) +
		               " result=" + ( result_ok ? "ok" : "failed" ) + ( result_ok ? "" : " stage=" + failure_stage ) + " map=" + map_name +
		               ( result_ok ? "" : " manifest_hint=particle_may_require_pcf_or_map_event" ),
		           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), result_ok ? 0.25f : 0.5f, particle_name );

		if ( !result_ok )
			return finish( { false, false, 0xffff, failure_stage } );

		return finish( { true, false, index_after, "ok" } );
	}

	using dispatch_particle_origin_fn = void( __fastcall* )( const char*, void*, float, float, float, float, float, float );
	using dispatch_particle_tracer_fn = void( __fastcall* )( int, const c_vector*, const c_vector*, int, c_base_entity*, int );

	dispatch_particle_origin_fn dispatch_particle_origin( )
	{
		static auto fn = reinterpret_cast< dispatch_particle_origin_fn >( g_modules[ CLIENT_DLL ].find_pattern(
			"55 8B EC 83 E4 F8 81 EC 8C 00 00 00 56 8B F1 85 F6 74 25 8B 0D ? ? ? ? 56 8B 01 FF 50 30" ) );
		return fn;
	}

	dispatch_particle_tracer_fn dispatch_particle_tracer( )
	{
		static auto fn = reinterpret_cast< dispatch_particle_tracer_fn >(
			g_modules[ CLIENT_DLL ].find_pattern( "55 8B EC 83 EC 6C 56 8B F1 8D 4D 98 E8 ? ? ? ? F3 0F 10 02" ) );
		return fn;
	}

	bool dispatch_csgo_particle( const char* feature, const char* particle_name, const c_vector& origin, const c_angle& angles )
	{
		const auto precache = precache_csgo_particle( particle_name );
		if ( !precache.ok ) {
			g_drainware_particle_health = std::string( "missing: " ) + ( particle_name ? particle_name : "<null>" ) +
			                              " (" + precache.reason + ")";
			debug_log( feature, std::string( "selected=" ) + ( particle_name ? particle_name : "<null>" ) + " failed=precache_failed reason=" +
			                       precache.reason + " origin=" + vector_string( origin ),
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.25f, particle_name ? particle_name : "invalid_particle" );
			return false;
		}

		const auto fn = dispatch_particle_origin( );
		if ( !fn ) {
			g_drainware_particle_health = std::string( "dispatch failed: " ) + particle_name;
			debug_log( feature, std::string( "selected=" ) + particle_name + " precache=ok index=" + std::to_string( precache.index ) +
			                       " failed=dispatch_failed reason=no_dispatch_interface origin=" + vector_string( origin ),
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.25f, particle_name );
			return false;
		}

		fn( particle_name, nullptr, origin.m_x, origin.m_y, origin.m_z, angles.m_x, angles.m_y, angles.m_z );
		g_drainware_particle_health = std::string( "OK: " ) + particle_name;
		debug_log( feature, std::string( "selected=" ) + particle_name + " precache=ok" + ( precache.from_cache ? " cache=hit" : "" ) +
		                       " index=" + std::to_string( precache.index ) + " dispatch=ok origin=" + vector_string( origin ),
		           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.08f, particle_name );
		return true;
	}

	bool dispatch_csgo_particle_tracer( const char* feature, const char* particle_name, const c_vector& start, const c_vector& end )
	{
		const auto precache = precache_csgo_particle( particle_name );
		if ( !precache.ok ) {
			g_drainware_particle_health = std::string( "missing tracer: " ) + ( particle_name ? particle_name : "<null>" ) +
			                              " (" + precache.reason + ")";
			debug_log( feature, std::string( "selected=" ) + ( particle_name ? particle_name : "<null>" ) + " failed=precache_failed reason=" +
			                       precache.reason + " start=" + vector_string( start ) + " end=" + vector_string( end ),
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.25f, particle_name ? particle_name : "invalid_particle" );
			return false;
		}

		const auto fn = dispatch_particle_tracer( );
		if ( !fn ) {
			g_drainware_particle_health = std::string( "tracer dispatch failed: " ) + particle_name;
			debug_log( feature, std::string( "selected=" ) + particle_name + " precache=ok index=" + std::to_string( precache.index ) +
			                       " failed=dispatch_failed reason=no_tracer_dispatch_interface start=" + vector_string( start ) +
			                       " end=" + vector_string( end ),
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.25f, particle_name );
			return false;
		}

		fn( precache.index, &start, &end, 0, nullptr, 0 );
		g_drainware_particle_health = std::string( "OK tracer: " ) + particle_name;
		debug_log( feature, std::string( "selected=" ) + particle_name + " precache=ok" + ( precache.from_cache ? " cache=hit" : "" ) +
		                       " index=" + std::to_string( precache.index ) + " dispatch=ok start=" + vector_string( start ) +
		                       " end=" + vector_string( end ),
		           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.08f, particle_name );
		return true;
	}

	void emit_beam_line( const c_vector& start, const c_vector& end, const c_color& color, float life, float width, float noise )
	{
		if ( !g_interfaces.m_effects || !g_interfaces.m_model_info )
			return;

		const int beam_model = g_interfaces.m_model_info->get_model_index( "sprites/physbeam.vmt" );
		const int halo_model = g_interfaces.m_model_info->get_model_index( "sprites/glow01.vmt" );
		if ( beam_model <= 0 )
			return;

		const auto alpha = static_cast< unsigned char >( std::clamp( static_cast< int >( color[ 3 ] ), 24, 255 ) );
		g_interfaces.m_effects->beam( start, end, beam_model, halo_model, 0, 16, std::clamp( life, 0.05f, 2.5f ),
		                              static_cast< unsigned char >( std::clamp( width, 1.f, 24.f ) ),
		                              static_cast< unsigned char >( std::clamp( width * 0.65f, 1.f, 24.f ) ), 0,
		                              static_cast< unsigned char >( std::clamp( noise, 0.f, 32.f ) ), color[ 0 ], color[ 1 ], color[ 2 ],
		                              alpha, 12 );
	}

	bool emit_engine_particle( const char* feature, const particle_option_t& option, const c_vector& origin, const c_vector* end,
	                           const c_color& color, float lifetime, float intensity )
	{
		if ( !g_interfaces.m_effects )
			return false;

		intensity = std::clamp( intensity, 0.25f, 3.0f );
		const c_vector up_dir{ 0.f, 0.f, 1.f };
		const c_vector effect_origin = origin + c_vector( 0.f, 0.f, 1.5f );

		if ( strcmp( option.name, "engine_dust" ) == 0 ) {
			g_interfaces.m_effects->dust( effect_origin, up_dir, 7.f * intensity, 12.f * intensity );
		} else if ( strcmp( option.name, "engine_sparks" ) == 0 ) {
			g_interfaces.m_effects->sparks( effect_origin, std::max( 1, static_cast< int >( 2.f * intensity ) ),
			                                std::max( 1, static_cast< int >( 3.f * intensity ) ), nullptr );
		} else if ( strcmp( option.name, "engine_smoke" ) == 0 ) {
			if ( g_interfaces.m_model_info ) {
				const int smoke_model = g_interfaces.m_model_info->get_model_index( "sprites/steam1.vmt" );
				if ( smoke_model > 0 )
					g_interfaces.m_effects->smoke( effect_origin, smoke_model, 0.55f * intensity, 6.f );
				else
					return false;
			}
		} else if ( strcmp( option.name, "engine_ring_beam" ) == 0 ) {
			const float radius = 11.f * intensity;
			emit_beam_line( effect_origin + c_vector( -radius, 0.f, 1.f ), effect_origin + c_vector( radius, 0.f, 1.f ), color, 0.16f, 2.f, 2.f );
			emit_beam_line( effect_origin + c_vector( 0.f, -radius, 1.f ), effect_origin + c_vector( 0.f, radius, 1.f ), color, 0.16f, 2.f, 2.f );
		} else if ( strcmp( option.name, "engine_lightning_beam" ) == 0 ) {
			if ( !end )
				return false;
			emit_beam_line( origin, *end, color, lifetime, 2.5f * intensity, 12.f * intensity );
		} else {
			return false;
		}

		debug_log( feature, std::string( "selected=" ) + option.name + " resolved=" + option.name +
		                       " precache=local dispatch=ok origin=" + vector_string( origin ) + ( end ? " end=" + vector_string( *end ) : "" ),
		           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.08f, option.name );
		return true;
	}

	bool emit_selected_particle( const char* feature, const c_vector& origin, int mode, float intensity, const c_vector* end = nullptr,
	                             c_color color = c_color( 174, 255, 0, 220 ), float lifetime = 0.22f )
	{
		mode = std::clamp( mode, 0, static_cast< int >( k_particle_options.size( ) ) - 1 );
		const auto& option = particle_option( mode );
		if ( mode <= 0 || option.name[ 0 ] == '\0' ) {
			debug_log( feature, "skipped=off", GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.4f, "particle_off" );
			return false;
		}

		if ( mode == 1 ) {
			const int resolved_mode = resolve_surface_particle_mode( origin );
			const auto& resolved = particle_option( resolved_mode );
			debug_log( feature, std::string( "selected=surface-based resolved=" ) + resolved.name + " origin=" + vector_string( origin ),
			           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.12f, "surface_resolve" );
			return emit_selected_particle( feature, origin, resolved_mode, intensity, end, color, lifetime );
		}

		if ( option.local_effect )
			return emit_engine_particle( feature, option, origin, end, color, lifetime, intensity );

		if ( end )
			return dispatch_csgo_particle_tracer( feature, option.name, origin, *end );

		return dispatch_csgo_particle( feature, option.name, origin, c_angle( 0.f, 0.f, 0.f ) );
	}

	bool emit_particle_by_name( const char* feature, const char* particle_name, const c_vector& origin, float intensity,
	                            const c_vector* end = nullptr, c_color color = c_color( 174, 255, 0, 220 ), float lifetime = 0.22f )
	{
		if ( !particle_name || !particle_name[ 0 ] ) {
			debug_log( feature, "skipped=empty_particle_name", GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.4f,
			           "empty_particle_name" );
			return false;
		}

		if ( const auto option = find_particle_option_by_name( particle_name ) ) {
			if ( option->local_effect )
				return emit_engine_particle( feature, *option, origin, end, color, lifetime, intensity );
		}

		if ( end )
			return dispatch_csgo_particle_tracer( feature, particle_name, origin, *end );

		return dispatch_csgo_particle( feature, particle_name, origin, c_angle( 0.f, 0.f, 0.f ) );
	}

	int resolve_surface_particle_mode( const c_vector& origin )
	{
		if ( !g_interfaces.m_engine_trace )
			return 12;

		c_vector end = origin;
		end.m_z -= 48.f;
		trace_t surface_trace{ };
		c_trace_filter_world filter{ };
		g_interfaces.m_engine_trace->trace_ray( ray_t( origin, end ), mask_playersolid, &filter, &surface_trace );

		const char* surface_name = nullptr;
		if ( surface_trace.m_fraction < 1.f && g_interfaces.m_physics_surface_props )
			surface_name = g_interfaces.m_physics_surface_props->get_prop_name( surface_trace.surface.m_surface_props );

		const std::string surface = surface_name ? surface_name : "";
		if ( surface.find( "metal" ) != std::string::npos || surface.find( "grate" ) != std::string::npos )
			return 13;
		if ( surface.find( "snow" ) != std::string::npos )
			return 14;
		if ( surface.find( "water" ) != std::string::npos || surface.find( "slosh" ) != std::string::npos )
			return 15;
		if ( surface.find( "dirt" ) != std::string::npos || surface.find( "grass" ) != std::string::npos )
			return 12;

		const float now = g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_real_time : 0.f;
		return 12 + static_cast< int >( std::fmod( now * 2.0f, 4.f ) );
	}

	std::string current_map_name( )
	{
		if ( !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_in_game( ) )
			return {};

		const char* level = g_interfaces.m_engine_client->get_level_name_short( );
		return level ? level : "";
	}

	bool g_px_delete_nearest_requested = false;
	bool g_px_clear_current_map_requested = false;
	int g_particle_debug_request = -1;
	bool g_ambient_restore_requested = false;
}

void n_misc::impl_t::request_px_delete_nearest( )
{
	g_px_delete_nearest_requested = true;
}

void n_misc::impl_t::request_px_clear_current_map( )
{
	g_px_clear_current_map_requested = true;
}

void n_misc::impl_t::clear_debug_log( )
{
	clear_debug_file( );
}

void n_misc::impl_t::request_particle_debug_test( const int mode )
{
	g_particle_debug_request = mode;
}

void n_misc::impl_t::request_ambient_light_restore( )
{
	g_ambient_restore_requested = true;
}

void n_misc::impl_t::request_panic_restore( )
{
	g_drainware_panic_restore_requested = true;
}

void n_misc::impl_t::panic_restore( )
{
	drainware_stability_breadcrumb( "panic_restore" );
	g_drainware_custom_movement_sounds_muted = true;
	g_drainware_sound_spam_detected = false;
	g_drainware_suppressed_sound_count = 0;
	g_drainware_last_suppressed_sound = "panic reset";

	GET_VARIABLE( g_variables.edge_bug, bool ) = false;
	GET_VARIABLE( g_variables.m_pixel_surf, bool ) = false;
	GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) = false;
	GET_VARIABLE( g_variables.m_bouncee_assist, bool ) = false;
	GET_VARIABLE( g_variables.m_jump_bug, bool ) = false;
	GET_VARIABLE( g_variables.m_air_stuck, bool ) = false;
	GET_VARIABLE( g_variables.m_blockbot, bool ) = false;
	GET_VARIABLE( g_variables.m_deagle_spinner, bool ) = false;
	GET_VARIABLE( g_variables.m_footstep_fx_enabled, bool ) = false;
	GET_VARIABLE( g_variables.m_edgebug_particles, bool ) = false;
	GET_VARIABLE( g_variables.m_bullet_tracer, bool ) = false;
	GET_VARIABLE( g_variables.m_jump_feedback_sound, bool ) = false;
	GET_VARIABLE( g_variables.m_old_edit_vibes_sound, bool ) = false;
	GET_VARIABLE( g_variables.m_world_modulation, bool ) = false;
	GET_VARIABLE( g_variables.m_bloom, bool ) = false;
	GET_VARIABLE( g_variables.m_engine_ambient_light, bool ) = false;
	GET_VARIABLE( g_variables.m_dof, bool ) = false;
	GET_VARIABLE( g_variables.m_px_database_auto_record, bool ) = false;
	g_ambient_restore_requested = true;
	g_ctx.force_full_update = true;

	g_drainware_particle_health = "cooldowns reset";
	g_drainware_world_health = "restore requested";
	g_drainware_last_error_line = "panic restore executed";
	movement_assist_debug_log( "stability", "panic_restore disabled risky movement/sound/visual systems restore_requested=1", 0.f, nullptr );
}

n_misc::impl_t::health_snapshot_t n_misc::impl_t::get_health_snapshot( )
{
	health_snapshot_t snapshot{ };
	snapshot.particles = g_drainware_particle_health.empty( ) ? "unknown" : g_drainware_particle_health;
	snapshot.sound_guard = g_drainware_sound_spam_detected ? "spam detected" : g_drainware_custom_movement_sounds_muted ? "muted" : "OK";
	snapshot.prediction_guard = g_in_movement_assist_simulation ? std::string( "active: " ) + g_movement_assist_simulation_reason : "inactive";
	snapshot.world_modulation = GET_VARIABLE( g_variables.m_world_modulation, bool ) ? "applied/enabled" : "restored/off";
	snapshot.inventory = g_drainware_inventory_health.empty( ) ? "OK" : g_drainware_inventory_health;
	snapshot.config = g_drainware_config_health.empty( ) ? "unknown" : g_drainware_config_health;
	snapshot.last_error = g_drainware_last_error_line.empty( ) ? "none" : g_drainware_last_error_line;
	snapshot.last_action = g_drainware_last_stability_action.empty( ) ? "none" : g_drainware_last_stability_action;
	snapshot.last_suppressed_sound = g_drainware_last_suppressed_sound.empty( ) ? "none" : g_drainware_last_suppressed_sound;
	snapshot.suppressed_sounds = g_drainware_suppressed_sound_count;
	snapshot.custom_sounds_muted = g_drainware_custom_movement_sounds_muted;
	return snapshot;
}

std::string n_misc::impl_t::debug_summary( )
{
	const auto health = get_health_snapshot( );
	std::ostringstream out;
	out << "particles: " << health.particles << '\n'
	    << "sound guard: " << health.sound_guard << '\n'
	    << "prediction guard: " << health.prediction_guard << '\n'
	    << "world modulation: " << health.world_modulation << '\n'
	    << "inventory: " << health.inventory << '\n'
	    << "config: " << health.config << '\n'
	    << "suppressed sounds: " << health.suppressed_sounds << '\n'
	    << "last suppressed sound: " << health.last_suppressed_sound << '\n'
	    << "last action: " << health.last_action << '\n'
	    << "last error: " << health.last_error;
	return out.str( );
}

bool n_misc::impl_t::save_stable_snapshot( )
{
	drainware_stability_breadcrumb( "config_save_stable_snapshot" );
	const bool ok = g_config.save( "stable_snapshot" );
	g_drainware_config_health = ok ? "stable snapshot saved" : "stable snapshot save failed";
	if ( !ok )
		drainware_stability_error( "config", "stable snapshot save failed" );
	return ok;
}

bool n_misc::impl_t::restore_stable_snapshot( )
{
	drainware_stability_breadcrumb( "config_restore_stable_snapshot" );
	const bool ok = g_config.load( std::string( "stable_snapshot" ) + n_branding::k_config_extension );
	g_drainware_config_health = ok ? "stable snapshot restored" : "stable snapshot restore failed";
	if ( !ok )
		drainware_stability_error( "config", "stable snapshot restore failed" );
	return ok;
}

void n_misc::impl_t::restore_defaults( )
{
	drainware_stability_breadcrumb( "restore_safe_defaults" );
	GET_VARIABLE( g_variables.m_debug_logging, bool ) = false;
	GET_VARIABLE( g_variables.m_footstep_fx_enabled, bool ) = false;
	GET_VARIABLE( g_variables.m_edgebug_particles, bool ) = false;
	GET_VARIABLE( g_variables.m_bullet_tracer, bool ) = false;
	GET_VARIABLE( g_variables.m_bloom, bool ) = false;
	GET_VARIABLE( g_variables.m_engine_ambient_light, bool ) = false;
	GET_VARIABLE( g_variables.m_dof, bool ) = false;
	GET_VARIABLE( g_variables.m_world_modulation, bool ) = false;
	GET_VARIABLE( g_variables.m_run_analyzer, bool ) = false;
	GET_VARIABLE( g_variables.m_old_edit_vibes_sound, bool ) = false;
	GET_VARIABLE( g_variables.m_jump_feedback_sound, bool ) = false;
	GET_VARIABLE( g_variables.m_px_database_auto_record, bool ) = true;
	GET_VARIABLE( g_variables.m_accent, c_color ) = c_color( 174, 255, 0, 255 );
	g_ambient_restore_requested = true;
	g_drainware_config_health = "safe defaults restored";
}

std::vector< std::string > n_misc::impl_t::bind_conflicts( const bool only_active )
{
	struct bind_entry_t {
		const char* name;
		key_bind_t bind;
		bool active;
		bool important;
	};

	const std::array< bind_entry_t, 17 > binds{ {
		{ "edge jump", GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ), GET_VARIABLE( g_variables.m_edge_jump, bool ), false },
		{ "long jump", GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ), GET_VARIABLE( g_variables.m_long_jump, bool ), false },
		{ "mini jump", GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ), GET_VARIABLE( g_variables.m_mini_jump, bool ), false },
		{ "jumpbug", GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ), GET_VARIABLE( g_variables.m_jump_bug, bool ), true },
		{ "edgebug", GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ), GET_VARIABLE( g_variables.edge_bug, bool ), true },
		{ "pixelsurf", GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ), GET_VARIABLE( g_variables.m_pixel_surf, bool ), true },
		{ "pixelsurf assist", GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ),
		  GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ), true },
		{ "pixelsurf point", GET_VARIABLE( g_variables.m_pixel_surf_assist_point_key, key_bind_t ),
		  GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ), false },
		{ "bounce assist", GET_VARIABLE( g_variables.m_bounce_assist_key, key_bind_t ), GET_VARIABLE( g_variables.m_bouncee_assist, bool ), true },
		{ "bounce point", GET_VARIABLE( g_variables.m_bounce_assist_point_key, key_bind_t ), GET_VARIABLE( g_variables.m_bouncee_assist, bool ), false },
		{ "fire man", GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ), GET_VARIABLE( g_variables.m_fire_man, bool ), false },
		{ "ladder bug", GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ), GET_VARIABLE( g_variables.m_ladder_bug, bool ), false },
		{ "airstuck", GET_VARIABLE( g_variables.m_air_stuck_key, key_bind_t ), GET_VARIABLE( g_variables.m_air_stuck, bool ), true },
		{ "deagle spinner", GET_VARIABLE( g_variables.m_deagle_spinner_key, key_bind_t ), GET_VARIABLE( g_variables.m_deagle_spinner, bool ), false },
		{ "blockbot", GET_VARIABLE( g_variables.m_blockbot_key, key_bind_t ), GET_VARIABLE( g_variables.m_blockbot, bool ), false },
		{ "practice cp", GET_VARIABLE( g_variables.m_practice_cp_key, key_bind_t ), GET_VARIABLE( g_variables.m_practice_window, bool ), false },
		{ "panic", GET_VARIABLE( g_variables.m_panic_key, key_bind_t ), true, true },
	} };

	std::unordered_map< int, std::vector< const bind_entry_t* > > by_key;
	std::vector< std::string > out;
	for ( const auto& bind : binds ) {
		if ( only_active && !bind.active )
			continue;
		if ( bind.bind.m_key <= 0 ) {
			if ( bind.important && bind.active )
				out.emplace_back( std::string( "unset: " ) + bind.name );
			continue;
		}
		by_key[ bind.bind.m_key ].push_back( &bind );
	}

	for ( const auto& [ key, entries ] : by_key ) {
		if ( entries.size( ) < 2U )
			continue;

		std::string line = key >= 0 && key < IM_ARRAYSIZE( FILTERED_KEY_NAMES ) ? FILTERED_KEY_NAMES[ key ] : std::to_string( key );
		line += ": ";
		for ( std::size_t i = 0; i < entries.size( ); ++i ) {
			if ( i )
				line += ", ";
			line += entries[ i ]->name;
			line += entries[ i ]->bind.m_key_style == 2 ? " (toggle)" : entries[ i ]->bind.m_key_style == 1 ? " (hold)" : " (always)";
		}
		out.push_back( line );
	}

	if ( out.empty( ) )
		out.emplace_back( "no conflicts" );
	return out;
}

n_misc::impl_t::session_hub_snapshot_t n_misc::impl_t::get_session_hub_snapshot( )
{
	auto snapshot = m_session_hub_snapshot;
	const auto health = get_health_snapshot( );
	snapshot.particle_mode = GET_VARIABLE( g_variables.m_debug_logging, bool ) ? "debug/experimental visible" : "working/default";
	snapshot.last_particle_error = health.particles;
	snapshot.current_map_profile = current_map_name( ).empty( ) ? "none" : current_map_name( );
	if ( const auto table = particle_effect_names_table( ) )
		snapshot.particle_table_count = table->get_num_strings( );
	return snapshot;
}

bool n_misc::impl_t::save_current_map_profile( )
{
	drainware_stability_breadcrumb( "map_profile_save" );
	const std::string map = current_map_name( );
	if ( map.empty( ) ) {
		drainware_stability_error( "map_profile", "save failed reason=no_current_map" );
		return false;
	}

	std::filesystem::create_directories( n_branding::k_config_directory );
	const std::filesystem::path path = std::filesystem::path( n_branding::k_config_directory ) / ( "map_profile_" + map + ".txt" );
	std::ofstream file( path, std::ios::trunc );
	if ( !file.good( ) ) {
		drainware_stability_error( "map_profile", "save failed reason=open_failed map=" + map );
		return false;
	}

	file << "version 1\n"
	     << "px_database " << GET_VARIABLE( g_variables.m_px_database, bool ) << '\n'
	     << "px_auto_record " << GET_VARIABLE( g_variables.m_px_database_auto_record, bool ) << '\n'
	     << "px_distance " << GET_VARIABLE( g_variables.m_px_database_distance, float ) << '\n'
	     << "px_thickness " << GET_VARIABLE( g_variables.m_px_database_thickness, float ) << '\n'
	     << "pixelsurf_assist_radius " << GET_VARIABLE( g_variables.m_pixel_surf_assist_radius, float ) << '\n'
	     << "world_modulation " << GET_VARIABLE( g_variables.m_world_modulation, bool ) << '\n'
	     << "debug_logging " << GET_VARIABLE( g_variables.m_debug_logging, bool ) << '\n'
	     << "particle_favorites " << GET_VARIABLE( g_variables.m_particle_favorites, std::string ) << '\n';

	g_drainware_config_health = "map profile saved: " + map;
	movement_assist_debug_log( "map_profile", "saved map=" + map + " path=" + path.string( ), 0.f, nullptr );
	return true;
}

bool n_misc::impl_t::reload_current_map_profile( )
{
	drainware_stability_breadcrumb( "map_profile_load" );
	const std::string map = current_map_name( );
	if ( map.empty( ) ) {
		drainware_stability_error( "map_profile", "load failed reason=no_current_map" );
		return false;
	}

	const std::filesystem::path path = std::filesystem::path( n_branding::k_config_directory ) / ( "map_profile_" + map + ".txt" );
	std::ifstream file( path );
	if ( !file.good( ) ) {
		drainware_stability_error( "map_profile", "load failed reason=open_failed map=" + map );
		return false;
	}

	std::string key;
	while ( file >> key ) {
		if ( key == "version" ) {
			int ignored = 0;
			file >> ignored;
		} else if ( key == "px_database" ) {
			file >> GET_VARIABLE( g_variables.m_px_database, bool );
		} else if ( key == "px_auto_record" ) {
			file >> GET_VARIABLE( g_variables.m_px_database_auto_record, bool );
		} else if ( key == "px_distance" ) {
			file >> GET_VARIABLE( g_variables.m_px_database_distance, float );
		} else if ( key == "px_thickness" ) {
			file >> GET_VARIABLE( g_variables.m_px_database_thickness, float );
		} else if ( key == "pixelsurf_assist_radius" ) {
			file >> GET_VARIABLE( g_variables.m_pixel_surf_assist_radius, float );
		} else if ( key == "world_modulation" ) {
			file >> GET_VARIABLE( g_variables.m_world_modulation, bool );
		} else if ( key == "debug_logging" ) {
			file >> GET_VARIABLE( g_variables.m_debug_logging, bool );
		} else if ( key == "particle_favorites" ) {
			std::string rest;
			std::getline( file, rest );
			if ( !rest.empty( ) && rest.front( ) == ' ' )
				rest.erase( rest.begin( ) );
			GET_VARIABLE( g_variables.m_particle_favorites, std::string ) = rest;
		}
	}

	g_drainware_config_health = "map profile loaded: " + map;
	movement_assist_debug_log( "map_profile", "loaded map=" + map + " path=" + path.string( ), 0.f, nullptr );
	return true;
}

void n_misc::impl_t::dump_particle_table( )
{
	drainware_stability_breadcrumb( "particle_table_dump" );
	const auto table = particle_effect_names_table( );
	if ( !table ) {
		drainware_stability_error( "particles", "dump failed reason=ParticleEffectNames missing" );
		return;
	}

	const int count = table->get_num_strings( );
	movement_assist_debug_log( "particles", "dump ParticleEffectNames count=" + std::to_string( count ) + " map=" + current_map_name( ), 0.f, nullptr );
	for ( int i = 0; i < count; ++i ) {
		const char* value = table->get_string( i );
		if ( value && value[ 0 ] )
			movement_assist_debug_log( "particles][table", std::to_string( i ) + "=" + value, 0.f, nullptr );
	}
	g_drainware_particle_health = "dumped table count=" + std::to_string( count );
}

void n_misc::impl_t::on_create_move_pre( )
{
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_panic_key, key_bind_t ) ) )
		g_drainware_panic_restore_requested = true;

	if ( g_drainware_panic_restore_requested ) {
		g_drainware_panic_restore_requested = false;
		this->panic_restore( );
	}

	this->clantag_changer( );

	this->disable_post_processing( );

	this->remove_panorama_blur( );

	this->apply_game_visual_convars( );

	this->apply_bloom_settings( );

	this->apply_ambient_light_settings( );

	this->deagle_spinner( );

	this->blockbot( );

	this->practice_window_think( );
}

void n_misc::impl_t::clantag_changer( )
{
	static std::string last_tag;
	static float next_update = 0.f;
	static int animation_frame = 0;
	static int last_mode = -1;
	static std::string spotify_last_text;
	static std::size_t spotify_offset = 0U;
	static int spotify_direction = 1;
	static int spotify_edge_hold_ticks = 0;

	auto set_tag = [ & ]( const std::string& tag ) {
		if ( tag == last_tag )
			return;

		apply_clantag( tag );
		last_tag = tag;
	};

	if ( !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_in_game( ) ) {
		set_tag( "" );
		return;
	}

	const int mode = GET_VARIABLE( g_variables.m_clantag_mode, int );
	if ( mode <= 0 ) {
		set_tag( "" );
		return;
	}

	const float now = g_interfaces.m_global_vars_base->m_real_time;
	const float speed = std::clamp( GET_VARIABLE( g_variables.m_clantag_speed, float ), 0.25f, 4.f );
	const float interval = mode == 2 ? ( 0.45f / speed ) : ( mode == 4 ? ( 0.36f / speed ) : 1.f );
	if ( now < next_update )
		return;

	next_update = now + interval;

	if ( mode != last_mode ) {
		animation_frame = 0;
		spotify_last_text.clear( );
		spotify_offset = 0U;
		spotify_direction = 1;
		spotify_edge_hold_ticks = 0;
		last_mode = mode;
	}

	switch ( mode ) {
	case 1:
		set_tag( n_branding::k_default_clantag );
		break;
	case 2: {
		static constexpr std::array< const char*, 18 > frames{
			"", "", "d", "dg", "dgw", "dgwa", "dgwar", "dgware", n_branding::k_default_clantag, n_branding::k_default_clantag,
			"dgware", "dgwar", "dgwa", "dgw", "dg", "d", "", ""
		};

		set_tag( frames[ animation_frame % frames.size( ) ] );
		animation_frame++;
		break;
	}
	case 3: {
		const auto& custom_tag = GET_VARIABLE( g_variables.m_custom_clantag, std::string );
		set_tag( custom_tag );
		break;
	}
	case 4: {
		std::string spotify_text;
		if ( !strartist.empty( ) && !strtitle.empty( ) )
			spotify_text = strartist + " - " + strtitle;
		else if ( !strtitle.empty( ) )
			spotify_text = strtitle;

		if ( spotify_text.empty( ) ) {
			set_tag( n_branding::k_default_clantag );
			break;
		}

		if ( spotify_text != spotify_last_text ) {
			spotify_last_text = spotify_text;
			spotify_offset = 0U;
			spotify_direction = 1;
			spotify_edge_hold_ticks = 0;
		}

		constexpr std::size_t visible_chars = 18U;
		const std::string prefix = "\xE2\x99\xAA ";
		const auto max_offset = spotify_text.length( ) > visible_chars ? spotify_text.length( ) - visible_chars : 0U;
		const auto safe_offset = std::min( spotify_offset, max_offset );
		std::string spotify_tag = prefix + spotify_text.substr( safe_offset, visible_chars );

		if ( max_offset > 0U ) {
			if ( spotify_edge_hold_ticks > 0 ) {
				--spotify_edge_hold_ticks;
			} else if ( spotify_direction > 0 ) {
				if ( spotify_offset >= max_offset ) {
					spotify_offset = max_offset;
					spotify_direction = -1;
					spotify_edge_hold_ticks = 2;
				} else {
					++spotify_offset;
				}
			} else {
				if ( spotify_offset == 0U ) {
					spotify_direction = 1;
					spotify_edge_hold_ticks = 2;
				} else {
					--spotify_offset;
				}
			}
		}

		set_tag( spotify_tag );
		break;
	}
	default:
		set_tag( "" );
		break;
	}
}

void n_misc::impl_t::on_paint_traverse( )
{
	this->draw_spectating_local( );
	
	
#ifdef _DEBUG
	// TESTING FUNCTION
	[ & ]( const bool run ) {
		if ( !run || !GET_VARIABLE( g_variables.m_debugger_visual, bool ) )
			return;

		float offset = 0.f;

		static auto render_jb_debug = [ & ]( const std::string& text ) {
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_text, std::make_any< text_draw_object_t >(
																				g_render.m_fonts[ e_font_names::font_name_tahoma_bd_12 ],
																				c_vector_2d( g_ctx.m_width / 2, ( g_ctx.m_height / 1.5 ) + offset ),
																				text.c_str( ), ImColor( 1.f, 1.f, 1.f, 1.f ),
																				ImColor( 0.f, 0.f, 0.f, 1.f ), e_text_flags::text_flag_dropshadow ) );
			offset += 10;
		};

		render_jb_debug( std::string( "can_jb = " ).append( std::to_string( g_movement.m_jumpbug_data.m_can_jb ) ) );
		render_jb_debug( std::string( "height_diff = " ).append( std::to_string( g_movement.m_jumpbug_data.m_height_diff ) ) );
		render_jb_debug(
			std::string( "vertical_velocity_at_landing = " ).append( std::to_string( g_movement.m_jumpbug_data.m_vertical_velocity_at_landing ) ) );
		render_jb_debug( std::string( "abs_height_diff = " ).append( std::to_string( g_movement.m_jumpbug_data.m_abs_height_diff ) ) );
		render_jb_debug( std::string( "ticks_till_land = " ).append( std::to_string( g_movement.m_jumpbug_data.m_ticks_till_land ) ) );
	}( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) );
#endif
}

static std::chrono::steady_clock::time_point progressStartTime = std::chrono::steady_clock::now( );
static double calculatedPositionMs                             = 0.0; // ïîçèöèÿ â ìèëëèñåêóíäàõ
static std::string lastTitle                                   = "";  // äëÿ îòñëåæèâàíèÿ ñìåíû òðåêà

void UpdateCalculatedTrackPosition( Player& player )
{
	// Åñëè ïðîèçîøëà ñìåíà òðåêà, ñáðàñûâàåì ïîçèöèþ è âðåìÿ ñòàðòà
	if ( lastTitle != player.Title ) {
		calculatedPositionMs = 0.0;
		progressStartTime    = std::chrono::steady_clock::now( );
		lastTitle            = player.Title;
	}

	// Åñëè òðåê âîñïðîèçâîäèòñÿ, ïðèáàâëÿåì ïðîøåäøåå âðåìÿ
	if ( player.isPlaying ) {
		auto now     = std::chrono::steady_clock::now( );
		auto deltaMs = std::chrono::duration_cast< std::chrono::milliseconds >( now - progressStartTime ).count( );
		calculatedPositionMs += deltaMs;
		progressStartTime = now;
	}
}



// Ïðèìåð îòðèñîâêè ïëååðà ñ èñïîëüçîâàíèåì ImGui è íîâîãî ðàñ÷¸òà ïðîãðåññà
void RenderMediaPlayer( )
{
	// Îáíîâëÿåì ðàññ÷èòàííóþ ïîçèöèþ òðåêà
	UpdateCalculatedTrackPosition( player );

	// Âû÷èñëÿåì ïðîãðåññ, èñïîëüçóÿ äëèòåëüíîñòü òðåêà (TotalTime â ìñ)
	float progress = 0.0f;
	if ( player.TotalTime > 0 ) {
		progress = static_cast< float >( calculatedPositionMs ) / static_cast< float >( player.TotalTime );
		if ( progress > 1.0f )
			progress = 1.0f;
	}

	// Äîáàâëÿåì èíòåðïîëÿöèþ äëÿ ïëàâíîãî äâèæåíèÿ ïðîãðåññà
	static float smoothProgress = 0.0f;
	if ( progress < smoothProgress )
		smoothProgress = progress;
	smoothProgress += ( progress - smoothProgress ) * 0.1f; // êîýôôèöèåíò èíòåðïîëÿöèè (ìîæíî íàñòðîèòü)

	static ImVec2 sz{ };
	int x = g_ctx.m_width;
	int y = g_ctx.m_height;
	if ( GET_VARIABLE( g_variables.m_watermark_mode, int ) > 0 )
		ImGui::SetNextWindowPos( { x - sz.x - 10, 45 } );
	else
		ImGui::SetNextWindowPos( { x - sz.x - 10, 10 } );

	ImGui::Begin( "Media Player", nullptr,
	              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );
	ImGui::PushFont( fonts_for_gui::regular_11 );

	float padding    = 10.0f; // Îòñòóï ìåæäó òåêñòîì è èçîáðàæåíèåì
	float imageWidth = 30.0f; // Øèðèíà èçîáðàæåíèÿ

	// Ðàññ÷èòûâàåì ðàçìåðû òåêñòà
	auto sizex1       = ImGui::CalcTextSize( strartist.c_str( ) ).x;
	auto sizex2       = ImGui::CalcTextSize( strtitle.c_str( ) ).x;
	float windowWidth = ImGui::GetWindowSize( ).x;
	if ( albumArtTexture ) {
		ImGui::SetCursorPos( ImVec2( windowWidth - imageWidth - padding, 3 ) );
		ImGui::Image( albumArtTexture, ImVec2( imageWidth, imageWidth ) );
	}
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex1 - padding, 20 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex1 - padding, 20 } );

	ImGui::TextColored( ImVec4( 0.f, 0.f, 0.f, 1.f ), strartist.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex1 - padding + 1, 19 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex1 - padding + 1, 19 } );
	ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.6f, 1.f ), strartist.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex2 - padding, 5 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex2 - padding, 5 } );

	ImGui::TextColored( ImVec4( 0.f, 0.f, 0.f, 1.f ), strtitle.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex2 - padding + 1, 4 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex2 - padding + 1, 4 } );
	ImGui::TextColored( ImVec4( 1.f, 1.f, 1.f, 1.f ), strtitle.c_str( ) );

	if ( albumArtTexture ) {
		ImGui::PushItemWidth( 158 );
		ImGui::SetCursorPos( { sz.x - imageWidth - padding - 126, 40 } );
		ImGui::ProgressBar( smoothProgress, ImVec2( 0.0f, 2.0f ) );
		ImGui::PopItemWidth( );
	}
	ImGui::Spacing( );
	
	ImGui::SetWindowSize( { 400, 100 } );
	sz = ImGui::GetWindowSize( );

	ImGui::PopFont( );
	ImGui::End( );
}

void RenderWatermark( )
{
	static IDirect3DTexture9* clarity_texture = nullptr;
	static IDirect3DTexture9* fake_drain_texture = nullptr;
	static std::vector< IDirect3DTexture9* > extra_watermark_textures;
	static bool tried_textures = false;
	static float frame_rate = 0.f;
	static float last_fps_update = 0.f;
	static std::array< float, 24 > animated_width{ };
	static std::array< float, 24 > animated_alpha{ };

	if ( !tried_textures && g_interfaces.m_direct_device ) {
		D3DXCreateTextureFromFileInMemory( g_interfaces.m_direct_device, n_embedded_assets::g_clarity_watermark_png,
		                                   static_cast< UINT >( n_embedded_assets::g_clarity_watermark_png_size ), &clarity_texture );
		D3DXCreateTextureFromFileInMemory( g_interfaces.m_direct_device, n_embedded_assets::g_fake_drain_watermark_png,
		                                   static_cast< UINT >( n_embedded_assets::g_fake_drain_watermark_png_size ), &fake_drain_texture );
		extra_watermark_textures.assign( n_embedded_assets::g_extra_watermark_assets_count, nullptr );
		for ( std::size_t i = 0; i < n_embedded_assets::g_extra_watermark_assets_count; ++i ) {
			const auto& asset = n_embedded_assets::g_extra_watermark_assets[ i ];
			if ( asset.data && asset.size > 0U )
				D3DXCreateTextureFromFileInMemory( g_interfaces.m_direct_device, asset.data, static_cast< UINT >( asset.size ),
				                                   &extra_watermark_textures[ i ] );
		}
		tried_textures = true;
	}

	const int watermark_mode_max = static_cast< int >( 3 + n_embedded_assets::g_extra_watermark_assets_count );
	const int mode = std::clamp( GET_VARIABLE( g_variables.m_watermark_mode, int ), 0, watermark_mode_max );
	if ( mode == 0 )
		return;

	const auto now = g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_real_time : 0.f;
	const auto frame_time = g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_frame_time : ImGui::GetIO( ).DeltaTime;
	if ( now - last_fps_update >= 0.5f ) {
		const float safe_frame_time = frame_time > 0.f ? frame_time : 0.0001f;
		frame_rate = ( frame_rate * 0.7f ) + ( ( 1.f / safe_frame_time ) * 0.3f );
		last_fps_update = now;
	}

	std::string nick_label = GET_VARIABLE( g_variables.m_watermark_nickname, std::string );
	if ( nick_label.empty( ) ) {
		player_info_t local_info{ };
		if ( g_interfaces.m_engine_client && g_interfaces.m_engine_client->is_in_game( ) &&
		     g_interfaces.m_engine_client->get_player_info( g_interfaces.m_engine_client->get_local_player( ), &local_info ) &&
		     local_info.m_name[ 0 ] )
			nick_label = std::string( local_info.m_name ).substr( 0, 16 );
		else
			nick_label = "verionfe";
	}
	if ( nick_label.size( ) > 18U )
		nick_label = nick_label.substr( 0U, 18U );

	const bool extra_mode = mode >= 4;
	const int extra_index = mode - 4;
	const std::string label = mode == 2   ? "clarity"
	                          : mode == 3 ? "chroma"
	                          : extra_mode && extra_index >= 0 &&
	                                    extra_index < static_cast< int >( n_embedded_assets::g_extra_watermark_assets_count )
	                              ? n_embedded_assets::g_extra_watermark_assets[ extra_index ].label
	                              : n_branding::k_client_name;
	const std::string sep = "|";
	const std::string fps_number = std::to_string( static_cast< int >( frame_rate ) );
	const std::string fps_label = " fps";
	auto accent_cfg = resolved_watermark_accent( );
	if ( mode == 3 ) {
		const float speed = std::clamp( GET_VARIABLE( g_variables.m_watermark_chroma_speed, float ), 0.15f, 4.f );
		const float strength = std::clamp( GET_VARIABLE( g_variables.m_watermark_chroma_strength, float ), 0.f, 1.f );
		if ( GET_VARIABLE( g_variables.m_theme_accent_gradient, bool ) ) {
			const auto start = GET_VARIABLE( g_variables.m_theme_gradient_start, c_color );
			const auto end = GET_VARIABLE( g_variables.m_theme_gradient_end, c_color );
			const float t = 0.5f + 0.5f * std::sin( now * speed * 0.95f );
			accent_cfg = c_color( static_cast< int >( std::lerp( static_cast< float >( start[ 0 ] ), static_cast< float >( end[ 0 ] ), t ) ),
			                      static_cast< int >( std::lerp( static_cast< float >( start[ 1 ] ), static_cast< float >( end[ 1 ] ), t ) ),
			                      static_cast< int >( std::lerp( static_cast< float >( start[ 2 ] ), static_cast< float >( end[ 2 ] ), t ) ),
			                      255 );
		} else {
			const auto wave = [ & ]( float offset ) {
				return static_cast< int >( 127.f + std::sin( now * speed * 1.6f + offset ) * 127.f * strength );
			};
			accent_cfg = c_color( wave( 0.f ), wave( 2.094f ), wave( 4.188f ), 255 );
		}
	}
	const float background_alpha = std::clamp( GET_VARIABLE( g_variables.m_watermark_background_alpha, float ), 0.15f, 1.0f );
	const float font_size = 12.f;
	auto* font = fonts_for_gui::bolder_11 ? fonts_for_gui::bolder_11 : ImGui::GetFont( );

	const auto label_size = font->CalcTextSizeA( font_size, 4096.f, 0.f, label.c_str( ) );
	const auto sep_size = font->CalcTextSizeA( font_size, 4096.f, 0.f, sep.c_str( ) );
	const auto nick_size = font->CalcTextSizeA( font_size, 4096.f, 0.f, nick_label.c_str( ) );
	const auto fps_number_size = font->CalcTextSizeA( font_size, 4096.f, 0.f, fps_number.c_str( ) );
	const auto fps_label_size = font->CalcTextSizeA( font_size, 4096.f, 0.f, fps_label.c_str( ) );
	const float target_width = std::max( 118.f, 14.f + label_size.x + 9.f + sep_size.x + 8.f + nick_size.x + 8.f + sep_size.x + 8.f +
	                                             fps_number_size.x + 4.f + fps_label_size.x + 10.f );
	const float height = 24.f;
	const float ft = ImGui::GetIO( ).DeltaTime > 0.f ? ImGui::GetIO( ).DeltaTime : 0.016f;
	const int index = std::clamp< int >( mode, 0, static_cast< int >( animated_width.size( ) ) - 1 );
	animated_width[ index ] =
		std::lerp( animated_width[ index ] <= 0.f ? target_width : animated_width[ index ], target_width, std::clamp( ft * 10.f, 0.f, 1.f ) );
	if ( animated_width[ index ] > target_width - 45.f )
		animated_alpha[ index ] = std::lerp( animated_alpha[ index ], 255.f, std::clamp( ft * 10.f, 0.f, 1.f ) );

	const float alpha = std::clamp( animated_alpha[ index ], 0.f, 255.f );
	const float width = animated_width[ index ];
	const ImVec2 min( static_cast< float >( g_ctx.m_width ) - width - 12.f, 10.f );
	const ImVec2 max( min.x + width, min.y + height );
	auto* draw_list = ImGui::GetBackgroundDrawList( );

	draw_list->AddRectFilled( ImVec2( min.x - 2.5f, min.y - 2.5f ), ImVec2( max.x + 2.5f, max.y + 2.5f ),
	                          ImColor( 4, 5, 7, static_cast< int >( 125.f * background_alpha ) ), 7.f );
	draw_list->AddRectFilled( ImVec2( min.x - 1.5f, min.y - 1.5f ), ImVec2( max.x + 1.5f, max.y + 1.5f ),
	                          ImColor( 70, 70, 72, static_cast< int >( 210.f * background_alpha ) ), 6.5f );
	draw_list->AddRectFilled( min, max, ImColor( 17, 17, 19, static_cast< int >( 245.f * background_alpha ) ), 6.f );
	draw_list->AddRect( min, max, ImColor( accent_cfg[ 0 ], accent_cfg[ 1 ], accent_cfg[ 2 ], static_cast< int >( 95.f * background_alpha ) ),
	                    6.f, ImDrawFlags_RoundCornersAll, 1.f );
	draw_list->AddRectFilled( ImVec2( min.x + 1.f, min.y + 1.f ), ImVec2( min.x + 3.f, max.y - 1.f ),
	                          ImColor( accent_cfg[ 0 ], accent_cfg[ 1 ], accent_cfg[ 2 ], static_cast< int >( 210.f * background_alpha ) ),
	                          5.f, ImDrawFlags_RoundCornersLeft );
	if ( mode == 3 ) {
		const float speed = std::clamp( GET_VARIABLE( g_variables.m_watermark_chroma_speed, float ), 0.15f, 4.f );
		const int segments = 18;
		for ( int i = 0; i < segments; ++i ) {
			const float t0 = static_cast< float >( i ) / static_cast< float >( segments );
			const float t1 = static_cast< float >( i + 1 ) / static_cast< float >( segments );
			const auto col_at = [ & ]( float t ) {
				if ( GET_VARIABLE( g_variables.m_theme_accent_gradient, bool ) ) {
					const auto start = GET_VARIABLE( g_variables.m_theme_gradient_start, c_color );
					const auto end = GET_VARIABLE( g_variables.m_theme_gradient_end, c_color );
					const float phase = 0.5f + 0.5f * std::sin( now * speed * 0.95f + t * 6.283f );
					return ImColor( static_cast< int >( std::lerp( static_cast< float >( start[ 0 ] ), static_cast< float >( end[ 0 ] ), phase ) ),
					                static_cast< int >( std::lerp( static_cast< float >( start[ 1 ] ), static_cast< float >( end[ 1 ] ), phase ) ),
					                static_cast< int >( std::lerp( static_cast< float >( start[ 2 ] ), static_cast< float >( end[ 2 ] ), phase ) ),
					                static_cast< int >( 190.f * background_alpha ) );
				}
				return ImColor( static_cast< int >( 127.f + std::sin( now * speed * 1.7f + t * 6.283f ) * 127.f ),
				                static_cast< int >( 127.f + std::sin( now * speed * 1.7f + t * 6.283f + 2.094f ) * 127.f ),
				                static_cast< int >( 127.f + std::sin( now * speed * 1.7f + t * 6.283f + 4.188f ) * 127.f ),
				                static_cast< int >( 190.f * background_alpha ) );
			};
			draw_list->AddRectFilledMultiColor( ImVec2( min.x + width * t0, min.y + 1.f ), ImVec2( min.x + width * t1 + 1.f, min.y + 3.f ),
			                                    col_at( t0 ), col_at( t1 ), col_at( t1 ), col_at( t0 ) );
		}
	}

	IDirect3DTexture9* bg_texture = mode == 2 ? clarity_texture : fake_drain_texture;
	if ( extra_mode && extra_index >= 0 && extra_index < static_cast< int >( extra_watermark_textures.size( ) ) )
		bg_texture = extra_watermark_textures[ extra_index ];
	if ( bg_texture ) {
		draw_list->PushClipRect( min, ImVec2( min.x + ( mode == 2 ? 70.f : 92.f ), max.y ), true );
		if ( mode == 2 ) {
			draw_list->AddImage( reinterpret_cast< ImTextureID >( bg_texture ), ImVec2( min.x - 18.f, min.y - 20.f ),
			                     ImVec2( min.x + 44.f, min.y + 42.f ), ImVec2( 0.f, 0.f ), ImVec2( 1.f, 1.f ),
			                     ImColor( accent_cfg[ 0 ], accent_cfg[ 1 ], accent_cfg[ 2 ], static_cast< int >( std::max( alpha - 196.f, 0.f ) ) ) );
		} else if ( extra_mode ) {
			draw_list->AddImage( reinterpret_cast< ImTextureID >( bg_texture ), ImVec2( min.x - 10.f, min.y - 18.f ),
			                     ImVec2( min.x + 78.f, min.y + 48.f ), ImVec2( 0.f, 0.f ), ImVec2( 1.f, 1.f ),
			                     ImColor( 255, 255, 255, static_cast< int >( std::max( alpha - 176.f, 0.f ) ) ) );
		} else {
			draw_list->AddImage( reinterpret_cast< ImTextureID >( bg_texture ), ImVec2( min.x - 22.f, min.y - 5.f ),
			                     ImVec2( min.x + 102.f, min.y + 30.f ), ImVec2( 0.f, 0.f ), ImVec2( 1.f, 1.f ),
			                     ImColor( accent_cfg[ 0 ], accent_cfg[ 1 ], accent_cfg[ 2 ], static_cast< int >( std::max( alpha - 188.f, 0.f ) ) ) );
		}
		draw_list->PopClipRect( );
	}

	const int text_alpha = static_cast< int >( alpha );
	const float text_y = min.y + ( height - font_size ) * 0.5f - 0.5f;
	float cursor_x = min.x + 8.f;
	auto draw_segment = [ & ]( const std::string& text, const ImColor& color, float advance_extra = 0.f ) {
		draw_list->AddText( font, font_size, ImVec2( cursor_x, text_y ), color, text.c_str( ) );
		cursor_x += font->CalcTextSizeA( font_size, 4096.f, 0.f, text.c_str( ) ).x + advance_extra;
	};

	draw_segment( label, ImColor( accent_cfg[ 0 ], accent_cfg[ 1 ], accent_cfg[ 2 ], text_alpha ), 10.f );
	draw_segment( sep, ImColor( 60, 60, 60, text_alpha ), 9.f );
	draw_segment( nick_label, ImColor( 138, 138, 138, text_alpha ), 9.f );
	draw_segment( sep, ImColor( 60, 60, 60, text_alpha ), 9.f );
	draw_segment( fps_number, ImColor( 229, 229, 234, text_alpha ), 5.f );
	draw_segment( fps_label, ImColor( 138, 138, 138, text_alpha ) );
}

void n_misc::impl_t::draw_visual_cosmetics( )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !g_interfaces.m_global_vars_base )
		return;

	struct movement_sample_t {
		c_vector origin{ };
		float speed = 0.f;
		float time = 0.f;
		bool grounded = false;
	};

	struct roast_marker_t {
		c_vector origin{ };
		float time = 0.f;
		const char* text = "";
	};

	struct px_path_t {
		std::string map{ };
		c_vector start{ };
		c_vector end{ };
		c_vector normal{ };
		c_vector direction{ };
	};

	struct velocity_graph_sample_t {
		float speed = 0.f;
		float vertical = 0.f;
		float accel = 0.f;
		float yaw_delta = 0.f;
		float wish_dir = 0.f;
		bool grounded = false;
		bool jump_event = false;
		bool land_event = false;
		bool tech_event = false;
		const char* marker = "";
		const char* direction = "unknown";
		float time = 0.f;
	};

	static std::deque< movement_sample_t > movement_samples;
	static std::deque< float > graph_samples;
	static std::deque< velocity_graph_sample_t > velocity_graph_samples;
	static std::vector< roast_marker_t > roast_markers;
	static std::vector< px_path_t > px_paths;
	static std::vector< c_vector > px_active_points;
	static bool px_database_loaded = false;
	static bool px_was_pixelsurfing = false;
	static float last_px_path_hit_print = 0.f;
	static bool run_active = false;
	static float run_max_speed = 0.f;
	static float run_start_time = 0.f;
	static float run_last_event_time = 0.f;
	static bool run_failed = false;
	static std::string run_fail_reason;
	static std::vector< std::string > run_combo;
	static std::vector< std::string > run_binds;
	static bool run_jumpbug_attempt = false;
	static float old_edit_until = 0.f;
	static float last_sample_time = 0.f;
	static float last_graph_sample_time = 0.f;
	static float last_velocity_graph_sample_time = 0.f;
	static float last_tracer_time = 0.f;
	static float last_footstep_time = 0.f;
	static c_vector last_footstep_origin{ };
	static int last_footstep_mode = 0;
	static float pulse_alpha = 0.f;
	static float pts_heat = 0.f;
	static float pts_flash = 0.f;
	static int pts_value = 0;
	static int aura_debt = 0;
	static float jump_feedback_alpha = 0.f;
	static float jump_feedback_delta = 0.f;
	static float peak_speed = 0.f;
	static float last_roast_time = 0.f;
	static float last_feedback_sound_time = 0.f;
	static float air_start_time = 0.f;
	static c_vector air_start_origin{ };
	static float air_peak_speed = 0.f;
	static float last_edgebug_score_time = 0.f;
	static float last_pixelsurf_score_time = 0.f;
	static float last_edgebug_particle_time = 0.f;
	static float last_chat_times[ 4 ]{ };
	static bool last_jumpbug_state = false;
	static bool last_grounded = true;
	static bool last_edgebug_state = false;
	static bool last_pixelsurf_state = false;
	static bool last_jump_pressed = false;
	static bool last_duck_pressed = false;
	static bool last_lj_active = false;
	static bool last_mj_active = false;
	static bool last_eb_key_active = false;
	static bool last_px_key_active = false;
	static bool last_jb_key_active = false;
	static bool last_speed_over_threshold = false;
	static float last_speed = 0.f;
	static float last_speed_for_graph = 0.f;
	static const char* last_bhop_direction = "unknown";
	static float last_direction_change_time = 0.f;
	static float last_px_sound_time = 0.f;
	static bool px_sound_active = false;
	static float last_jump_stats_sound_time = 0.f;
	static std::string last_jump_stats_line = "none";
	static float last_pts_tick = 0.f;
	static IDirect3DTexture9* fumo_texture = nullptr;
	static bool tried_fumo_texture = false;

	const float now = g_interfaces.m_global_vars_base->m_real_time;
	const float frame_time = std::clamp( g_interfaces.m_global_vars_base->m_frame_time, 0.f, 0.05f );
	const float speed = g_ctx.m_local->get_velocity( ).length_2d( );
	const bool grounded = ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) != 0;
	const auto accent_cfg = GET_VARIABLE( g_variables.m_accent, c_color );
	const auto pts_color_cfg = GET_VARIABLE( g_variables.m_pts_meter_color, c_color );
	const ImColor accent( accent_cfg[ 0 ], accent_cfg[ 1 ], accent_cfg[ 2 ], accent_cfg[ 3 ] );
	auto draw = ImGui::GetBackgroundDrawList( );
	const float width = static_cast< float >( g_ctx.m_width );
	const float height = static_cast< float >( g_ctx.m_height );
	const std::string map_name = current_map_name( );

	const std::string px_database_path = std::string( n_branding::k_config_directory ) + "px_database.dat";
	auto load_px_database = [ & ]( ) {
		if ( px_database_loaded )
			return;

		px_database_loaded = true;
		px_paths.clear( );

		std::ifstream file( px_database_path );
		if ( !file.good( ) )
			return;

		px_path_t path{ };
		while ( file >> path.map >> path.start.m_x >> path.start.m_y >> path.start.m_z >> path.end.m_x >> path.end.m_y >> path.end.m_z >>
		        path.normal.m_x >> path.normal.m_y >> path.normal.m_z >> path.direction.m_x >> path.direction.m_y >> path.direction.m_z ) {
			if ( path.start.is_valid( ) && path.end.is_valid( ) && path.start.dist_to_2d( path.end ) > 8.f )
				px_paths.push_back( path );
			path = px_path_t{ };
		}
	};
	auto save_px_database = [ & ]( ) {
		std::filesystem::create_directories( n_branding::k_config_directory );
		std::ofstream file( px_database_path, std::ios::trunc );
		for ( const auto& path : px_paths ) {
			file << path.map << ' ' << path.start.m_x << ' ' << path.start.m_y << ' ' << path.start.m_z << ' ' << path.end.m_x << ' '
			     << path.end.m_y << ' ' << path.end.m_z << ' ' << path.normal.m_x << ' ' << path.normal.m_y << ' ' << path.normal.m_z << ' '
			     << path.direction.m_x << ' ' << path.direction.m_y << ' ' << path.direction.m_z << '\n';
		}
	};
	auto distance_to_segment = []( const c_vector& point, const c_vector& start, const c_vector& end ) {
		const c_vector ab = end - start;
		const float denom = std::max( ab.length_squared( ), 1.f );
		const float t = std::clamp( ( point - start ).dot_product( ab ) / denom, 0.f, 1.f );
		return point.dist_to( start + ab * t );
	};
	auto average_points = []( const std::vector< c_vector >& points, std::size_t first, std::size_t last ) {
		c_vector out{ };
		if ( points.empty( ) )
			return out;

		last = std::min( last, points.size( ) - 1U );
		float count = 0.f;
		for ( std::size_t i = first; i <= last; ++i ) {
			out += points[ i ];
			count += 1.f;
		}
		return count > 0.f ? out / count : out;
	};
	auto scan_wall_normal = [ & ]( const c_vector& origin, c_vector& out_normal ) {
		if ( !g_interfaces.m_engine_trace )
			return false;

		c_trace_filter_world filter{ };
		float best_distance = std::numeric_limits< float >::max( );
		bool found = false;
		for ( int i = 0; i < 32; ++i ) {
			const float angle = ( static_cast< float >( i ) / 32.f ) * 6.28318530718f;
			const c_vector direction( std::cos( angle ), std::sin( angle ), 0.f );
			trace_t trace{ };
			g_interfaces.m_engine_trace->trace_ray( ray_t( origin, origin + direction * 96.f ), mask_playersolid, &filter, &trace );
			if ( trace.m_fraction >= 1.f || std::fabs( trace.m_plane.m_normal.m_z ) > 0.16f )
				continue;

			const float distance = origin.dist_to( trace.m_end );
			if ( distance < best_distance ) {
				best_distance = distance;
				out_normal = trace.m_plane.m_normal;
				found = true;
			}
		}

		if ( found )
			out_normal.normalize_in_place( );
		return found;
	};
	auto build_clean_px_path = [ & ]( const std::vector< c_vector >& raw_points, px_path_t& out_path ) {
		if ( map_name.empty( ) || raw_points.size( ) < 4U )
			return false;

		std::size_t first = 0U;
		std::size_t last = raw_points.size( ) - 1U;
		while ( first + 1U < last && raw_points[ first + 1U ].m_z - raw_points[ first ].m_z < -11.f )
			++first;
		while ( last > first + 1U ) {
			const float dz = raw_points[ last ].m_z - raw_points[ last - 1U ].m_z;
			if ( dz < -9.f || dz > 8.f ) {
				--last;
				continue;
			}
			break;
		}
		if ( last <= first + 1U )
			return false;

		const std::size_t sample_count = last - first + 1U;
		const std::size_t window = std::min< std::size_t >( 3U, sample_count );
		const c_vector start_anchor = average_points( raw_points, first, first + window - 1U );
		const c_vector end_anchor = average_points( raw_points, last - window + 1U, last );
		c_vector direction = end_anchor - start_anchor;
		direction.m_z = 0.f;
		const float line_length = direction.length_2d( );
		if ( line_length < 32.f )
			return false;
		direction /= line_length;

		float flat_z = 0.f;
		for ( std::size_t i = first; i <= last; ++i )
			flat_z += raw_points[ i ].m_z;
		flat_z /= static_cast< float >( sample_count );

		out_path.map = map_name;
		out_path.direction = direction;
		out_path.start = c_vector( start_anchor.m_x, start_anchor.m_y, flat_z );
		out_path.end = c_vector( start_anchor.m_x + direction.m_x * line_length, start_anchor.m_y + direction.m_y * line_length, flat_z );
		out_path.normal = c_vector{ };
		scan_wall_normal( ( out_path.start + out_path.end ) * 0.5f, out_path.normal );
		return out_path.start.dist_to_2d( out_path.end ) > 32.f;
	};
	auto insert_or_merge_px_path = [ & ]( px_path_t path ) {
		path.direction.m_z = 0.f;
		if ( path.direction.normalize_in_place( ) < 0.001f )
			return false;

		bool changed = false;
		for ( auto& existing : px_paths ) {
			if ( existing.map != path.map )
				continue;

			c_vector existing_dir = existing.direction;
			existing_dir.m_z = 0.f;
			if ( existing_dir.normalize_in_place( ) < 0.001f )
				continue;
			if ( std::fabs( existing_dir.dot_product( path.direction ) ) < 0.96f )
				continue;
			if ( existing.normal.length_squared( ) > 0.01f && path.normal.length_squared( ) > 0.01f &&
			     existing.normal.dot_product( path.normal ) < 0.72f )
				continue;

			const c_vector reference = existing.start;
			const c_vector perpendicular( -existing_dir.m_y, existing_dir.m_x, 0.f );
			const float new_offset = std::fabs( ( path.start - reference ).dot_product( perpendicular ) );
			const float z_offset = std::fabs( path.start.m_z - existing.start.m_z );
			if ( new_offset > 28.f || z_offset > 14.f )
				continue;

			float min_t = 0.f;
			float max_t = ( existing.end - reference ).dot_product( existing_dir );
			const float t0 = ( path.start - reference ).dot_product( existing_dir );
			const float t1 = ( path.end - reference ).dot_product( existing_dir );
			min_t = std::min( min_t, std::min( t0, t1 ) );
			max_t = std::max( max_t, std::max( t0, t1 ) );
			existing.start = reference + existing_dir * min_t;
			existing.end = reference + existing_dir * max_t;
			existing.start.m_z = existing.end.m_z = ( existing.start.m_z + existing.end.m_z + path.start.m_z + path.end.m_z ) * 0.25f;
			if ( existing.normal.length_squared( ) <= 0.01f )
				existing.normal = path.normal;
			changed = true;
			return true;
		}

		for ( const auto& existing : px_paths ) {
			if ( existing.map == path.map && distance_to_segment( ( path.start + path.end ) * 0.5f, existing.start, existing.end ) < 24.f )
				return false;
		}

		px_paths.push_back( path );
		changed = true;
		return changed;
	};
	auto delete_nearest_px_path = [ & ]( ) {
		if ( px_paths.empty( ) )
			return false;

		const c_vector origin = g_ctx.m_local->get_abs_origin( );
		float best_distance = 96.f;
		auto best = px_paths.end( );
		for ( auto it = px_paths.begin( ); it != px_paths.end( ); ++it ) {
			if ( GET_VARIABLE( g_variables.m_px_database_current_map_only, bool ) && it->map != map_name )
				continue;
			const float distance = distance_to_segment( origin, it->start, it->end );
			if ( distance < best_distance ) {
				best_distance = distance;
				best = it;
			}
		}
		if ( best == px_paths.end( ) )
			return false;

		px_paths.erase( best );
		save_px_database( );
		return true;
	};
	load_px_database( );

	if ( now - last_sample_time > 0.035f ) {
		if ( speed > 20.f )
			movement_samples.push_front( { g_ctx.m_local->get_abs_origin( ), speed, now, grounded } );
		last_sample_time = now;
	}

	while ( movement_samples.size( ) > 56U || ( !movement_samples.empty( ) && now - movement_samples.back( ).time > 1.35f ) )
		movement_samples.pop_back( );

	if ( now - last_graph_sample_time > 0.05f ) {
		graph_samples.push_front( speed );
		while ( graph_samples.size( ) > 86U )
			graph_samples.pop_back( );
		last_graph_sample_time = now;
	}

	const float speed_threshold = std::clamp( GET_VARIABLE( g_variables.m_speed_pulse_threshold, float ), 120.f, 520.f );
	const bool speed_over_threshold = speed >= speed_threshold;
	if ( speed_over_threshold && !last_speed_over_threshold )
		pulse_alpha = 1.f;
	last_speed_over_threshold = speed_over_threshold;
	pulse_alpha = std::max( 0.f, pulse_alpha - frame_time * 1.9f );

	const float speed_delta = speed - last_speed;
	const bool just_left_ground = last_grounded && !grounded;
	const bool just_landed = !last_grounded && grounded;
	const bool allow_movement_side_effects = !g_in_movement_assist_simulation;
	const float view_yaw = g_ctx.m_cmd ? g_ctx.m_cmd->m_view_point.m_y : g_ctx.m_last_tick_yaw;
	const c_vector velocity_now = g_ctx.m_local->get_velocity( );
	float velocity_yaw = std::atan2( velocity_now.m_y, velocity_now.m_x ) * 57.295779513f;
	if ( speed < 4.f )
		velocity_yaw = view_yaw;
	float yaw_delta = std::remainder( velocity_yaw - view_yaw, 360.f );
	const char* direction_label = bhop_direction_label( yaw_delta );
	if ( std::strcmp( direction_label, last_bhop_direction ) != 0 ) {
		last_bhop_direction = direction_label;
		last_direction_change_time = now;
	}
	const bool direction_transition = now - last_direction_change_time < 0.22f;
	const float wish_dir = g_ctx.m_cmd ? std::atan2( -g_ctx.m_cmd->m_side_move, g_ctx.m_cmd->m_forward_move ) * 57.295779513f : 0.f;
	const bool graph_jump_pressed = g_ctx.m_cmd && ( g_ctx.m_cmd->m_buttons & in_jump );
	const bool graph_duck_pressed = g_ctx.m_cmd && ( g_ctx.m_cmd->m_buttons & in_duck );
	const bool graph_lj_active = GET_VARIABLE( g_variables.m_long_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) );
	const bool graph_mj_active = GET_VARIABLE( g_variables.m_mini_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) );
	const bool graph_pixelsurf_confirmed = g_movement.m_pixelsurf_data.m_in_pixel_surf;
	const bool graph_edgebug_confirmed = g_movement.m_edgebug_data.m_will_edgebug && !g_movement.m_edgebug_data.m_will_fail;
	const bool graph_jumpbug_confirmed = g_movement.m_jumpbug_data.m_can_jb;
	const char* graph_marker = "";
	if ( graph_jumpbug_confirmed && !last_jumpbug_state )
		graph_marker = "JB";
	else if ( graph_edgebug_confirmed && !last_edgebug_state )
		graph_marker = "EB";
	else if ( graph_pixelsurf_confirmed && !last_pixelsurf_state )
		graph_marker = "PX";
	else if ( just_left_ground && graph_jump_pressed ) {
		if ( graph_lj_active )
			graph_marker = "LJ";
		else if ( graph_mj_active )
			graph_marker = "MJ";
		else if ( graph_duck_pressed )
			graph_marker = "CJ";
		else
			graph_marker = "J";
	} else if ( just_landed ) {
		graph_marker = "L";
	}

	if ( now - last_velocity_graph_sample_time > 0.05f ) {
		velocity_graph_sample_t sample{ };
		sample.speed = speed;
		sample.vertical = velocity_now.m_z;
		sample.accel = speed - last_speed_for_graph;
		sample.yaw_delta = yaw_delta;
		sample.wish_dir = wish_dir;
		sample.grounded = grounded;
		sample.jump_event = just_left_ground;
		sample.land_event = just_landed;
		sample.tech_event = g_movement.m_pixelsurf_data.m_in_pixel_surf || g_movement.m_jumpbug_data.m_can_jb ||
		                    ( g_movement.m_edgebug_data.m_will_edgebug && !g_movement.m_edgebug_data.m_will_fail );
		sample.marker = graph_marker;
		sample.direction = direction_transition ? "transition" : direction_label;
		sample.time = now;
		velocity_graph_samples.push_front( sample );
		last_speed_for_graph = speed;
		last_velocity_graph_sample_time = now;
	}
	const int max_graph_samples = std::clamp( GET_VARIABLE( g_variables.m_velocity_graph_samples, int ), 32, 220 );
	while ( velocity_graph_samples.size( ) > static_cast< std::size_t >( max_graph_samples ) )
		velocity_graph_samples.pop_back( );
	if ( just_left_ground ) {
		air_start_origin = g_ctx.m_local->get_abs_origin( );
		air_start_time = now;
		air_peak_speed = speed;
	}
	if ( !grounded )
		air_peak_speed = std::max( air_peak_speed, speed );

	bool pts_event = false;
	auto add_score_event = [ & ]( int pts_delta, int aura_delta, float heat_delta ) {
		if ( !allow_movement_side_effects )
			return;

		if ( GET_VARIABLE( g_variables.m_pts_meter, bool ) ) {
			pts_value = std::max( 0, pts_value + pts_delta );
			pts_event = true;
		}
		if ( GET_VARIABLE( g_variables.m_aura_debt_counter, bool ) )
			aura_debt = std::max( 0, aura_debt + aura_delta );

		pts_heat = std::min( 1.f, pts_heat + heat_delta );
		pts_flash = 1.f;
		last_pts_tick = now;
	};

	auto emit_movement_print = [ & ]( const int index, const bool enabled, const char* text ) {
		if ( !allow_movement_side_effects )
			return;

		const float cooldown = std::clamp( GET_VARIABLE( g_variables.m_chat_print_cooldown, float ), 0.15f, 4.f );
		if ( enabled && index >= 0 && index < 4 && now - last_chat_times[ index ] > cooldown ) {
			movement_print( text );
			last_chat_times[ index ] = now;
		}
	};

	const bool jump_pressed = g_ctx.m_cmd && ( g_ctx.m_cmd->m_buttons & in_jump );
	const bool duck_pressed = g_ctx.m_cmd && ( g_ctx.m_cmd->m_buttons & in_duck );
	const bool lj_active = GET_VARIABLE( g_variables.m_long_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) );
	const bool mj_active = GET_VARIABLE( g_variables.m_mini_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) );
	const bool eb_key_active = GET_VARIABLE( g_variables.edge_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) );
	const bool px_key_active = GET_VARIABLE( g_variables.m_pixel_surf, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) );
	const bool jb_key_active = GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) );

	auto start_run = [ & ]( const char* reason ) {
		if ( !allow_movement_side_effects || !GET_VARIABLE( g_variables.m_run_analyzer, bool ) || run_active )
			return;

		run_active = true;
		m_session_hub_snapshot.run_status = "active";
		run_max_speed = speed;
		run_start_time = now;
		run_last_event_time = now;
		run_failed = false;
		run_fail_reason.clear( );
		run_combo.clear( );
		run_binds.clear( );
		run_jumpbug_attempt = false;
		debug_log( "run", std::string( "started reason=" ) + reason + " velocity=" + std::to_string( static_cast< int >( speed ) ),
		           GET_VARIABLE( g_variables.m_debug_log_run, bool ), 0.15f, "run_start" );
	};
	auto add_run_event = [ & ]( const char* token, const bool allow_consecutive_duplicate = true ) {
		if ( !allow_movement_side_effects || !GET_VARIABLE( g_variables.m_run_analyzer, bool ) )
			return;

		start_run( token );
		if ( run_combo.empty( ) || allow_consecutive_duplicate || run_combo.back( ) != token )
			run_combo.emplace_back( token );
		m_session_hub_snapshot.last_tech = token;
		run_last_event_time = now;
		debug_log( "run", std::string( "event=" ) + token + " velocity=" + std::to_string( static_cast< int >( speed ) ) +
		                       " grounded=" + ( grounded ? "1" : "0" ),
		           GET_VARIABLE( g_variables.m_debug_log_run, bool ), 0.05f, token );
	};
	auto add_run_bind = [ & ]( const char* token ) {
		if ( !allow_movement_side_effects || !GET_VARIABLE( g_variables.m_run_analyzer, bool ) )
			return;

		start_run( token );
		if ( std::find( run_binds.begin( ), run_binds.end( ), token ) == run_binds.end( ) )
			run_binds.emplace_back( token );
		debug_log( "run", std::string( "bind=" ) + token + " buttons=" + ( g_ctx.m_cmd ? std::to_string( g_ctx.m_cmd->m_buttons ) : "0" ),
		           GET_VARIABLE( g_variables.m_debug_log_run, bool ), 0.05f, token );
	};
	auto join_strings = []( const std::vector< std::string >& values, const char* separator, const char* fallback ) {
		if ( values.empty( ) )
			return std::string( fallback );

		std::string out;
		for ( std::size_t i = 0U; i < values.size( ); ++i ) {
			if ( i > 0U )
				out += separator;
			out += values[ i ];
		}
		return out;
	};
	auto finish_run = [ & ]( bool failed, const char* reason ) {
		if ( !allow_movement_side_effects || !run_active )
			return;

		failed = failed || run_failed;
		if ( run_failed && !run_fail_reason.empty( ) )
			reason = run_fail_reason.c_str( );

		if ( !failed || GET_VARIABLE( g_variables.m_run_analyzer_show_failed, bool ) ) {
			const std::string combo_text = join_strings( run_combo, " + ", failed ? "failed" : "none" );
			const std::string bind_text = join_strings( run_binds, ", ", "none" );
			std::string run_text = std::string( failed ? "fail" : "success" ) + " | max speed: " +
			                       std::to_string( static_cast< int >( run_max_speed ) ) + " | combo: " + combo_text;
			if ( failed && reason && reason[ 0 ] != '\0' )
				run_text += std::string( " | reason: " ) + reason;
			if ( GET_VARIABLE( g_variables.m_run_analyzer_include_raw_binds, bool ) )
				run_text += " | binds: " + bind_text;
			m_session_hub_snapshot.run_status = failed ? "fail" : "success";
			m_session_hub_snapshot.run_max_speed = static_cast< int >( run_max_speed );
			m_session_hub_snapshot.run_combo = combo_text;
			m_session_hub_snapshot.last_fail_reason = failed && reason && reason[ 0 ] ? reason : "none";
			m_session_hub_snapshot.last_run_summary = run_text;
			m_session_hub_snapshot.coach_feedback = failed && reason && std::string( reason ).find( "JB" ) != std::string::npos ?
			                                            "JB failed; no timing data" :
			                                            combo_text.find( "PX" ) != std::string::npos ? "PX used; no timing data" : "no timing data";
			if ( GET_VARIABLE( g_variables.m_session_movement_coach, bool ) ) {
				const int coach_output = std::clamp( GET_VARIABLE( g_variables.m_session_movement_coach_output, int ), 0, 2 );
				if ( coach_output == 1 )
					movement_print( m_session_hub_snapshot.coach_feedback.c_str( ), false );
				else if ( coach_output == 2 )
					debug_log( "coach", m_session_hub_snapshot.coach_feedback, GET_VARIABLE( g_variables.m_debug_log_run, bool ), 0.25f, "coach" );
			}
			movement_print( run_text.c_str( ), false );

			debug_log( "run", std::string( "ended reason=" ) + ( reason ? reason : "timeout" ) + " max_vel=" +
			                       std::to_string( static_cast< int >( run_max_speed ) ) + " combo=" + combo_text + " binds=" + bind_text,
			           GET_VARIABLE( g_variables.m_debug_log_run, bool ), 0.1f, "run_end" );

			if ( GET_VARIABLE( g_variables.m_old_edit_vibes, bool ) &&
			     static_cast< int >( run_combo.size( ) ) >= std::clamp( GET_VARIABLE( g_variables.m_old_edit_vibes_min_combo, int ), 1, 6 ) ) {
				const float chance = std::clamp( GET_VARIABLE( g_variables.m_old_edit_vibes_chance, float ), 0.f, 1.f );
				const float roll = std::fmod( std::abs( std::sin( now * 12.9898f ) * 43758.5453f ), 1.f );
				if ( roll < chance ) {
					old_edit_until = now + 1.55f;
					if ( GET_VARIABLE( g_variables.m_old_edit_vibes_sound, bool ) ) {
						const auto sound_path = GET_VARIABLE( g_variables.m_old_edit_vibes_sound_path, std::string );
						if ( !sound_path.empty( ) )
							safe_play_ui_sound( sound_path, "old_edit_vibes", 0.85f );
					}
				}
			}
		}

		run_active = false;
		run_failed = false;
		run_fail_reason.clear( );
		run_combo.clear( );
		run_binds.clear( );
		run_jumpbug_attempt = false;
	};

	if ( GET_VARIABLE( g_variables.m_run_analyzer, bool ) ) {
		const float min_speed = std::clamp( GET_VARIABLE( g_variables.m_run_analyzer_min_speed, float ), 120.f, 520.f );
		if ( !run_active && speed >= min_speed )
			start_run( "speed" );
		if ( run_active )
			run_max_speed = std::max( run_max_speed, speed );
	}

	if ( jump_pressed && !last_jump_pressed )
		add_run_bind( "jump" );
	if ( duck_pressed && !last_duck_pressed )
		add_run_bind( "duck" );
	if ( lj_active && !last_lj_active )
		add_run_bind( "LJ" );
	if ( mj_active && !last_mj_active )
		add_run_bind( "minijump" );
	if ( eb_key_active && !last_eb_key_active )
		add_run_bind( "EB bind" );
	if ( px_key_active && !last_px_key_active )
		add_run_bind( "PX bind" );
	if ( jb_key_active && !last_jb_key_active )
		add_run_bind( "JB bind" );

	if ( jb_key_active && !grounded && g_ctx.m_local->get_velocity( ).m_z < -6.25f )
		run_jumpbug_attempt = true;
	if ( just_left_ground && jump_pressed ) {
		if ( lj_active )
			add_run_event( "LJ", false );
		else if ( mj_active )
			add_run_event( "MJ", false );
		else if ( duck_pressed )
			add_run_event( "CJ", false );
		else
			add_run_event( "J" );
	}

	const bool pixelsurf_state = g_movement.m_pixelsurf_data.m_in_pixel_surf || g_movement.m_pixelsurf_data.m_predicted_succesful;
	const bool pixelsurf_confirmed = g_movement.m_pixelsurf_data.m_in_pixel_surf;
	const bool edgebug_state = g_movement.m_edgebug_data.m_will_edgebug && !g_movement.m_edgebug_data.m_will_fail;
	if ( pixelsurf_state && !last_pixelsurf_state && now - last_pixelsurf_score_time > 0.45f ) {
		add_score_event( 125, 8, 0.44f );
		add_run_event( "PX" );
		emit_movement_print( 1, GET_VARIABLE( g_variables.m_chat_print_pixelsurf, bool ), "pixelsurfed" );
		last_pixelsurf_score_time = now;
	}
	if ( edgebug_state && !last_edgebug_state && now - last_edgebug_score_time > 0.45f ) {
		add_score_event( 95, 6, 0.38f );
		add_run_event( "EB" );
		emit_movement_print( 0, GET_VARIABLE( g_variables.m_chat_print_edgebug, bool ), "edgebugged" );
		last_edgebug_score_time = now;
	}
	if ( allow_movement_side_effects && edgebug_state && !last_edgebug_state && GET_VARIABLE( g_variables.m_edgebug_particles, bool ) &&
	     now - last_edgebug_particle_time > std::clamp( GET_VARIABLE( g_variables.m_edgebug_particle_cooldown, float ), 0.18f, 2.0f ) ) {
		emit_selected_particle( "particles][edgebug", g_ctx.m_local->get_abs_origin( ),
		                        std::clamp( GET_VARIABLE( g_variables.m_edgebug_particle_type, int ), 1, static_cast< int >( k_particle_options.size( ) ) - 1 ),
		                        GET_VARIABLE( g_variables.m_edgebug_particle_intensity, float ), nullptr, accent_cfg, 0.2f );
		last_edgebug_particle_time = now;
	} else if ( allow_movement_side_effects && edgebug_state && !last_edgebug_state && GET_VARIABLE( g_variables.m_edgebug_particles, bool ) ) {
		const float cooldown = std::clamp( GET_VARIABLE( g_variables.m_edgebug_particle_cooldown, float ), 0.18f, 2.0f );
		std::ostringstream message;
		message << "skipped=cooldown remaining=" << std::fixed << std::setprecision( 2 )
		        << std::max( 0.f, cooldown - ( now - last_edgebug_particle_time ) );
		debug_log( "particles][edgebug", message.str( ), GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.1f, "eb_cooldown" );
	}
	const bool jumpbug_state = g_movement.m_jumpbug_data.m_can_jb;
	if ( jumpbug_state && !last_jumpbug_state ) {
		add_run_event( "JB" );
		run_jumpbug_attempt = false;
		emit_movement_print( 3, GET_VARIABLE( g_variables.m_chat_print_jumpbug, bool ), "jumpbugged" );
	}
	if ( just_landed && run_jumpbug_attempt && !jumpbug_state ) {
		run_failed = true;
		run_fail_reason = "JB failed";
		add_run_event( "JB", false );
		finish_run( true, "JB failed" );
	}
	last_jumpbug_state = jumpbug_state;

	if ( allow_movement_side_effects && GET_VARIABLE( g_variables.m_px_database, bool ) ) {
		if ( GET_VARIABLE( g_variables.m_px_database_auto_record, bool ) ) {
			const c_vector origin = g_ctx.m_local->get_abs_origin( );
			if ( pixelsurf_state ) {
				if ( !px_was_pixelsurfing ) {
					px_active_points.clear( );
					px_active_points.push_back( origin );
				} else if ( px_active_points.empty( ) || px_active_points.back( ).dist_to_2d( origin ) >= 8.f ) {
					px_active_points.push_back( origin );
				}
			} else if ( px_was_pixelsurfing ) {
				px_path_t finished{ };
				if ( build_clean_px_path( px_active_points, finished ) && insert_or_merge_px_path( finished ) ) {
					save_px_database( );
					emit_movement_print( 2, GET_VARIABLE( g_variables.m_chat_print_new_px, bool ), "new ps found" );
				}
				px_active_points.clear( );
			}
			px_was_pixelsurfing = pixelsurf_state;
		} else {
			px_active_points.clear( );
			px_was_pixelsurfing = false;
		}

		if ( g_px_delete_nearest_requested ) {
			delete_nearest_px_path( );
			g_px_delete_nearest_requested = false;
		}
		if ( g_px_clear_current_map_requested ) {
			px_paths.erase( std::remove_if( px_paths.begin( ), px_paths.end( ),
			                                [ & ]( const px_path_t& path ) { return map_name.empty( ) || path.map == map_name; } ),
			                px_paths.end( ) );
			save_px_database( );
			g_px_clear_current_map_requested = false;
		}

		if ( pixelsurf_state && now - last_px_path_hit_print > std::clamp( GET_VARIABLE( g_variables.m_chat_print_cooldown, float ), 0.15f, 4.f ) ) {
			for ( const auto& path : px_paths ) {
				if ( GET_VARIABLE( g_variables.m_px_database_current_map_only, bool ) && path.map != map_name )
					continue;
				if ( distance_to_segment( g_ctx.m_local->get_abs_origin( ), path.start, path.end ) < 42.f ) {
					emit_movement_print( 1, GET_VARIABLE( g_variables.m_chat_print_pixelsurf, bool ), "pixelsurfed" );
					last_px_path_hit_print = now;
					break;
				}
			}
		}
	}
	last_pixelsurf_state = pixelsurf_state;
	last_edgebug_state = edgebug_state;

	if ( GET_VARIABLE( g_variables.m_px_sound_enable, bool ) ) {
		const int px_sound_type = std::clamp( GET_VARIABLE( g_variables.m_px_sound_type, int ), 0, 2 );
		const float retrigger = std::clamp( GET_VARIABLE( g_variables.m_px_sound_retrigger, float ), 0.05f, 1.0f );
		const float min_v = std::clamp( GET_VARIABLE( g_variables.m_px_sound_vel_min, float ), 50.f, 600.f );
		const float max_v = std::max( min_v + 1.f, std::clamp( GET_VARIABLE( g_variables.m_px_sound_vel_max, float ), 100.f, 800.f ) );
		const float normalized = std::clamp( ( speed - min_v ) / ( max_v - min_v ), 0.f, 1.f );
		const float pitch = std::lerp( std::clamp( GET_VARIABLE( g_variables.m_px_sound_pitch_min, float ), 0.5f, 2.0f ),
		                               std::clamp( GET_VARIABLE( g_variables.m_px_sound_pitch_max, float ), 0.5f, 2.0f ), normalized );
		const std::string sound_path = px_sound_type == 0 ? "weapons/revolver/revolver_prepare.wav" :
		                               px_sound_type == 1 ? GET_VARIABLE( g_variables.m_px_sound_custom_path, std::string ) : "";
		if ( g_in_movement_assist_simulation ) {
			if ( GET_VARIABLE( g_variables.m_px_sound_debug, bool ) )
				debug_log( "sound][px", "skipped reason=in_assist_simulation", true, 0.2f, "px_sim" );
		} else if ( pixelsurf_confirmed && px_sound_type != 2 && !sound_path.empty( ) ) {
			if ( !px_sound_active && now - last_px_sound_time > retrigger ) {
				safe_play_ui_sound( sound_path, "pixelsurf", retrigger );
				if ( GET_VARIABLE( g_variables.m_px_sound_debug, bool ) )
					debug_log( "sound][px", "start velocity=" + std::to_string( static_cast< int >( speed ) ) + " pitch=" + std::to_string( pitch ),
					           true, 0.08f, "px_start" );
				px_sound_active = true;
				last_px_sound_time = now;
			} else if ( px_sound_active && now - last_px_sound_time > retrigger ) {
				safe_play_ui_sound( sound_path, "pixelsurf", retrigger );
				if ( GET_VARIABLE( g_variables.m_px_sound_debug, bool ) )
					debug_log( "sound][px", "retrigger velocity=" + std::to_string( static_cast< int >( speed ) ) + " pitch=" + std::to_string( pitch ),
					           true, 0.08f, "px_retrigger" );
				last_px_sound_time = now;
			}
		} else if ( px_sound_active ) {
			px_sound_active = false;
			if ( GET_VARIABLE( g_variables.m_px_sound_debug, bool ) )
				debug_log( "sound][px", "fade_out reason=px_end", true, 0.1f, "px_end" );
		}
	} else {
		px_sound_active = false;
	}

	if ( just_landed && !air_start_origin.is_zero( ) ) {
		const float airtime = now - air_start_time;
		const float distance = g_ctx.m_local->get_abs_origin( ).dist_to_2d( air_start_origin );
		const float landing_speed = speed;
		const float speed_gain = landing_speed - std::max( 0.f, air_peak_speed );
		int strafes_count = 0;
		for ( std::size_t i = 0U; i + 1U < velocity_graph_samples.size( ); ++i ) {
			if ( velocity_graph_samples[ i ].time < air_start_time )
				break;
			const float cur_wish = velocity_graph_samples[ i ].wish_dir;
			const float next_wish = velocity_graph_samples[ i + 1U ].wish_dir;
			if ( std::fabs( std::remainder( cur_wish - next_wish, 360.f ) ) > 20.f )
				++strafes_count;
		}
		const bool made_longjump = airtime > 0.28f && distance >= 235.f && air_peak_speed >= 230.f;
		if ( !made_longjump && airtime > 0.20f && distance > 48.f ) {
			const int loss = std::clamp( 6 + static_cast< int >( ( 235.f - std::min( distance, 235.f ) ) * 0.035f ), 6, 14 );
			add_score_event( -loss, -std::max( 2, loss / 3 ), 0.05f );
		} else if ( made_longjump ) {
			add_score_event( 55 + static_cast< int >( std::min( 70.f, ( distance - 235.f ) * 0.35f ) ), 3, 0.28f );
			add_run_event( "LJ", false );
		}
		if ( GET_VARIABLE( g_variables.m_jump_stats_enable, bool ) && allow_movement_side_effects ) {
			const char* jump_type = g_movement.m_pixelsurf_data.m_in_pixel_surf ? "PX" :
			                        g_movement.m_jumpbug_data.m_can_jb ? "JB" :
			                        lj_active ? "LJ" : mj_active ? "MJ" : duck_pressed ? "CJ" : "J";
			std::ostringstream stats;
			stats << jump_type << " pre=" << static_cast< int >( std::max( 0.f, air_peak_speed ) )
			      << " land=" << static_cast< int >( landing_speed )
			      << " gain=" << static_cast< int >( speed_gain )
			      << " dist=" << static_cast< int >( distance )
			      << " air=" << std::fixed << std::setprecision( 2 ) << airtime
			      << " strafes=" << strafes_count;
			last_jump_stats_line = stats.str( );
			if ( GET_VARIABLE( g_variables.m_jump_stats_chat, bool ) )
				movement_print( last_jump_stats_line.c_str( ), false );
			if ( GET_VARIABLE( g_variables.m_jump_stats_debug_log, bool ) )
				debug_log( "jump_stats", last_jump_stats_line, true, 0.05f, "jump_stats" );
			if ( GET_VARIABLE( g_variables.m_jump_stats_play_sound, bool ) &&
			     landing_speed >= std::clamp( GET_VARIABLE( g_variables.m_jump_stats_sound_threshold, float ), 100.f, 450.f ) &&
			     now - last_jump_stats_sound_time >
			         std::clamp( GET_VARIABLE( g_variables.m_jump_stats_sound_cooldown, float ), 0.1f, 3.0f ) ) {
				const auto sound_path = GET_VARIABLE( g_variables.m_jump_stats_sound_path, std::string );
				if ( !sound_path.empty( ) && safe_play_ui_sound( sound_path, "jump_stats", 0.12f ) )
					last_jump_stats_sound_time = now;
			}
		}
		air_start_origin = c_vector{ };
	}

	if ( GET_VARIABLE( g_variables.m_pts_meter, bool ) ) {
		if ( speed > 260.f && speed_delta > 10.f && now - last_pts_tick > 0.18f ) {
			pts_value += 8 + static_cast< int >( std::min( 24.f, speed_delta * 0.45f ) );
			pts_event = true;
		}
		if ( just_left_ground && speed > 170.f ) {
			pts_value += 22;
			pts_event = true;
		}
		if ( just_landed && speed > 220.f ) {
			pts_value += 18 + static_cast< int >( std::min( 42.f, speed * 0.04f ) );
			pts_event = true;
		}
		if ( speed > 320.f && now - last_pts_tick > 0.85f ) {
			pts_value += 6;
			pts_event = true;
		}
	}
	if ( pts_event ) {
		pts_heat = std::min( 1.f, pts_heat + 0.26f );
		pts_flash = 1.f;
		last_pts_tick = now;
	}
	if ( GET_VARIABLE( g_variables.m_jump_strafe_feedback, bool ) || GET_VARIABLE( g_variables.m_aura_debt_counter, bool ) ||
	     GET_VARIABLE( g_variables.m_drain_check_widget, bool ) ) {
		if ( just_left_ground || just_landed ) {
			jump_feedback_delta = speed_delta;
			jump_feedback_alpha = 1.f;
			if ( jump_feedback_delta < -4.f )
				++aura_debt;
			if ( GET_VARIABLE( g_variables.m_jump_feedback_sound, bool ) && jump_feedback_delta > 6.f && now - last_feedback_sound_time > 0.18f ) {
				safe_play_ui_sound( "ui\\beep07.wav", "jump_feedback", 0.22f );
				last_feedback_sound_time = now;
			}
		}
	}
	pts_heat = std::max( 0.f, pts_heat - frame_time * ( grounded ? 0.23f : 0.13f ) );
	pts_flash = std::max( 0.f, pts_flash - frame_time * 2.8f );
	jump_feedback_alpha = std::max( 0.f, jump_feedback_alpha - frame_time * 1.9f );
	if ( pts_value > 0 && now - last_pts_tick > 4.5f )
		pts_value = std::max( 0, pts_value - static_cast< int >( 90.f * frame_time ) );

	if ( GET_VARIABLE( g_variables.m_run_analyzer, bool ) && run_active ) {
		const float min_speed = std::clamp( GET_VARIABLE( g_variables.m_run_analyzer_min_speed, float ), 120.f, 520.f );
		const float end_timeout = std::clamp( GET_VARIABLE( g_variables.m_run_analyzer_end_timeout, float ), 0.2f, 2.5f );
		const bool slow_end = speed < min_speed * 0.35f && now - run_last_event_time > end_timeout;
		const bool landed_end = grounded && now - run_last_event_time > end_timeout && now - run_start_time > 0.35f;
		if ( slow_end )
			finish_run( true, "speed dropped" );
		else if ( landed_end )
			finish_run( false, "timeout" );
	} else if ( !GET_VARIABLE( g_variables.m_run_analyzer, bool ) ) {
		run_active = false;
		run_failed = false;
		run_combo.clear( );
		run_binds.clear( );
		run_jumpbug_attempt = false;
		if ( m_session_hub_snapshot.last_run_summary == "none" )
			m_session_hub_snapshot.run_status = "idle";
	}

	auto update_session_hub_snapshot = [ & ]( ) {
		m_session_hub_snapshot.run_status = run_active ? "active" : m_session_hub_snapshot.run_status.empty( ) ? "idle" : m_session_hub_snapshot.run_status;
		m_session_hub_snapshot.run_max_speed = static_cast< int >( run_active ? run_max_speed : std::max( run_max_speed, speed ) );
		m_session_hub_snapshot.run_combo = run_active ? join_strings( run_combo, " + ", "none" ) : m_session_hub_snapshot.run_combo;
		m_session_hub_snapshot.last_fail_reason = run_fail_reason.empty( ) ? "none" : run_fail_reason;

		const bool px_assist_enabled = GET_VARIABLE( g_variables.m_pixel_surf_assist, bool );
		const bool px_assist_key =
			px_assist_enabled && g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) );
		m_session_hub_snapshot.pixelsurf_assist_status =
			!px_assist_enabled ? "off" : pixelsurf_state ? "locked" : px_assist_key ? "searching" : "ready";
		m_session_hub_snapshot.edgebug_status = !GET_VARIABLE( g_variables.edge_bug, bool ) ? "off" :
		                                        g_movement.m_edgebug_data.m_will_edgebug ? "predicted" :
		                                        g_movement.m_edgebug_data.m_will_fail ? "failed" :
		                                        eb_key_active ? "armed" : "ready";
		m_session_hub_snapshot.jumpbug_status = !GET_VARIABLE( g_variables.m_jump_bug, bool ) ? "off" :
		                                        jumpbug_state ? "success" : jb_key_active ? "armed" : "ready";
		m_session_hub_snapshot.px_database_status = !GET_VARIABLE( g_variables.m_px_database, bool ) ? "disabled" :
		                                           GET_VARIABLE( g_variables.m_px_database_auto_record, bool ) ? "recording" : "loaded";
		m_session_hub_snapshot.current_map_profile = map_name.empty( ) ? "none" : map_name;
		m_session_hub_snapshot.px_lines_current_map = 0;
		for ( const auto& path : px_paths ) {
			if ( map_name.empty( ) || path.map == map_name )
				++m_session_hub_snapshot.px_lines_current_map;
		}
		if ( const auto table = particle_effect_names_table( ) )
			m_session_hub_snapshot.particle_table_count = table->get_num_strings( );
		m_session_hub_snapshot.particle_mode = GET_VARIABLE( g_variables.m_debug_logging, bool ) ? "debug/experimental visible" : "working/default";
		m_session_hub_snapshot.last_particle_error = g_drainware_particle_health.empty( ) ? "none" : g_drainware_particle_health;
	};
	update_session_hub_snapshot( );

	last_jump_pressed = jump_pressed;
	last_duck_pressed = duck_pressed;
	last_lj_active = lj_active;
	last_mj_active = mj_active;
	last_eb_key_active = eb_key_active;
	last_px_key_active = px_key_active;
	last_jb_key_active = jb_key_active;

	last_grounded = grounded;
	last_speed = speed;

	auto draw_edge_vignette = [ & ]( ImColor edge_color, float alpha ) {
		alpha = std::clamp( alpha, 0.f, 1.f );
		if ( alpha <= 0.01f )
			return;

		const float thickness = 84.f;
		const auto col = ImColor( edge_color.Value.x, edge_color.Value.y, edge_color.Value.z, alpha );
		const auto clear = ImColor( edge_color.Value.x, edge_color.Value.y, edge_color.Value.z, 0.f );
		draw->AddRectFilledMultiColor( ImVec2( 0.f, 0.f ), ImVec2( width, thickness ), col, col, clear, clear );
		draw->AddRectFilledMultiColor( ImVec2( 0.f, height - thickness ), ImVec2( width, height ), clear, clear, col, col );
		draw->AddRectFilledMultiColor( ImVec2( 0.f, 0.f ), ImVec2( thickness, height ), col, clear, clear, col );
		draw->AddRectFilledMultiColor( ImVec2( width - thickness, 0.f ), ImVec2( width, height ), clear, col, col, clear );
	};

	if ( GET_VARIABLE( g_variables.m_world_glow_lights, bool ) )
		draw_edge_vignette( accent, 0.10f );

	c_angle view_angles{ };
	g_interfaces.m_engine_client->get_view_angles( view_angles );
	const c_vector forward = c_vector::fromAngle( c_vector( view_angles.m_x, view_angles.m_y, view_angles.m_z ) );
	const auto active_weapon =
		g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_active_weapon_handle( ) );
	const auto weapon_data = active_weapon ? g_interfaces.m_weapon_system->get_weapon_data( active_weapon->get_item_definition_index( ) ) : nullptr;
	const bool weapon_is_gun = weapon_data && weapon_data->is_gun( );
	const ImVec2 center( width * 0.5f, height * 0.5f );

	if ( g_particle_debug_request >= 0 ) {
		const int request = std::exchange( g_particle_debug_request, -1 );
		const int feature = std::clamp( GET_VARIABLE( g_variables.m_particle_debug_feature, int ), 0, 2 );
		const int particle = std::clamp( GET_VARIABLE( g_variables.m_particle_debug_particle, int ), 0, static_cast< int >( k_particle_options.size( ) ) - 1 );
		const auto custom_particle = GET_VARIABLE( g_variables.m_particle_debug_name, std::string );
		const char* feature_name = feature == 0 ? "particles][footstep" : feature == 1 ? "particles][edgebug" : "particles][tracer";

		if ( request == 3 ) {
			for ( const char* known_particle : k_known_particle_validation_names ) {
				const auto result = precache_csgo_particle( known_particle );
				debug_log( "particles][validate",
				           std::string( "selected=" ) + known_particle + " result=" + ( result.ok ? "ok" : "failed" ) +
				               " index=" + std::to_string( result.index ) + " reason=" + result.reason +
				               " table=ParticleEffectNames map=" + current_map_name( ),
				           GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.f, nullptr );
			}
			return;
		}

		const c_vector feet = g_ctx.m_local->get_abs_origin( );
		const c_vector eye = g_ctx.m_local->get_eye_position( false );
		c_vector impact = eye + forward * 1800.f;
		if ( g_interfaces.m_engine_trace ) {
			trace_t trace{ };
			c_trace_filter filter( g_ctx.m_local );
			g_interfaces.m_engine_trace->trace_ray( ray_t( eye, impact ), mask_shot, &filter, &trace );
			if ( trace.m_fraction < 1.f )
				impact = trace.m_end;
		}

		const bool use_custom_particle = !custom_particle.empty( );
		if ( request == 0 ) {
			if ( use_custom_particle )
				emit_particle_by_name( feature_name, custom_particle.c_str( ), feet, 1.f, nullptr, accent_cfg, 0.22f );
			else
				emit_selected_particle( feature_name, feet, particle, 1.f, nullptr, accent_cfg, 0.22f );
		} else if ( request == 1 ) {
			if ( use_custom_particle )
				emit_particle_by_name( feature_name, custom_particle.c_str( ), impact, 1.f, nullptr, accent_cfg, 0.22f );
			else
				emit_selected_particle( feature_name, impact, particle, 1.f, nullptr, accent_cfg, 0.22f );
		} else {
			if ( use_custom_particle )
				emit_particle_by_name( "particles][tracer", custom_particle.c_str( ), eye, 1.f, &impact, accent_cfg, 0.35f );
			else
				emit_selected_particle( "particles][tracer", eye, particle, 1.f, &impact, accent_cfg, 0.35f );
		}
	}

	auto draw_cross = [ & ]( const ImVec2& position, const ImColor& color, const float radius, const float thickness = 1.2f ) {
		draw->AddLine( ImVec2( position.x - radius, position.y ), ImVec2( position.x - 2.f, position.y ), ImColor( 0, 0, 0, 190 ),
		               thickness + 1.2f );
		draw->AddLine( ImVec2( position.x + 2.f, position.y ), ImVec2( position.x + radius, position.y ), ImColor( 0, 0, 0, 190 ),
		               thickness + 1.2f );
		draw->AddLine( ImVec2( position.x, position.y - radius ), ImVec2( position.x, position.y - 2.f ), ImColor( 0, 0, 0, 190 ),
		               thickness + 1.2f );
		draw->AddLine( ImVec2( position.x, position.y + 2.f ), ImVec2( position.x, position.y + radius ), ImColor( 0, 0, 0, 190 ),
		               thickness + 1.2f );
		draw->AddLine( ImVec2( position.x - radius, position.y ), ImVec2( position.x - 2.f, position.y ), color, thickness );
		draw->AddLine( ImVec2( position.x + 2.f, position.y ), ImVec2( position.x + radius, position.y ), color, thickness );
		draw->AddLine( ImVec2( position.x, position.y - radius ), ImVec2( position.x, position.y - 2.f ), color, thickness );
		draw->AddLine( ImVec2( position.x, position.y + 2.f ), ImVec2( position.x, position.y + radius ), color, thickness );
	};

	if ( GET_VARIABLE( g_variables.m_force_crosshair, bool ) )
		draw_cross( center, ImColor( 235, 235, 235, 230 ), 7.f );

	if ( GET_VARIABLE( g_variables.m_recoil_crosshair, bool ) ) {
		const auto punch = g_ctx.m_local->get_punch( );
		const ImVec2 recoil_pos( center.x - punch.m_y * 10.f, center.y + punch.m_x * 10.f );
		draw_cross( recoil_pos, ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.95f ), 8.f, 1.4f );
	}

	if ( GET_VARIABLE( g_variables.m_spread_circle, bool ) && active_weapon && weapon_is_gun ) {
		const auto spread_color = GET_VARIABLE( g_variables.m_spread_circle_color, c_color );
		const float spread_radius = std::clamp( ( active_weapon->get_spread( ) + active_weapon->get_inaccuracy( ) ) * 560.f, 3.f, 180.f );
		draw->AddCircle( center, spread_radius, ImColor( spread_color[ 0 ], spread_color[ 1 ], spread_color[ 2 ], spread_color[ 3 ] ), 96, 1.15f );
	}

	if ( GET_VARIABLE( g_variables.m_penetration_reticle, bool ) && active_weapon && weapon_is_gun ) {
		static float last_penetration_check = 0.f;
		static bool can_penetrate = false;
		if ( now - last_penetration_check > 0.14f ) {
			can_penetrate = g_auto_wall.get_damage( g_ctx.m_local->get_eye_position( false ) + forward * 8192.f ) > 1.f;
			last_penetration_check = now;
		}

		const ImColor reticle_color = can_penetrate ? ImColor( 1, 192, 0, 220 ) : ImColor( 220, 45, 55, 190 );
		draw->AddCircleFilled( center, 3.3f, ImColor( 0, 0, 0, 190 ), 24 );
		draw->AddCircleFilled( center, 2.1f, reticle_color, 24 );
	}

	if ( GET_VARIABLE( g_variables.m_bullet_tracer, bool ) && active_weapon && weapon_is_gun &&
	     ( LI_FN( GetAsyncKeyState )( VK_LBUTTON ) & 0x8000 ) && now - last_tracer_time > 0.08f ) {
		const c_vector start = g_ctx.m_local->get_eye_position( false );
		c_vector end = start + forward * 1800.f;
		if ( g_interfaces.m_engine_trace ) {
			trace_t trace{ };
			c_trace_filter filter( g_ctx.m_local );
			g_interfaces.m_engine_trace->trace_ray( ray_t( start, end ), mask_shot, &filter, &trace );
			if ( trace.m_fraction < 1.f )
				end = trace.m_end;
		}
		const auto tracer_color = GET_VARIABLE( g_variables.m_bullet_tracer_use_accent, bool )
		                              ? accent_cfg
		                              : GET_VARIABLE( g_variables.m_bullet_tracer_color, c_color );
		const int tracer_type = std::clamp( GET_VARIABLE( g_variables.m_bullet_tracer_type, int ), 0,
		                                    static_cast< int >( k_tracer_particle_names.size( ) ) - 1 );
		const float intensity = std::clamp( GET_VARIABLE( g_variables.m_bullet_tracer_intensity, float ), 0.25f, 4.f );
		const float lifetime = std::clamp( GET_VARIABLE( g_variables.m_bullet_tracer_lifetime, float ), 0.06f, 1.5f );
		emit_particle_by_name( "particles][tracer", tracer_particle_name( tracer_type ), start, intensity, &end, tracer_color, lifetime );
		last_tracer_time = now;
	}

	const bool footstep_enabled = GET_VARIABLE( g_variables.m_footstep_fx_enabled, bool );
	const int footstep_mode = std::clamp( GET_VARIABLE( g_variables.m_footstep_fx_mode, int ), 0, static_cast< int >( k_particle_options.size( ) ) - 1 );
	if ( footstep_mode != last_footstep_mode ) {
		last_footstep_origin = c_vector{ };
		last_footstep_time = 0.f;
		last_footstep_mode = footstep_mode;
	}

	if ( !footstep_enabled || footstep_mode == 0 || !grounded || speed <= 35.f )
		last_footstep_origin = c_vector{ };

	const float footstep_distance = std::clamp( 28.f + speed * 0.02f, 28.f, 48.f );
	const float footstep_cooldown = std::clamp( GET_VARIABLE( g_variables.m_footstep_fx_cooldown, float ), 0.06f, 0.65f );
	if ( allow_movement_side_effects && footstep_enabled && footstep_mode > 0 && grounded && speed > 35.f &&
	     ( last_footstep_origin.is_zero( ) || g_ctx.m_local->get_abs_origin( ).dist_to_2d( last_footstep_origin ) > footstep_distance ) &&
	     now - last_footstep_time > footstep_cooldown ) {
		last_footstep_origin = g_ctx.m_local->get_abs_origin( );
		const int resolved_mode = footstep_mode == 1 ? resolve_surface_particle_mode( last_footstep_origin ) : footstep_mode;
		emit_selected_particle( "particles][footstep", last_footstep_origin, resolved_mode, GET_VARIABLE( g_variables.m_footstep_fx_intensity, float ),
		                        nullptr, accent_cfg, 0.18f );
		last_footstep_time = now;
	} else if ( allow_movement_side_effects && footstep_enabled && footstep_mode > 0 && grounded && speed > 35.f ) {
		const float remaining = footstep_cooldown - ( now - last_footstep_time );
		if ( remaining > 0.f ) {
			std::ostringstream message;
			message << "skipped=cooldown remaining=" << std::fixed << std::setprecision( 2 ) << remaining;
			debug_log( "particles][footstep", message.str( ), GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.18f, "footstep_cooldown" );
		}
	}

	if ( GET_VARIABLE( g_variables.m_px_database, bool ) && g_interfaces.m_debug_overlay ) {
		const auto line_color = GET_VARIABLE( g_variables.m_px_database_use_accent, bool ) ? accent_cfg : GET_VARIABLE( g_variables.m_px_database_color, c_color );
		const float distance_limit = std::clamp( GET_VARIABLE( g_variables.m_px_database_distance, float ), 120.f, 8000.f );
		const float line_thickness = std::clamp( GET_VARIABLE( g_variables.m_px_database_thickness, float ), 1.f, 6.f );
		const bool draw_through = GET_VARIABLE( g_variables.m_px_database_draw_through_walls, bool );
		for ( const auto& path : px_paths ) {
			if ( GET_VARIABLE( g_variables.m_px_database_current_map_only, bool ) && path.map != map_name )
				continue;
			if ( g_ctx.m_local->get_abs_origin( ).dist_to( path.start ) > distance_limit &&
			     g_ctx.m_local->get_abs_origin( ).dist_to( path.end ) > distance_limit )
				continue;

			if ( draw_through )
				g_interfaces.m_debug_overlay->add_line_overlay_alpha( path.start, path.end, line_color[ 0 ], line_color[ 1 ], line_color[ 2 ],
				                                                      line_color[ 3 ], true, 0.035f );
			else
				g_interfaces.m_debug_overlay->add_line_overlay( path.start, path.end, line_color[ 0 ], line_color[ 1 ], line_color[ 2 ],
				                                                line_color[ 3 ], line_thickness, 0.035f );
		}
	}

	if ( GET_VARIABLE( g_variables.m_predicted_grenade_path, bool ) && active_weapon ) {
		const short definition_index = active_weapon->get_item_definition_index( );
		const bool is_grenade = g_utilities.is_in< short >( definition_index,
		                                                    { weapon_flashbang, weapon_hegrenade, weapon_smokegrenade, weapon_molotov,
		                                                      weapon_decoy, weapon_incgrenade } );
		if ( is_grenade ) {
			const auto path_color = GET_VARIABLE( g_variables.m_predicted_grenade_path_color, c_color );
			c_vector position = g_ctx.m_local->get_eye_position( false ) + forward * 16.f;
			c_vector velocity = forward * 362.f + g_ctx.m_local->get_velocity( ) * 0.45f;
			const float step = 1.f / 32.f;
			c_vector_2d last_screen{ };
			bool has_last = false;
			for ( int i = 0; i < 96; ++i ) {
				const c_vector next = position + velocity * step;
				velocity.m_z -= 800.f * step;

				c_vector_2d screen_pos{ };
				if ( g_render.world_to_screen( next, screen_pos ) ) {
					if ( has_last )
						draw->AddLine( ImVec2( last_screen.m_x, last_screen.m_y ), ImVec2( screen_pos.m_x, screen_pos.m_y ),
						               ImColor( path_color[ 0 ], path_color[ 1 ], path_color[ 2 ], path_color[ 3 ] ), 1.4f );
					last_screen = screen_pos;
					has_last = true;
				} else {
					has_last = false;
				}

				position = next;
				if ( i > 16 && position.m_z < g_ctx.m_local->get_abs_origin( ).m_z - 240.f )
					break;
			}
		}
	}

	if ( GET_VARIABLE( g_variables.m_velocity_trail, bool ) || GET_VARIABLE( g_variables.m_jump_trail, bool ) ) {
		for ( std::size_t i = 0U; i + 1U < movement_samples.size( ); ++i ) {
			if ( GET_VARIABLE( g_variables.m_jump_trail, bool ) && movement_samples[ i ].grounded &&
			     movement_samples[ i + 1U ].grounded && !GET_VARIABLE( g_variables.m_velocity_trail, bool ) )
				continue;

			c_vector_2d current{ }, next{ };
			if ( !g_render.world_to_screen( movement_samples[ i ].origin, current ) ||
			     !g_render.world_to_screen( movement_samples[ i + 1U ].origin, next ) )
				continue;

			const float age = std::clamp( ( now - movement_samples[ i ].time ) / 1.35f, 0.f, 1.f );
			const float alpha = ( 1.f - age ) * std::clamp( movement_samples[ i ].speed / 430.f, 0.18f, 0.85f );
			draw->AddLine( ImVec2( current.m_x, current.m_y ), ImVec2( next.m_x, next.m_y ),
			               ImColor( accent.Value.x, accent.Value.y, accent.Value.z, alpha ), 2.0f );
		}
	}

	if ( GET_VARIABLE( g_variables.m_local_afterimage, bool ) ) {
		for ( std::size_t i = 0U; i < movement_samples.size( ); i += 4U ) {
			c_vector origin = movement_samples[ i ].origin;
			origin.m_z += 10.f;
			c_vector_2d screen{ };
			if ( !g_render.world_to_screen( origin, screen ) )
				continue;

			const float age = std::clamp( ( now - movement_samples[ i ].time ) / 1.35f, 0.f, 1.f );
			const float alpha = ( 1.f - age ) * 0.26f;
			const float radius = 5.f + std::clamp( movement_samples[ i ].speed / 45.f, 1.f, 8.f );
			draw->AddCircle( ImVec2( screen.m_x, screen.m_y ), radius, ImColor( accent.Value.x, accent.Value.y, accent.Value.z, alpha ), 24, 1.4f );
		}
	}

	if ( GET_VARIABLE( g_variables.m_strafe_aura, bool ) ) {
		c_vector aura_origin = g_ctx.m_local->get_abs_origin( );
		aura_origin.m_z += 22.f;
		c_vector_2d aura_screen{ };
		if ( g_render.world_to_screen( aura_origin, aura_screen ) ) {
			const float pulse = 0.5f + std::sin( now * 5.5f ) * 0.5f;
			const float radius = 18.f + std::clamp( speed / 18.f, 0.f, 28.f ) + pulse * 5.f;
			const float alpha = std::clamp( speed / 430.f, 0.18f, 0.65f );
			draw->AddCircle( ImVec2( aura_screen.m_x, aura_screen.m_y ), radius, ImColor( accent.Value.x, accent.Value.y, accent.Value.z, alpha ),
			                 72, 2.0f );
			draw->AddCircle( ImVec2( aura_screen.m_x, aura_screen.m_y ), radius * 0.62f,
			                 ImColor( 255, 255, 255, static_cast< int >( 55.f * alpha ) ), 54, 1.0f );
		}
	}

	if ( GET_VARIABLE( g_variables.m_speed_pulse_overlay, bool ) ) {
		const float alpha = pulse_alpha * 0.32f;
		if ( alpha > 0.01f ) {
			draw->AddRect( ImVec2( 8.f, 8.f ), ImVec2( width - 8.f, height - 8.f ),
			               ImColor( accent.Value.x, accent.Value.y, accent.Value.z, alpha ), 8.f, ImDrawFlags_RoundCornersAll, 2.2f );
			draw_edge_vignette( accent, alpha * 0.42f );
		}
	}

	if ( GET_VARIABLE( g_variables.m_pts_meter, bool ) ) {
		const ImVec2 pos( static_cast< float >( GET_VARIABLE( g_variables.m_pts_meter_x, int ) ),
		                  static_cast< float >( GET_VARIABLE( g_variables.m_pts_meter_y, int ) ) );
		const ImVec2 size( 176.f, 62.f );
		const ImColor pts_color( pts_color_cfg[ 0 ], pts_color_cfg[ 1 ], pts_color_cfg[ 2 ], pts_color_cfg[ 3 ] );
		const float flash = 0.74f + pts_flash * 0.26f;
		const ImVec2 pos_max( pos.x + size.x, pos.y + size.y );
		draw->AddRectFilled( pos, pos_max, ImColor( 9, 9, 11, 210 ), 6.f );
		draw->AddRect( pos, pos_max, ImColor( 42, 42, 44, 170 ), 6.f );
		draw->AddRectFilled( pos, ImVec2( pos.x + size.x * pts_heat, pos.y + 3.f ),
		                      ImColor( pts_color.Value.x, pts_color.Value.y, pts_color.Value.z, 0.95f * flash ), 6.f, ImDrawFlags_RoundCornersTop );
		ImGui::PushFont( fonts_for_gui::bolder_11 );
		draw->AddText( ImVec2( pos.x + 10.f, pos.y + 9.f ), ImColor( 220, 220, 220, 255 ), "PTS" );
		const std::string pts_text = std::to_string( pts_value );
		draw->AddText( ImVec2( pos.x + 48.f, pos.y + 8.f ), ImColor( 255, 255, 255, 255 ), pts_text.c_str( ) );
		ImGui::PopFont( );
		ImGui::PushFont( fonts_for_gui::regular_11 );
		const std::string speed_text = "speed " + std::to_string( static_cast< int >( speed ) );
		draw->AddText( ImVec2( pos.x + 10.f, pos.y + 35.f ), ImColor( 175, 175, 175, 235 ), speed_text.c_str( ) );
		const char* state_text = grounded ? "flow" : "air";
		draw->AddText( ImVec2( pos.x + size.x - 42.f, pos.y + 35.f ), ImColor( pts_color.Value.x, pts_color.Value.y, pts_color.Value.z, 1.f ),
		               state_text );
		ImGui::PopFont( );
	}

	if ( GET_VARIABLE( g_variables.m_velocity_graph, bool ) ) {
		if ( GET_VARIABLE( g_variables.m_velocity_graph_reset, bool ) ) {
			velocity_graph_samples.clear( );
			GET_VARIABLE( g_variables.m_velocity_graph_reset, bool ) = false;
		}
		const ImVec2 graph_pos( static_cast< float >( GET_VARIABLE( g_variables.m_velocity_graph_x, int ) ),
		                        static_cast< float >( GET_VARIABLE( g_variables.m_velocity_graph_y, int ) ) );
		const ImVec2 graph_size( static_cast< float >( std::clamp( GET_VARIABLE( g_variables.m_velocity_graph_w, int ), 140, 640 ) ),
		                         static_cast< float >( std::clamp( GET_VARIABLE( g_variables.m_velocity_graph_h, int ), 56, 260 ) ) );
		const bool compact = GET_VARIABLE( g_variables.m_velocity_graph_compact, bool );
		const auto graph_col_cfg = GET_VARIABLE( g_variables.m_velocity_graph_use_accent, bool ) ?
		                           GET_VARIABLE( g_variables.m_accent, c_color ) :
		                           GET_VARIABLE( g_variables.m_velocity_graph_color, c_color );
#if LOBO_USE_IMPLOT
		static std::vector< double > graph_x;
		static std::vector< double > speed_values;
		static std::vector< double > accel_values;
		static std::vector< double > vertical_values;
		static std::vector< double > marker_x;
		static std::vector< std::string > marker_labels;
		static std::vector< double > direction_x;
		static std::vector< std::string > direction_labels;
		static std::vector< ImU32 > speed_line_colors;

		const int sample_count = static_cast< int >( velocity_graph_samples.size( ) );
		graph_x.resize( sample_count );
		speed_values.resize( sample_count );
		accel_values.resize( sample_count );
		vertical_values.resize( sample_count );
		speed_line_colors.resize( sample_count );
		marker_x.clear( );
		marker_labels.clear( );
		direction_x.clear( );
		direction_labels.clear( );

		float max_speed_value = 260.f;
		for ( int i = 0; i < sample_count; ++i ) {
			const auto& sample = velocity_graph_samples[ static_cast< std::size_t >( sample_count - 1 - i ) ];
			max_speed_value = std::max( max_speed_value, sample.speed + 36.f );
		}
		max_speed_value = std::clamp( max_speed_value, 260.f, 720.f );

		const auto graph_color = [ & ]( int alpha = 255 ) {
			return ImVec4( graph_col_cfg[ 0 ] / 255.f, graph_col_cfg[ 1 ] / 255.f, graph_col_cfg[ 2 ] / 255.f,
			               std::clamp( static_cast< float >( alpha ) / 255.f, 0.f, 1.f ) );
		};
		for ( int i = 0; i < sample_count; ++i ) {
			const auto& sample = velocity_graph_samples[ static_cast< std::size_t >( sample_count - 1 - i ) ];
			graph_x[ i ] = static_cast< double >( i );
			speed_values[ i ] = std::clamp( sample.speed, 0.f, max_speed_value );
			accel_values[ i ] = max_speed_value * 0.42 + std::clamp( sample.accel, -90.f, 90.f ) * 1.35;
			vertical_values[ i ] = max_speed_value * 0.50 + std::clamp( sample.vertical, -320.f, 320.f ) / 320.0 * max_speed_value * 0.34;
			const float age_alpha = std::clamp( 0.35f + 0.65f * ( static_cast< float >( i + 1 ) / static_cast< float >( std::max( sample_count, 1 ) ) ),
			                                    0.2f, 1.f );
			if ( GET_VARIABLE( g_variables.m_velocity_graph_show_gain_loss, bool ) ) {
				const bool gain = sample.accel >= 0.f;
				speed_line_colors[ i ] = gain ? ImColor( 130, 255, 146, static_cast< int >( 230.f * age_alpha ) )
				                              : ImColor( 255, 96, 96, static_cast< int >( 220.f * age_alpha ) );
			} else {
				speed_line_colors[ i ] =
					ImColor( graph_col_cfg[ 0 ], graph_col_cfg[ 1 ], graph_col_cfg[ 2 ], static_cast< int >( 230.f * age_alpha ) );
			}

			if ( GET_VARIABLE( g_variables.m_velocity_graph_show_tech_markers, bool ) && sample.marker && sample.marker[ 0 ] ) {
				marker_x.push_back( graph_x[ i ] );
				marker_labels.emplace_back( sample.marker );
			}
			if ( GET_VARIABLE( g_variables.m_velocity_graph_show_dir_labels, bool ) && i > 0 ) {
				const auto& previous = velocity_graph_samples[ static_cast< std::size_t >( sample_count - i ) ];
				if ( std::strcmp( sample.direction, previous.direction ) != 0 ) {
					direction_x.push_back( graph_x[ i ] );
					direction_labels.emplace_back( sample.direction );
				}
			}
		}

		ImGui::SetNextWindowPos( graph_pos, ImGuiCond_Always );
		ImGui::SetNextWindowSize( graph_size, ImGuiCond_Always );
		ImGui::SetNextWindowBgAlpha( compact ? 0.18f : 0.32f );
		ImGuiWindowFlags graph_window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
		if ( !g_menu.m_opened )
			graph_window_flags |= ImGuiWindowFlags_NoInputs;
		ImGui::Begin( "##velocity_graph_implot_window", nullptr, graph_window_flags );
		ImGui::PushFont( fonts_for_gui::regular_11 );
		const char* current_dir = velocity_graph_samples.empty( ) ? "unknown" : velocity_graph_samples.front( ).direction;
		if ( compact )
			ImGui::TextColored( graph_color( 235 ), "%d", static_cast< int >( speed ) );
		else {
			ImGui::TextColored( ImVec4( 0.82f, 0.82f, 0.84f, 0.95f ), "velocity / strafe" );
			if ( GET_VARIABLE( g_variables.m_velocity_graph_show_dir_labels, bool ) ) {
				ImGui::SameLine( );
				ImGui::TextColored( graph_color( 230 ), "%s", current_dir );
			}
		}
		ImGui::PopFont( );

		const float plot_header = compact ? 0.f : 16.f;
		ImPlotFlags plot_flags = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText;
		if ( !g_menu.m_opened || compact )
			plot_flags |= ImPlotFlags_NoInputs;
		const ImPlotAxisFlags x_flags =
			compact ? ImPlotAxisFlags_NoDecorations : ( ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoMenus );
		const ImPlotAxisFlags y_flags =
			compact ? ImPlotAxisFlags_NoDecorations : ( ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoMenus );

		if ( ImPlot::BeginPlot( "##velocity_graph_implot", ImVec2( -1.f, graph_size.y - plot_header - 8.f ), plot_flags ) ) {
			ImPlot::SetupAxes( nullptr, nullptr, x_flags, y_flags );
			ImPlot::SetupAxesLimits( 0.0, static_cast< double >( std::max( sample_count - 1, 1 ) ), 0.0, static_cast< double >( max_speed_value ),
			                         ImPlotCond_Always );

			if ( sample_count > 1 && GET_VARIABLE( g_variables.m_velocity_graph_show_velocity, bool ) ) {
				ImPlotSpec speed_spec;
				speed_spec.LineColor = graph_color( 235 );
				speed_spec.LineColors = speed_line_colors.empty( ) ? nullptr : speed_line_colors.data( );
				speed_spec.LineWeight = compact ? 1.7f : 2.1f;
				speed_spec.Flags = ImPlotItemFlags_NoLegend;
				ImPlot::PlotLine( "speed", graph_x.data( ), speed_values.data( ), sample_count, speed_spec );
			}
			if ( sample_count > 1 && GET_VARIABLE( g_variables.m_velocity_graph_show_accel, bool ) ) {
				ImPlotSpec accel_spec;
				accel_spec.LineColor = ImVec4( 0.58f, 1.00f, 0.55f, compact ? 0.30f : 0.45f );
				accel_spec.LineWeight = 1.0f;
				accel_spec.Flags = ImPlotItemFlags_NoLegend;
				ImPlot::PlotLine( "gain", graph_x.data( ), accel_values.data( ), sample_count, accel_spec );
			}
			if ( sample_count > 1 && GET_VARIABLE( g_variables.m_velocity_graph_show_vertical, bool ) ) {
				ImPlotSpec vertical_spec;
				vertical_spec.LineColor = ImVec4( 0.43f, 0.66f, 1.00f, compact ? 0.32f : 0.50f );
				vertical_spec.LineWeight = 1.0f;
				vertical_spec.Flags = ImPlotItemFlags_NoLegend;
				ImPlot::PlotLine( "vertical", graph_x.data( ), vertical_values.data( ), sample_count, vertical_spec );
			}
			if ( !marker_x.empty( ) ) {
				ImPlotSpec marker_spec;
				marker_spec.LineColor = graph_color( 175 );
				marker_spec.LineWeight = 1.0f;
				marker_spec.Flags = ImPlotItemFlags_NoLegend;
				ImPlot::PlotInfLines( "tech", marker_x.data( ), static_cast< int >( marker_x.size( ) ), marker_spec );

				ImPlotSpec text_spec;
				text_spec.LineColor = graph_color( 240 );
				text_spec.Flags = ImPlotItemFlags_NoLegend;
				for ( std::size_t i = 0; i < marker_x.size( ); ++i )
					ImPlot::PlotText( marker_labels[ i ].c_str( ), marker_x[ i ], max_speed_value * 0.90, ImVec2( 0.f, -6.f ), text_spec );
			}
			if ( GET_VARIABLE( g_variables.m_velocity_graph_show_dir_labels, bool ) && !direction_x.empty( ) && !compact ) {
				ImPlotSpec dir_spec;
				dir_spec.LineColor = ImVec4( 0.82f, 0.82f, 0.84f, 0.72f );
				dir_spec.Flags = ImPlotItemFlags_NoLegend;
				for ( std::size_t i = 0; i < direction_x.size( ); ++i )
					ImPlot::PlotText( direction_labels[ i ].c_str( ), direction_x[ i ], max_speed_value * 0.12, ImVec2( 3.f, 0.f ), dir_spec );
			}
			ImPlot::EndPlot( );
		}
		ImGui::End( );
#else
		const ImVec2 graph_max( graph_pos.x + graph_size.x, graph_pos.y + graph_size.y );
		draw->AddRectFilled( graph_pos, graph_max, ImColor( 9, 9, 11, compact ? 140 : 185 ), 6.f );
		draw->AddRect( graph_pos, graph_max, ImColor( graph_col_cfg[ 0 ], graph_col_cfg[ 1 ], graph_col_cfg[ 2 ], 105 ), 6.f );
		draw->AddText( ImVec2( graph_pos.x + 8.f, graph_pos.y + 6.f ), ImColor( 210, 210, 210, 230 ), "velocity graph requires ImPlot" );
#endif
	}

	if ( GET_VARIABLE( g_variables.m_keystrokes_overlay, bool ) ) {
		struct key_state_t {
			const char* label;
			bool down;
			float alpha;
		};
		static std::array< float, 16 > key_alpha{ };
		const float dt_anim = std::clamp( frame_time * std::clamp( GET_VARIABLE( g_variables.m_keystrokes_anim_speed, float ), 1.f, 20.f ), 0.f, 1.f );
		const float scale = std::clamp( GET_VARIABLE( g_variables.m_keystrokes_scale, float ), 0.65f, 2.f );
		const float spacing = std::clamp( GET_VARIABLE( g_variables.m_keystrokes_spacing, float ), 0.f, 22.f );
		const auto key_color_cfg = GET_VARIABLE( g_variables.m_keystrokes_use_accent, bool ) ? GET_VARIABLE( g_variables.m_accent, c_color ) :
		                           GET_VARIABLE( g_variables.m_keystrokes_color, c_color );
		const float bg_alpha = std::clamp( GET_VARIABLE( g_variables.m_keystrokes_bg_alpha, float ), 0.f, 1.f );
		float x = static_cast< float >( GET_VARIABLE( g_variables.m_keystrokes_x, int ) );
		float y = static_cast< float >( GET_VARIABLE( g_variables.m_keystrokes_y, int ) );
		const float key_w = 30.f * scale;
		const float key_h = 22.f * scale;
		const float overlay_w = ( key_w + spacing ) * 3.f + key_w;
		const float overlay_h = ( key_h + spacing ) * 4.f + key_h;
		if ( GET_VARIABLE( g_variables.m_keystrokes_draggable, bool ) && g_menu.m_opened ) {
			static bool dragging = false;
			static ImVec2 drag_offset{ };
			const ImVec2 min( x, y );
			const ImVec2 max( x + overlay_w, y + overlay_h );
			if ( ImGui::IsMouseHoveringRect( min, max ) && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
				dragging = true;
				drag_offset = ImVec2( ImGui::GetMousePos( ).x - x, ImGui::GetMousePos( ).y - y );
			}
			if ( dragging ) {
				if ( ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
					x = ImGui::GetMousePos( ).x - drag_offset.x;
					y = ImGui::GetMousePos( ).y - drag_offset.y;
					GET_VARIABLE( g_variables.m_keystrokes_x, int ) = std::clamp< int >( static_cast< int >( x ), 0, std::max( 0, static_cast< int >( g_ctx.m_width ) - 24 ) );
					GET_VARIABLE( g_variables.m_keystrokes_y, int ) = std::clamp< int >( static_cast< int >( y ), 0, std::max( 0, static_cast< int >( g_ctx.m_height ) - 24 ) );
				} else {
					dragging = false;
				}
			}
		}
		const auto down = []( int vk ) { return ( GetAsyncKeyState( vk ) & 0x8000 ) != 0; };
		std::vector< key_state_t > keys{
			{ "W", down( 'W' ), 0.f }, { "A", down( 'A' ), 0.f }, { "S", down( 'S' ), 0.f }, { "D", down( 'D' ), 0.f },
			{ "JUMP", down( VK_SPACE ), 0.f }, { "DUCK", down( VK_CONTROL ), 0.f }, { "WALK", down( VK_SHIFT ), 0.f },
			{ "M1", down( VK_LBUTTON ), 0.f }, { "M2", down( VK_RBUTTON ), 0.f },
			{ "LJ", GET_VARIABLE( g_variables.m_long_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) ), 0.f },
			{ "JB", GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ), 0.f },
			{ "EB", GET_VARIABLE( g_variables.edge_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ), 0.f },
			{ "PX", GET_VARIABLE( g_variables.m_pixel_surf, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) ), 0.f },
			{ "AST", GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ), 0.f },
		};
		const std::size_t mouse_first = 7U;
		const std::size_t mouse_last = 9U;
		const std::size_t bind_first = 9U;
		const std::size_t bind_last = 13U;
		if ( !GET_VARIABLE( g_variables.m_keystrokes_show_mouse, bool ) )
			keys.erase( keys.begin( ) + std::min( mouse_first, keys.size( ) ), keys.begin( ) + std::min( mouse_last, keys.size( ) ) );
		if ( !GET_VARIABLE( g_variables.m_keystrokes_show_movement_binds, bool ) )
			keys.erase( keys.begin( ) + std::min( bind_first, keys.size( ) ), keys.begin( ) + std::min( bind_last, keys.size( ) ) );
		if ( !GET_VARIABLE( g_variables.m_keystrokes_show_assist_binds, bool ) )
			keys.erase( keys.begin( ) + std::min< std::size_t >( keys.size( ), bind_last ), keys.end( ) );
		for ( std::size_t i = 0; i < keys.size( ) && i < key_alpha.size( ); ++i ) {
			key_alpha[ i ] = std::lerp( key_alpha[ i ], keys[ i ].down ? 1.f : 0.f, dt_anim );
			keys[ i ].alpha = key_alpha[ i ];
		}

		const std::array< ImVec2, 14 > pos{ ImVec2( key_w + spacing, 0.f ),
		                                     ImVec2( 0.f, key_h + spacing ), ImVec2( key_w + spacing, key_h + spacing ),
		                                     ImVec2( ( key_w + spacing ) * 2.f, key_h + spacing ),
		                                     ImVec2( 0.f, ( key_h + spacing ) * 2.f ), ImVec2( key_w + spacing, ( key_h + spacing ) * 2.f ),
		                                     ImVec2( ( key_w + spacing ) * 2.f, ( key_h + spacing ) * 2.f ),
		                                     ImVec2( 0.f, ( key_h + spacing ) * 3.f ), ImVec2( key_w + spacing, ( key_h + spacing ) * 3.f ),
		                                     ImVec2( 0.f, ( key_h + spacing ) * 4.f ), ImVec2( key_w + spacing, ( key_h + spacing ) * 4.f ),
		                                     ImVec2( ( key_w + spacing ) * 2.f, ( key_h + spacing ) * 4.f ),
		                                     ImVec2( ( key_w + spacing ) * 3.f, ( key_h + spacing ) * 4.f ),
		                                     ImVec2( ( key_w + spacing ) * 4.f, ( key_h + spacing ) * 4.f ) };
		for ( std::size_t i = 0; i < keys.size( ) && i < pos.size( ); ++i ) {
			const ImVec2 bmin( x + pos[ i ].x, y + pos[ i ].y );
			const ImVec2 bmax( bmin.x + key_w, bmin.y + key_h );
			const float press = keys[ i ].alpha;
			draw->AddRectFilled( bmin, bmax, ImColor( 10, 10, 10, static_cast< int >( 180.f * bg_alpha + press * 35.f ) ), 4.f );
			draw->AddRect( bmin, bmax, ImColor( key_color_cfg[ 0 ], key_color_cfg[ 1 ], key_color_cfg[ 2 ], static_cast< int >( 55 + press * 170.f ) ),
			               4.f );
			const ImVec2 ts = fonts_for_gui::regular_11->CalcTextSizeA( 11.f * scale, FLT_MAX, 0.f, keys[ i ].label );
			draw->AddText( fonts_for_gui::regular_11, 11.f * scale,
			               ImVec2( bmin.x + ( key_w - ts.x ) * 0.5f, bmin.y + ( key_h - ts.y ) * 0.5f ),
			               ImColor( 230, 230, 230, static_cast< int >( 180 + press * 70.f ) ), keys[ i ].label );
		}
	}

	if ( GET_VARIABLE( g_variables.m_jump_stats_enable, bool ) && GET_VARIABLE( g_variables.m_jump_stats_panel, bool ) ) {
		const ImVec2 pos( static_cast< float >( GET_VARIABLE( g_variables.m_jump_stats_x, int ) ),
		                  static_cast< float >( GET_VARIABLE( g_variables.m_jump_stats_y, int ) ) );
		const ImVec2 size( 280.f, 62.f );
		const ImVec2 max( pos.x + size.x, pos.y + size.y );
		const auto accent_jump = GET_VARIABLE( g_variables.m_accent, c_color );
		draw->AddRectFilled( pos, max, ImColor( 8, 8, 10, 190 ), 6.f );
		draw->AddRect( pos, max, ImColor( accent_jump[ 0 ], accent_jump[ 1 ], accent_jump[ 2 ], 110 ), 6.f );
		draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( pos.x + 8.f, pos.y + 7.f ), ImColor( 205, 205, 205, 235 ), "jump stats" );
		draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( pos.x + 8.f, pos.y + 26.f ), ImColor( 230, 230, 230, 230 ), last_jump_stats_line.c_str( ) );
		draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( pos.x + 8.f, pos.y + 44.f ),
		               ImColor( 160, 160, 160, 220 ),
		               styled_metric_text( "direction", last_bhop_direction, GET_VARIABLE( g_variables.m_jump_stats_label_style, int ) ).c_str( ) );
	}

	if ( GET_VARIABLE( g_variables.m_speedometer, bool ) ) {
		peak_speed = std::max( peak_speed, speed );
		if ( grounded && speed < 40.f )
			peak_speed = std::max( 0.f, peak_speed - frame_time * 80.f );
		const int units = std::clamp( GET_VARIABLE( g_variables.m_speedometer_units, int ), 0, 2 );
		const float display_speed = units == 1 ? speed * 0.06858f : units == 2 ? speed * 0.04261f : speed;
		const auto speed_text = std::to_string( static_cast< int >( std::round( display_speed ) ) );
		const char* unit_text = units == 1 ? "km/h" : units == 2 ? "mph" : "u/s";
		ImFont* speed_font = fonts_for_gui::bolder_11;
		ImFont* unit_font = fonts_for_gui::regular_11;
		const int style = std::clamp( GET_VARIABLE( g_variables.m_speedometer_style, int ), 0, 5 );
		const float speed_font_size = style == 0 ? 28.f : 30.f;
		const float unit_font_size = 11.f;
		const auto number_size = speed_font->CalcTextSizeA( speed_font_size, FLT_MAX, 0.f, speed_text.c_str( ) );
		const auto unit_size = unit_font->CalcTextSizeA( unit_font_size, FLT_MAX, 0.f, unit_text );
		const float configured_x = static_cast< float >( GET_VARIABLE( g_variables.m_speedometer_x, int ) );
		const float configured_y = static_cast< float >( GET_VARIABLE( g_variables.m_speedometer_y, int ) );
		const ImVec2 base( configured_x > 0.f ? configured_x : ( width - number_size.x - unit_size.x - 6.f ) * 0.5f,
		                   configured_y > 0.f ? configured_y : height - 178.f );
		const float bar_width = 132.f;
		const float bar_fill = std::clamp( speed / 520.f, 0.f, 1.f );
		const ImColor style_color = style == 2 ? ImColor( 170, 105, 255, 245 ) :
		                            style == 4 ? ImColor( 210, 210, 230, 245 ) :
		                            style == 5 ? ImColor( 255, 135, 205, 245 ) : ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.95f );

		draw->AddText( speed_font, speed_font_size, ImVec2( base.x + 1.f, base.y + 1.f ), ImColor( 0, 0, 0, 175 ), speed_text.c_str( ) );
		draw->AddText( speed_font, speed_font_size, base, ImColor( 245, 245, 245, 245 ), speed_text.c_str( ) );
		draw->AddText( unit_font, unit_font_size, ImVec2( base.x + number_size.x + 6.f, base.y + 13.f ),
		               style_color, unit_text );
		draw->AddRectFilled( ImVec2( base.x, base.y + number_size.y + 5.f ), ImVec2( base.x + bar_width, base.y + number_size.y + 8.f ),
		                      ImColor( 18, 18, 18, 180 ), 2.f );
		draw->AddRectFilled( ImVec2( base.x, base.y + number_size.y + 5.f ), ImVec2( base.x + bar_width * bar_fill, base.y + number_size.y + 8.f ),
		                      style_color, 2.f );
		ImGui::PushFont( fonts_for_gui::regular_11 );
		float detail_y = base.y + number_size.y + 12.f;
		if ( GET_VARIABLE( g_variables.m_speedometer_state, bool ) ) {
			draw->AddText( ImVec2( base.x, detail_y ), ImColor( 185, 185, 185, 230 ), grounded ? "ground" : "air" );
			detail_y += 12.f;
		}
		if ( GET_VARIABLE( g_variables.m_speedometer_peak, bool ) ) {
			const std::string peak_text = "peak " + std::to_string( static_cast< int >( peak_speed ) );
			draw->AddText( ImVec2( base.x, detail_y ), ImColor( 165, 165, 165, 220 ), peak_text.c_str( ) );
			detail_y += 12.f;
		}
		if ( GET_VARIABLE( g_variables.m_speedometer_jump_quality, bool ) && jump_feedback_alpha > 0.02f ) {
			const std::string quality_text = jump_feedback_delta >= 0.f ? "+" + std::to_string( static_cast< int >( jump_feedback_delta ) )
			                                                            : std::to_string( static_cast< int >( jump_feedback_delta ) );
			draw->AddText( ImVec2( base.x, detail_y ), jump_feedback_delta >= 0.f ? ImColor( style_color.Value.x, style_color.Value.y, style_color.Value.z, jump_feedback_alpha )
			                                                                      : ImColor( 255, 80, 90, static_cast< int >( 220.f * jump_feedback_alpha ) ),
			               quality_text.c_str( ) );
		}
		ImGui::PopFont( );
	}

	if ( GET_VARIABLE( g_variables.m_fumo_spin, bool ) ) {
		if ( !tried_fumo_texture && g_interfaces.m_direct_device ) {
			D3DXCreateTextureFromFileA( g_interfaces.m_direct_device, n_branding::k_logo_file, &fumo_texture );
			tried_fumo_texture = true;
		}

		if ( fumo_texture ) {
			const float size = std::clamp( GET_VARIABLE( g_variables.m_fumo_spin_size, float ), 24.f, 180.f );
			const ImVec2 center( static_cast< float >( GET_VARIABLE( g_variables.m_fumo_spin_x, int ) ),
			                     static_cast< float >( GET_VARIABLE( g_variables.m_fumo_spin_y, int ) ) );
			const int direction = std::clamp( GET_VARIABLE( g_variables.m_fumo_spin_direction, int ), 0, 2 );
			const float direction_sign = direction == 1 ? -1.f : 1.f;
			const float wobble = direction == 2 ? std::sin( now * 4.0f ) * 0.35f : 0.f;
			const float intensity = std::clamp( GET_VARIABLE( g_variables.m_fumo_spin_intensity, float ), 0.1f, 1.f );
			const float angle = now * std::clamp( GET_VARIABLE( g_variables.m_fumo_spin_speed, float ), 0.1f, 5.f ) * 3.2f * direction_sign + wobble;
			const float half = size * 0.5f;
			auto rotate = [ & ]( const float x, const float y ) {
				const float cs = std::cos( angle );
				const float sn = std::sin( angle );
				return ImVec2( center.x + x * cs - y * sn, center.y + x * sn + y * cs );
			};

			const auto p0 = rotate( -half, -half );
			const auto p1 = rotate( half, -half );
			const auto p2 = rotate( half, half );
			const auto p3 = rotate( -half, half );
			draw->AddCircleFilled( center, half * 0.74f, ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.10f * intensity ), 48 );
			if ( intensity > 0.55f ) {
				for ( int i = 0; i < 4; ++i ) {
					const float a = angle + i * 1.57f;
					draw->AddCircleFilled( ImVec2( center.x + std::cos( a ) * half * 0.82f, center.y + std::sin( a ) * half * 0.82f ), 2.0f,
					                       ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.20f * intensity ), 12 );
				}
			}
			draw->AddImageQuad( reinterpret_cast< ImTextureID >( fumo_texture ), p0, p1, p2, p3, ImVec2( 0.f, 0.f ), ImVec2( 1.f, 0.f ),
			                    ImVec2( 1.f, 1.f ), ImVec2( 0.f, 1.f ), ImColor( 255, 255, 255, 230 ) );
		}
	}

	if ( GET_VARIABLE( g_variables.m_jump_strafe_feedback, bool ) && jump_feedback_alpha > 0.02f ) {
		const ImVec2 pos( width * 0.5f - 46.f, height * 0.58f );
		const bool gained = jump_feedback_delta >= 0.f;
		const std::string text = gained ? "gain +" + std::to_string( static_cast< int >( jump_feedback_delta ) )
		                                : "loss " + std::to_string( static_cast< int >( jump_feedback_delta ) );
		ImGui::PushFont( fonts_for_gui::bolder_11 );
		draw->AddRectFilled( ImVec2( pos.x - 8.f, pos.y - 5.f ), ImVec2( pos.x + 92.f, pos.y + 18.f ),
		                      ImColor( 9, 9, 10, static_cast< int >( 145.f * jump_feedback_alpha ) ), 5.f );
		draw->AddText( pos, gained ? ImColor( accent.Value.x, accent.Value.y, accent.Value.z, jump_feedback_alpha )
		                            : ImColor( 255, 80, 90, static_cast< int >( 220.f * jump_feedback_alpha ) ),
		               text.c_str( ) );
		ImGui::PopFont( );
	}

	if ( GET_VARIABLE( g_variables.m_aura_debt_counter, bool ) ) {
		const ImVec2 pos( static_cast< float >( GET_VARIABLE( g_variables.m_aura_debt_x, int ) ),
		                  static_cast< float >( GET_VARIABLE( g_variables.m_aura_debt_y, int ) ) );
		const ImVec2 size( 154.f, 42.f );
		draw->AddRectFilled( pos, ImVec2( pos.x + size.x, pos.y + size.y ), ImColor( 9, 9, 11, 185 ), 6.f );
		draw->AddRect( pos, ImVec2( pos.x + size.x, pos.y + size.y ), ImColor( 42, 42, 44, 165 ), 6.f );
		draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( pos.x + 9.f, pos.y + 7.f ), ImColor( 190, 190, 190, 230 ), "aura debt" );
		const std::string debt_text = std::to_string( aura_debt );
		draw->AddText( fonts_for_gui::bolder_11, 16.f, ImVec2( pos.x + 96.f, pos.y + 16.f ), ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.95f ),
		               debt_text.c_str( ) );
		if ( g_input.is_key_released( VK_DELETE ) && g_menu.m_opened )
			aura_debt = 0;
	}

	if ( GET_VARIABLE( g_variables.m_drain_check_widget, bool ) ) {
		const ImVec2 pos( static_cast< float >( GET_VARIABLE( g_variables.m_drain_check_x, int ) ),
		                  static_cast< float >( GET_VARIABLE( g_variables.m_drain_check_y, int ) ) );
		const ImVec2 size( 184.f, 64.f );
		draw->AddRectFilled( pos, ImVec2( pos.x + size.x, pos.y + size.y ), ImColor( 8, 8, 10, 188 ), 6.f );
		draw->AddRect( pos, ImVec2( pos.x + size.x, pos.y + size.y ), ImColor( 42, 42, 44, 160 ), 6.f );
		draw->AddRectFilled( pos, ImVec2( pos.x + size.x * std::clamp( speed / 520.f, 0.f, 1.f ), pos.y + 3.f ),
		                      ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.9f ), 6.f, ImDrawFlags_RoundCornersTop );
		const std::string speed_line = "speed " + std::to_string( static_cast< int >( speed ) );
		const std::string state_line = std::string( "state " ) + ( grounded ? "ground" : "air" );
		const std::string jump_line = "last " + std::to_string( static_cast< int >( jump_feedback_delta ) );
		draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( pos.x + 9.f, pos.y + 10.f ), ImColor( 220, 220, 220, 235 ), speed_line.c_str( ) );
		draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( pos.x + 9.f, pos.y + 28.f ), ImColor( 175, 175, 175, 225 ), state_line.c_str( ) );
		draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( pos.x + 9.f, pos.y + 46.f ),
		               jump_feedback_delta >= 0.f ? ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.9f ) : ImColor( 255, 90, 90, 220 ),
		               jump_line.c_str( ) );
	}

	if ( GET_VARIABLE( g_variables.m_old_edit_vibes_text, bool ) && now < old_edit_until ) {
		const float fade = std::clamp( ( old_edit_until - now ) / 1.55f, 0.f, 1.f );
		const char* text = "\xD0\x9C\xD0\xBC\xD0\xBC\xD0\xBC\xD0\xBC\xD0\xBC\xD0\xBC\xD0\xBC\xD0\xBC\xD0\xB0\xD1\x80\xD0\xBC\xD0\xBE\xD0\xBA";
		const float font_size = 28.f + std::sin( now * 18.f ) * 2.f;
		const ImVec2 size = fonts_for_gui::bolder_11->CalcTextSizeA( font_size, 4096.f, 0.f, text );
		const ImVec2 pos( width * 0.5f - size.x * 0.5f, height * 0.32f - size.y * 0.5f );
		draw->AddText( fonts_for_gui::bolder_11, font_size, ImVec2( pos.x + 2.f, pos.y + 2.f ), ImColor( 0, 0, 0, static_cast< int >( 180.f * fade ) ), text );
		draw->AddText( fonts_for_gui::bolder_11, font_size, pos, ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.95f * fade ), text );
	}

	if ( GET_VARIABLE( g_variables.m_bind_overlay, bool ) ) {
		struct bind_overlay_item_t {
			const char* label;
			bool active;
		};
		static std::array< float, 7 > bind_alpha{ };
		const std::array< bind_overlay_item_t, 7 > binds{ {
			{ "airstuck", GET_VARIABLE( g_variables.m_air_stuck, bool ) &&
				             g_input.check_input( &GET_VARIABLE( g_variables.m_air_stuck_key, key_bind_t ) ) },
			{ "blockbot", GET_VARIABLE( g_variables.m_blockbot, bool ) &&
				             g_input.check_input( &GET_VARIABLE( g_variables.m_blockbot_key, key_bind_t ) ) },
			{ "deagle spin", GET_VARIABLE( g_variables.m_deagle_spinner, bool ) &&
				                    g_input.check_input( &GET_VARIABLE( g_variables.m_deagle_spinner_key, key_bind_t ) ) },
			{ "edgebug", GET_VARIABLE( g_variables.edge_bug, bool ) &&
				              g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) },
			{ "pixelsurf", GET_VARIABLE( g_variables.m_pixel_surf, bool ) &&
				                g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) ) },
			{ "jumpbug", GET_VARIABLE( g_variables.m_jump_bug, bool ) &&
				              g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) },
			{ "fireman", GET_VARIABLE( g_variables.m_fire_man, bool ) &&
				             g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) }
		} };
		const float anim_speed = std::clamp( GET_VARIABLE( g_variables.m_bind_overlay_animation_speed, float ), 0.2f, 4.f );
		std::vector< std::pair< const char*, float > > visible_binds;
		for ( std::size_t i = 0U; i < binds.size( ); ++i ) {
			const float target = binds[ i ].active ? 1.f : 0.f;
			bind_alpha[ i ] = std::lerp( bind_alpha[ i ], target, std::clamp( frame_time * 12.f * anim_speed, 0.f, 1.f ) );
			if ( bind_alpha[ i ] > 0.03f )
				visible_binds.push_back( { binds[ i ].label, bind_alpha[ i ] } );
		}
		if ( !visible_binds.empty( ) ) {
			const auto overlay_color = GET_VARIABLE( g_variables.m_bind_overlay_use_accent, bool ) ? accent_cfg : GET_VARIABLE( g_variables.m_bind_overlay_color, c_color );
			const float font_scale = std::clamp( GET_VARIABLE( g_variables.m_bind_overlay_font_scale, float ), 0.75f, 1.5f );
			const float font_size = 11.f * font_scale;
			float total_width = 18.f;
			for ( const auto& item : visible_binds )
				total_width += fonts_for_gui::regular_11->CalcTextSizeA( font_size, 4096.f, 0.f, item.first ).x + 20.f;
			total_width = std::max( 86.f, total_width );

			const int position = std::clamp( GET_VARIABLE( g_variables.m_bind_overlay_position, int ), 0, 2 );
			float x = position == 1 ? 14.f : position == 2 ? width - total_width - 14.f : width * 0.5f - total_width * 0.5f;
			const float y = height - 24.f;
			const float bg_alpha = std::clamp( GET_VARIABLE( g_variables.m_bind_overlay_background_alpha, float ), 0.f, 1.f );
			const int style = std::clamp( GET_VARIABLE( g_variables.m_bind_overlay_style, int ), 0, 2 );
			const float rounding = style == 0 ? 0.f : style == 1 ? 3.f : 2.f;
			draw->AddRectFilled( ImVec2( x, y - 2.f ), ImVec2( x + total_width, y + 18.f ),
			                      ImColor( 5, 5, 5, static_cast< int >( 205.f * bg_alpha ) ), rounding );
			draw->AddRectFilled( ImVec2( x, y - 2.f ), ImVec2( x + total_width, y ),
			                      ImColor( overlay_color[ 0 ], overlay_color[ 1 ], overlay_color[ 2 ], static_cast< int >( 210.f * bg_alpha ) ),
			                      rounding, ImDrawFlags_RoundCornersTop );
			draw->AddRect( ImVec2( x, y - 2.f ), ImVec2( x + total_width, y + 18.f ),
			               ImColor( 255, 255, 255, static_cast< int >( 45.f * bg_alpha ) ), rounding );
			x += 9.f;
			for ( const auto& item : visible_binds ) {
				const float alpha = item.second;
				const float slide = ( 1.f - alpha ) * 4.f;
				draw->AddText( fonts_for_gui::regular_11, font_size, ImVec2( x, y + 3.f + slide ),
				               ImColor( 232, 232, 232, static_cast< int >( 235.f * alpha ) ), item.first );
				x += fonts_for_gui::regular_11->CalcTextSizeA( font_size, 4096.f, 0.f, item.first ).x + 9.f;
				draw->AddText( fonts_for_gui::regular_11, font_size, ImVec2( x, y + 3.f ),
				               ImColor( overlay_color[ 0 ], overlay_color[ 1 ], overlay_color[ 2 ], static_cast< int >( 150.f * alpha ) ), "|" );
				x += 11.f;
			}
		}
	}

	if ( GET_VARIABLE( g_variables.m_schizo_vision, bool ) && std::fmod( now * 0.77f, 9.f ) < 0.16f ) {
		const float intensity = std::clamp( GET_VARIABLE( g_variables.m_schizo_vision_intensity, float ), 0.05f, 1.f );
		const float slice_y = std::fmod( now * 137.f, height );
		draw->AddRectFilled( ImVec2( 0.f, slice_y ), ImVec2( width, slice_y + 2.f + 9.f * intensity ),
		                      ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.12f * intensity ) );
		const float glitch_x = std::fmod( now * 211.f, width );
		draw->AddRectFilled( ImVec2( glitch_x, 0.f ), ImVec2( glitch_x + 2.f, height ),
		                      ImColor( 1.f, 1.f, 1.f, 0.06f * intensity ) );
	}

	if ( GET_VARIABLE( g_variables.m_npc_roast, bool ) && now - last_roast_time > 7.0f && std::fmod( now * 3.1f, 1.0f ) < 0.03f ) {
		static constexpr const char* roast_texts[] = {
			"movement filed for bankruptcy", "walking through soup", "velocity got evicted", "aura debt detected",
			"bro got outstrafed by furniture"
		};
		for ( int i = 1; i <= 64; ++i ) {
			auto player = g_interfaces.m_client_entity_list->get< c_base_entity >( i );
			if ( !player || player == g_ctx.m_local || !player->is_player( ) || !player->is_alive( ) || player->is_dormant( ) )
				continue;
			c_vector origin = player->get_abs_origin( );
			origin.m_z += 78.f;
			roast_markers.push_back( { origin, now, roast_texts[ static_cast< int >( now ) % IM_ARRAYSIZE( roast_texts ) ] } );
			last_roast_time = now;
			break;
		}
	}
	roast_markers.erase( std::remove_if( roast_markers.begin( ), roast_markers.end( ),
	                                     [ & ]( const roast_marker_t& marker ) { return now - marker.time > 2.4f; } ),
	                     roast_markers.end( ) );
	if ( GET_VARIABLE( g_variables.m_npc_roast, bool ) ) {
		for ( const auto& marker : roast_markers ) {
			c_vector_2d screen{ };
			if ( !g_render.world_to_screen( marker.origin, screen ) )
				continue;
			const float alpha = std::clamp( 1.f - ( now - marker.time ) / 2.4f, 0.f, 1.f );
			draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( screen.m_x + 1.f, screen.m_y + 1.f ), ImColor( 0, 0, 0, static_cast< int >( 165.f * alpha ) ),
			               marker.text );
			draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( screen.m_x, screen.m_y ), ImColor( 225, 225, 225, static_cast< int >( 230.f * alpha ) ),
			               marker.text );
		}
	}

	if ( GET_VARIABLE( g_variables.m_based_radar, bool ) || GET_VARIABLE( g_variables.m_cringe_detector, bool ) ) {
		for ( int i = 1; i <= 64; ++i ) {
			auto player = g_interfaces.m_client_entity_list->get< c_base_entity >( i );
			if ( !player || player == g_ctx.m_local || !player->is_player( ) || !player->is_alive( ) || player->is_dormant( ) )
				continue;
			if ( ( i + static_cast< int >( now * 0.2f ) ) % 9 != 0 )
				continue;
			c_vector origin = player->get_abs_origin( );
			origin.m_z += 88.f;
			c_vector_2d screen{ };
			if ( !g_render.world_to_screen( origin, screen ) )
				continue;
			const char* label = GET_VARIABLE( g_variables.m_cringe_detector, bool ) && speed < 90.f ? "cringe?" : "based";
			draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( screen.m_x, screen.m_y - 13.f ),
			               ImColor( accent.Value.x, accent.Value.y, accent.Value.z, 0.74f ), label );
		}
	}
}

void n_misc::impl_t::deagle_spinner( )
{
	static bool toggle_active = false;
	static bool was_running = false;
	static float inspect_request_time = 0.f;
	static int inspect_sequence = -1;
	static unsigned int spinner_weapon_handle = 0U;

	if ( !g_ctx.m_cmd || !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !g_interfaces.m_engine_client || !g_interfaces.m_global_vars_base ) {
		toggle_active = false;
		return;
	}

	const auto key = GET_VARIABLE( g_variables.m_deagle_spinner_key, key_bind_t );
	if ( key.m_key > 0 && g_input.is_key_released( key.m_key ) && !g_menu.m_opened && GET_VARIABLE( g_variables.m_deagle_spinner_mode, int ) == 1 )
		toggle_active = !toggle_active;

	const bool hold_active = key.m_key > 0 && g_input.is_key_down( key.m_key ) && !g_menu.m_opened;
	const bool active = GET_VARIABLE( g_variables.m_deagle_spinner_mode, int ) == 1 ? toggle_active : hold_active;
	if ( !GET_VARIABLE( g_variables.m_deagle_spinner, bool ) || !active ) {
		if ( was_running ) {
			if ( auto view_model = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_view_model_handle( ) ) )
				view_model->get_playback_rate( ) = 1.f;
		}
		was_running = false;
		inspect_sequence = -1;
		spinner_weapon_handle = 0U;
		return;
	}

	const auto weapon_handle = g_ctx.m_local->get_active_weapon_handle( );
	auto weapon = g_interfaces.m_client_entity_list->get< c_base_entity >( weapon_handle );
	if ( !weapon || weapon->get_item_definition_index( ) != weapon_deagle || weapon->is_reloading( ) ||
	     ( g_ctx.m_cmd->m_buttons & ( in_attack | in_reload ) ) ) {
		if ( was_running ) {
			if ( auto view_model = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_view_model_handle( ) ) )
				view_model->get_playback_rate( ) = 1.f;
		}
		was_running = false;
		inspect_sequence = -1;
		spinner_weapon_handle = 0U;
		return;
	}

	const float now = g_interfaces.m_global_vars_base->m_real_time;
	auto view_model = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_view_model_handle( ) );
	if ( !view_model ) {
		was_running = false;
		inspect_sequence = -1;
		return;
	}

	if ( !was_running || spinner_weapon_handle != weapon_handle ) {
		g_interfaces.m_engine_client->client_cmd_unrestricted( "+lookatweapon", true );
		g_interfaces.m_engine_client->client_cmd_unrestricted( "-lookatweapon", true );
		inspect_request_time = now;
		inspect_sequence = -1;
		spinner_weapon_handle = weapon_handle;
		was_running = true;
		return;
	}

	const int current_sequence = view_model->get_sequence( );
	if ( inspect_sequence < 0 && now - inspect_request_time > 0.08f && current_sequence > 0 )
		inspect_sequence = current_sequence;

	if ( inspect_sequence >= 0 ) {
		if ( current_sequence != inspect_sequence )
			view_model->set_sequence( inspect_sequence );

		const float speed = std::clamp( GET_VARIABLE( g_variables.m_deagle_spinner_speed, float ), 0.35f, 2.5f );
		constexpr float loop_start = 0.28f;
		constexpr float loop_end = 0.76f;
		auto& cycle = view_model->get_cycle( );
		if ( cycle < loop_start || cycle >= loop_end )
			cycle = loop_start;
		view_model->get_playback_rate( ) = speed;
	}
}

void n_misc::impl_t::blockbot( )
{
	if ( !GET_VARIABLE( g_variables.m_blockbot, bool ) || !g_ctx.m_cmd || !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ||
	     !g_interfaces.m_engine_client || !g_interfaces.m_client_entity_list )
		return;

	const auto key = GET_VARIABLE( g_variables.m_blockbot_key, key_bind_t );
	if ( key.m_key <= 0 || !g_input.is_key_down( key.m_key ) || g_menu.m_opened )
		return;

	c_angle view_angles{ };
	g_interfaces.m_engine_client->get_view_angles( view_angles );

	c_base_entity* best_target = nullptr;
	float best_score = std::numeric_limits< float >::max( );
	const int target_mode = std::clamp( GET_VARIABLE( g_variables.m_blockbot_target_mode, int ), 0, 1 );
	const int team_filter = std::clamp( GET_VARIABLE( g_variables.m_blockbot_team_filter, int ), 0, 2 );
	const int local_team = g_ctx.m_local->get_team( );
	const int max_clients = std::clamp( g_interfaces.m_engine_client->get_max_clients( ), 1, 64 );

	for ( int i = 1; i <= max_clients; ++i ) {
		auto player = g_interfaces.m_client_entity_list->get< c_base_entity >( i );
		if ( !player || player == g_ctx.m_local || !player->is_player( ) || !player->is_alive( ) || player->is_dormant( ) )
			continue;

		const bool teammate = player->get_team( ) == local_team;
		if ( team_filter == 1 && teammate )
			continue;
		if ( team_filter == 2 && !teammate )
			continue;

		const float distance = g_ctx.m_local->get_abs_origin( ).dist_to_2d( player->get_abs_origin( ) );
		if ( distance > ( target_mode == 1 ? 520.f : 360.f ) )
			continue;

		float score = distance;
		if ( target_mode == 1 ) {
			const c_angle angle = g_math.calculate_angle( g_ctx.m_local->get_eye_position( false ), player->get_eye_position( false ) );
			score = g_math.calculate_fov( view_angles, angle ) * 24.f + score * 0.035f;
		}

		if ( score < best_score ) {
			best_score = score;
			best_target = player;
		}
	}

	if ( !best_target )
		return;

	c_vector target_velocity = best_target->get_velocity( );
	target_velocity.m_z = 0.f;
	c_vector desired_point = best_target->get_abs_origin( );
	if ( target_velocity.length_2d( ) > 12.f ) {
		target_velocity.normalize_in_place( );
		desired_point += target_velocity * 22.f;
	}

	const c_vector to_target = desired_point - g_ctx.m_local->get_abs_origin( );
	if ( to_target.length_2d( ) < 1.f )
		return;

	const float target_yaw = rad2deg( std::atan2f( to_target.m_y, to_target.m_x ) );
	const float yaw_delta = deg2rad( g_math.normalize_angle( target_yaw - view_angles.m_y ) );
	const float move_strength = 450.f * std::clamp( GET_VARIABLE( g_variables.m_blockbot_strength, float ), 0.1f, 1.0f );
	const float distance_scale = std::clamp( to_target.length_2d( ) / 72.f, 0.35f, 1.f );
	const float blend = std::clamp( 0.18f + ( 1.f - GET_VARIABLE( g_variables.m_blockbot_smoothing, float ) ) * 0.72f, 0.18f, 0.9f );
	const float desired_forward = std::cos( yaw_delta ) * move_strength * distance_scale;
	const float desired_side = std::sin( yaw_delta ) * move_strength * distance_scale;

	g_ctx.m_cmd->m_forward_move = std::lerp( g_ctx.m_cmd->m_forward_move, desired_forward, blend );
	g_ctx.m_cmd->m_side_move = std::lerp( g_ctx.m_cmd->m_side_move, desired_side, blend );
}

void n_misc::impl_t::apply_bloom_settings( )
{
	struct bloom_convars_t {
		c_cconvar* postprocess = nullptr;
		c_cconvar* scale = nullptr;
		c_cconvar* exposure_min = nullptr;
		c_cconvar* exposure_max = nullptr;
		float scale_default = 1.f;
		float exposure_min_default = 1.f;
		float exposure_max_default = 1.f;
		int postprocess_default = 1;
		bool captured = false;
		bool applied = false;
	};
	static bloom_convars_t bloom{ };

	if ( !g_interfaces.m_convar )
		return;

	if ( !bloom.captured ) {
		bloom.postprocess = g_interfaces.m_convar->find_var( "mat_postprocess_enable" );
		bloom.scale = g_interfaces.m_convar->find_var( "mat_bloom_scalefactor_scalar" );
		bloom.exposure_min = g_interfaces.m_convar->find_var( "mat_autoexposure_min" );
		bloom.exposure_max = g_interfaces.m_convar->find_var( "mat_autoexposure_max" );
		if ( bloom.postprocess ) bloom.postprocess_default = bloom.postprocess->get_int( );
		if ( bloom.scale ) bloom.scale_default = bloom.scale->get_float( );
		if ( bloom.exposure_min ) bloom.exposure_min_default = bloom.exposure_min->get_float( );
		if ( bloom.exposure_max ) bloom.exposure_max_default = bloom.exposure_max->get_float( );
		bloom.captured = true;
	}

	if ( !GET_VARIABLE( g_variables.m_bloom, bool ) ) {
		if ( bloom.applied ) {
			if ( bloom.postprocess ) bloom.postprocess->set_value( bloom.postprocess_default );
			if ( bloom.scale ) bloom.scale->set_value( bloom.scale_default );
			if ( bloom.exposure_min ) bloom.exposure_min->set_value( bloom.exposure_min_default );
			if ( bloom.exposure_max ) bloom.exposure_max->set_value( bloom.exposure_max_default );
			bloom.applied = false;
		}
		return;
	}

	if ( bloom.postprocess ) bloom.postprocess->set_value( 1 );
	if ( bloom.scale ) bloom.scale->set_value( std::clamp( GET_VARIABLE( g_variables.m_bloom_intensity, float ), 0.f, 6.f ) );
	if ( bloom.exposure_min ) bloom.exposure_min->set_value( std::clamp( GET_VARIABLE( g_variables.m_bloom_exposure, float ), 0.05f, 3.f ) );
	if ( bloom.exposure_max ) bloom.exposure_max->set_value( std::clamp( GET_VARIABLE( g_variables.m_bloom_exposure, float ), 0.05f, 3.f ) );
	bloom.applied = true;
}

void n_misc::impl_t::apply_ambient_light_settings( )
{
	struct ambient_convars_t {
		c_cconvar* red = nullptr;
		c_cconvar* green = nullptr;
		c_cconvar* blue = nullptr;
		float red_default = 0.f;
		float green_default = 0.f;
		float blue_default = 0.f;
		bool captured = false;
		bool applied = false;
		bool missing_logged = false;
	};
	static ambient_convars_t ambient{ };

	if ( !g_interfaces.m_convar )
		return;

	if ( !ambient.captured ) {
		ambient.red = g_interfaces.m_convar->find_var( "mat_ambient_light_r" );
		ambient.green = g_interfaces.m_convar->find_var( "mat_ambient_light_g" );
		ambient.blue = g_interfaces.m_convar->find_var( "mat_ambient_light_b" );
		if ( ambient.red ) ambient.red_default = ambient.red->get_float( );
		if ( ambient.green ) ambient.green_default = ambient.green->get_float( );
		if ( ambient.blue ) ambient.blue_default = ambient.blue->get_float( );
		ambient.captured = true;
	}

	if ( !ambient.red || !ambient.green || !ambient.blue ) {
		if ( !ambient.missing_logged ) {
			debug_log( "ambient", "failed reason=missing mat_ambient_light_* cvars", GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.f,
			           "ambient_missing" );
			ambient.missing_logged = true;
		}
		GET_VARIABLE( g_variables.m_engine_ambient_light, bool ) = false;
		return;
	}

	const bool enabled = GET_VARIABLE( g_variables.m_engine_ambient_light, bool );
	if ( !enabled || g_ambient_restore_requested ) {
		if ( ambient.applied || g_ambient_restore_requested ) {
			ambient.red->set_value( ambient.red_default );
			ambient.green->set_value( ambient.green_default );
			ambient.blue->set_value( ambient.blue_default );
			debug_log( "ambient", "restored defaults", GET_VARIABLE( g_variables.m_debug_log_particles, bool ), 0.25f, "ambient_restore" );
		}
		ambient.applied = false;
		g_ambient_restore_requested = false;
		return;
	}

	const auto color = GET_VARIABLE( g_variables.m_engine_ambient_light_color, c_color );
	const float intensity = std::clamp( GET_VARIABLE( g_variables.m_engine_ambient_light_intensity, float ), 0.f, 4.f );
	ambient.red->set_value( color.base< e_color_type::color_type_r >( ) * intensity );
	ambient.green->set_value( color.base< e_color_type::color_type_g >( ) * intensity );
	ambient.blue->set_value( color.base< e_color_type::color_type_b >( ) * intensity );
	ambient.applied = true;
}


void n_misc::impl_t::on_end_scene( )
{
	if ( GET_VARIABLE( g_variables.trackdispay, bool ) )
		RenderMediaPlayer( );
	if (GET_VARIABLE(g_variables.m_watermark_mode, int) > 0)
		RenderWatermark();

	// functions that require player to be ingame
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || g_ctx.m_local->get_observer_mode( ) == 1 /*DEATH_CAM*/ )
		return;

	this->draw_visual_cosmetics( );

	// practice window
	[ & ]( const bool can_draw_practice_window ) {
		if ( !can_draw_practice_window )
			return;
		if ( !g_ctx.m_local->is_alive( ) )
			return;

		constexpr float window_width = 170.0f;
		constexpr float window_rounding = 7.0f;
		constexpr float window_padding = 0.0f;
		constexpr float title_top_margin = 5.0f;
		constexpr float first_bind_top = 25.0f;
		constexpr float bind_left_margin = 10.0f;
		constexpr float key_box_size = 20.0f;
		constexpr float key_box_rounding = 5.0f;
		constexpr float bind_row_height = 24.0f;
		constexpr float bind_row_spacing = 3.0f;
		constexpr float bottom_margin = 5.0f;
		constexpr ImU32 bg_color = IM_COL32(13,13,13,255);
		constexpr ImU32 key_box_border = IM_COL32(22,22,22,255);
		constexpr ImU32 key_text_color = IM_COL32(100,100,100,255);
		constexpr ImU32 label_color = IM_COL32(100,100,100,255);
		constexpr ImU32 title_color = IM_COL32(255,255,255,255);

		ImFont* key_font = fonts_for_gui::regular_11;
		float key_font_size = 10.f;
		float vertical_offset = -25.0f; // поднимаем бокс и текст чуть выше

		float window_height = first_bind_top + 2 * bind_row_height + bind_row_spacing + bottom_margin;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, window_rounding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(window_padding, window_padding));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);

		ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);
		int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
		ImGui::Begin("Checkpoints", nullptr, flags);
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 win_pos = ImGui::GetWindowPos();

			// Заголовок по центру
			ImGui::PushFont(fonts_for_gui::regular_11);
			const char* title = "Checkpoints";
			ImVec2 title_size = ImGui::CalcTextSize(title);
			ImGui::SetCursorPosY(title_top_margin);
			ImGui::SetCursorPosX((window_width - title_size.x) * 0.5f);
			ImGui::TextColored(ImColor(title_color), title);
			ImGui::PopFont();

			// Массив подписей и биндов (только Save и Teleport)
			struct bind_row_t { const char* label; const char* key; };
			bind_row_t rows[2] = {
				{"Save", FILTERED_KEY_NAMES[GET_VARIABLE(g_variables.m_practice_cp_key, key_bind_t).m_key]},
				{"Teleport", FILTERED_KEY_NAMES[GET_VARIABLE(g_variables.m_practice_tp_key, key_bind_t).m_key]}
			};

			float y = first_bind_top;
			for (int i = 0; i < 2; ++i) {
				// Label
				ImGui::SetCursorPosY(y);
				ImGui::SetCursorPosX(bind_left_margin);
				ImGui::PushFont(fonts_for_gui::regular_11);
				ImGui::TextColored(ImColor(label_color), rows[i].label);
				ImGui::PopFont();

				// Key box — строго напротив текста, но чуть выше
				ImVec2 label_pos = ImGui::GetCursorScreenPos(); // Сохраняем ДО вывода текста!
				ImVec2 key_box_pos = ImVec2(win_pos.x + window_width - bind_left_margin - key_box_size, label_pos.y + vertical_offset);
				draw_list->AddRect(
					key_box_pos,
					ImVec2(key_box_pos.x + key_box_size, key_box_pos.y + key_box_size),
					key_box_border,
					3.0f, // радиус скругления
					ImDrawFlags_RoundCornersAll, // скруглять все углы
					2.0f
				);

				// Key text — по центру бокса
				std::string key_text = rows[i].key;
				std::transform(key_text.begin(), key_text.end(), key_text.begin(), ::toupper);
				ImVec2 key_text_size = key_font->CalcTextSizeA(key_font_size, FLT_MAX, 0.0f, key_text.c_str());
				ImVec2 key_text_pos = ImVec2(
					key_box_pos.x + (key_box_size - key_text_size.x) * 0.5f,
					key_box_pos.y + (key_box_size - key_text_size.y) * 0.2f + 2.5f // <-- вот это!
				);
				draw_list->AddText(key_font, key_font_size, key_text_pos, key_text_color, key_text.c_str());

				y += bind_row_height + bind_row_spacing;
			}
		}
		ImGui::End();
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(3);
	}( GET_VARIABLE( g_variables.m_practice_window, bool ) );

	[ & ]( bool can_draw_spectator_list, int type ) {
		if ( type == 0 && can_draw_spectator_list )
			draw_spectator_list( );
	}( GET_VARIABLE( g_variables.m_spectators_list, bool ), GET_VARIABLE( g_variables.m_spectators_list_type, int ) );


	

}

void n_misc::impl_t::on_fire_event( ) {


}

void n_misc::impl_t::practice_window_think( )
{
	const bool practice_window = GET_VARIABLE( g_variables.m_practice_window, bool );
	const bool practice_mode = GET_VARIABLE( g_variables.m_practice_mode, bool );
	if ( !practice_window && !practice_mode )
		return;

	if ( !g_interfaces.m_engine_client || g_interfaces.m_engine_client->is_console_visible( ) )
		return;

	if ( practice_mode && GET_VARIABLE( g_variables.m_practice_apply_requested, bool ) ) {
		GET_VARIABLE( g_variables.m_practice_apply_requested, bool ) = false;
		std::vector< std::string > commands;
		if ( GET_VARIABLE( g_variables.m_practice_basic_setup, bool ) ) {
			commands.emplace_back( "sv_cheats 1" );
			commands.emplace_back( "sv_infinite_ammo 1" );
			commands.emplace_back( "mp_ignore_round_win_conditions 1" );
			commands.emplace_back( "mp_roundtime_defuse 60" );
			commands.emplace_back( "mp_roundtime 60" );
			commands.emplace_back( "mp_freezetime 0" );
			commands.emplace_back( "mp_buy_anywhere 1" );
			commands.emplace_back( "mp_buytime 9999" );
			commands.emplace_back( "ammo_grenade_limit_total 5" );
			commands.emplace_back( "bot_kick" );
		}
		if ( GET_VARIABLE( g_variables.m_practice_disable_fall_damage, bool ) )
			commands.emplace_back( "sv_falldamage_scale 0" );
		if ( GET_VARIABLE( g_variables.m_practice_ignore_round_win, bool ) )
			commands.emplace_back( "mp_ignore_round_win_conditions 1" );
		if ( GET_VARIABLE( g_variables.m_practice_give_weapons, bool ) )
			commands.emplace_back( "impulse 101" );
		if ( GET_VARIABLE( g_variables.m_practice_restart_after_apply, bool ) )
			commands.emplace_back( "mp_restartgame 1" );
		const auto custom_commands = GET_VARIABLE( g_variables.m_practice_custom_commands, std::string );
		if ( !custom_commands.empty( ) )
			commands.emplace_back( custom_commands );
		for ( const auto& command : commands )
			g_interfaces.m_engine_client->client_cmd_unrestricted( command.c_str( ) );
	}
	if ( practice_mode && GET_VARIABLE( g_variables.m_practice_reset_requested, bool ) ) {
		GET_VARIABLE( g_variables.m_practice_reset_requested, bool ) = false;
		g_interfaces.m_engine_client->client_cmd_unrestricted( "sv_falldamage_scale 1; sv_infinite_ammo 0; mp_ignore_round_win_conditions 0; mp_buy_anywhere 0; mp_buytime 45" );
	}

	if ( !practice_window )
		return;

	if ( !g_convars[ HASH_BT( "sv_cheats" ) ]->get_bool( ) )
		return;

	const auto cp_key = GET_VARIABLE( g_variables.m_practice_cp_key, key_bind_t ).m_key;

	const auto tp_key = GET_VARIABLE( g_variables.m_practice_tp_key, key_bind_t ).m_key;

	if ( g_input.is_key_released( cp_key ) ) {
		if ( !( g_ctx.m_local->get_flags( ) & fl_onground ) ) {
			g_logger.print( "you need to be onground to set a checkpoint.", "[practice]" );
			return;
		}

		c_angle saved_angles = { };
		g_interfaces.m_engine_client->get_view_angles( saved_angles );

		g_misc.practice.saved_angles   = saved_angles;
		g_misc.practice.saved_position = g_ctx.m_local->get_abs_origin( );

		g_logger.print( "saved checkpoint.", "[practice]" );
	} else if ( g_input.is_key_released( tp_key ) ) {
		if ( g_misc.practice.saved_angles.is_zero( ) || g_misc.practice.saved_position.is_zero( ) )
			return;

		g_interfaces.m_engine_client->client_cmd_unrestricted(
			std::string( "setpos " )
				.append( std::vformat( "{} {} {}", std::make_format_args( g_misc.practice.saved_position.m_x, g_misc.practice.saved_position.m_y,
		                                                                  g_misc.practice.saved_position.m_z ) ) )
				.append( ";setang " )
				.append( std::vformat( "{} {} {}", std::make_format_args( g_misc.practice.saved_angles.m_x, g_misc.practice.saved_angles.m_y,
		                                                                  g_misc.practice.saved_angles.m_z ) ) )
				.c_str( ) );
	}
}

void n_misc::impl_t::disable_post_processing( )
{
	g_convars[ HASH_BT( "mat_postprocess_enable" ) ]->set_value( !( GET_VARIABLE( g_variables.m_disable_post_processing, bool ) ) );
}

void n_misc::impl_t::remove_panorama_blur( )
{
	g_convars[ HASH_BT( "@panorama_disable_blur" ) ]->set_value( GET_VARIABLE( g_variables.m_remove_panorama_blur, bool ) );
}

void n_misc::impl_t::apply_game_visual_convars( )
{
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_convar )
		return;

	auto restore_or_set = []( const char* name, const bool enabled, const auto value, bool& was_enabled ) {
		auto var = g_interfaces.m_convar->find_var( name );
		if ( !var )
			return;

		if ( enabled ) {
			var->set_value( value );
			was_enabled = true;
		} else if ( was_enabled ) {
			if ( var->sm_default_value )
				var->set_value( var->sm_default_value );
			was_enabled = false;
		}
	};

	static bool was_viewmodel_fov = false;
	static bool was_viewmodel_x = false;
	static bool was_viewmodel_y = false;
	static bool was_viewmodel_z = false;
	static bool was_sway = false;
	static bool was_fullbright = false;
	static bool was_force_crosshair = false;
	static bool was_thirdperson = false;
	static bool was_flash_reduced = false;

	restore_or_set( "viewmodel_fov", GET_VARIABLE( g_variables.m_viewmodel_fov, bool ),
	                GET_VARIABLE( g_variables.m_viewmodel_fov_value, int ), was_viewmodel_fov );

	const bool viewmodel_offsets = GET_VARIABLE( g_variables.m_viewmodel_offsets, bool );
	restore_or_set( "viewmodel_offset_x", viewmodel_offsets, GET_VARIABLE( g_variables.m_viewmodel_offset_x, float ), was_viewmodel_x );
	restore_or_set( "viewmodel_offset_y", viewmodel_offsets, GET_VARIABLE( g_variables.m_viewmodel_offset_y, float ), was_viewmodel_y );
	restore_or_set( "viewmodel_offset_z", viewmodel_offsets, GET_VARIABLE( g_variables.m_viewmodel_offset_z, float ), was_viewmodel_z );

	restore_or_set( "cl_wpn_sway_scale", GET_VARIABLE( g_variables.m_weapon_sway_scale, bool ),
	                GET_VARIABLE( g_variables.m_weapon_sway_scale_value, float ), was_sway );

	restore_or_set( "mat_fullbright", GET_VARIABLE( g_variables.m_fullbright, bool ) ||
	                                    GET_VARIABLE( g_variables.m_world_material_changer, int ) == 1,
	                1, was_fullbright );

	restore_or_set( "weapon_debug_spread_show", GET_VARIABLE( g_variables.m_force_crosshair, bool ), 3, was_force_crosshair );

	const bool in_game = g_interfaces.m_engine_client->is_in_game( );
	const bool alive = g_ctx.m_local && g_ctx.m_local->is_alive( );
	const bool wants_thirdperson = in_game && alive && GET_VARIABLE( g_variables.m_thirdperson, bool );
	if ( wants_thirdperson != was_thirdperson ) {
		g_interfaces.m_engine_client->client_cmd_unrestricted( wants_thirdperson ? "thirdperson" : "firstperson" );
		was_thirdperson = wants_thirdperson;
	}

	if ( wants_thirdperson ) {
		if ( auto cam_dist = g_interfaces.m_convar->find_var( "cam_idealdist" ) )
			cam_dist->set_value( GET_VARIABLE( g_variables.m_thirdperson_distance, int ) );
	}

	if ( alive && GET_VARIABLE( g_variables.m_reduce_flash, bool ) ) {
		g_ctx.m_local->get_flash_alpha( ) = std::clamp( GET_VARIABLE( g_variables.m_flash_alpha, float ), 0.f, 255.f );
		was_flash_reduced = true;
	} else if ( alive && was_flash_reduced ) {
		g_ctx.m_local->get_flash_alpha( ) = 255.f;
		was_flash_reduced = false;
	}
}

void n_misc::impl_t::draw_spectating_local( )
{
	int m_y = 5;

	std::vector< spectator_data_t > spectator_data{ };

	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || !GET_VARIABLE( g_variables.m_spectators_list, bool ) ||
	     GET_VARIABLE( g_variables.m_spectators_list_type, int ) != 1 /*list local spectators*/ ) {
		if ( !spectator_data.empty( ) ) {
			g_ctx.m_last_spectators_y = 5;
			spectator_data.clear( );
		}
		return;
	}

	constexpr auto get_player_spec_type = [ & ]( int obs_mode ) -> std::string {
		switch ( obs_mode ) {
		case e_obs_mode::obs_mode_deathcam:
			return "deathcam";
		case e_obs_mode::obs_mode_freezecam:
			return "freezecam";
		case e_obs_mode::obs_mode_in_eye:
			return "first person";
		case e_obs_mode::obs_mode_chase:
			return "3rd person";
		case e_obs_mode::obs_mode_roaming:
			return "roaming";
		case e_obs_mode::obs_mode_fixed:
			return "fixed";
		default:
			return "first person";
		}
	};

	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || entity->is_alive( ) || entity->is_dormant( ) )
			return;

		const auto entity_index = entity->get_index( );
		const auto entity_team  = entity->get_team( );

		const auto spectated_player = reinterpret_cast< c_base_entity* >(
			g_interfaces.m_client_entity_list->get_client_entity_from_handle( entity->get_observer_target_handle( ) ) );

		if ( !spectated_player || spectated_player != g_ctx.m_local )
			return;

		player_info_t spectating_info{ };
		g_interfaces.m_engine_client->get_player_info( entity_index, &spectating_info );

		if ( spectating_info.m_is_hltv )
			return;

		spectator_data.push_back(
			{ std::format( "{} | {}",  std::string( spectating_info.m_name ).substr( 0, 12 ).append( "..." ),
		                                                      get_player_spec_type( entity->get_observer_mode( ) )  ),
		      spectating_info.m_fake_player ? entity_team == e_team_id::team_tt   ? g_render.m_terrorist_avatar
		                                      : entity_team == e_team_id::team_ct ? g_render.m_counter_terrorist_avatar
		                                                                          : nullptr
		                                    : g_avatar_cache[ entity_index ],
		      GET_VARIABLE( g_variables.m_spectators_list_text_color_one, c_color ) } );
	} );

	if ( spectator_data.empty( ) ) {
		g_ctx.m_last_spectators_y = 5;
		return;
	}

	const auto draw_avatar            = GET_VARIABLE( g_variables.m_spectators_avatar, bool );
	constexpr static auto avatar_size = 14.f;

	for ( const auto& data : spectator_data ) {
		if ( draw_avatar )
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_texture,
				std::make_any< texture_draw_object_t >( c_vector_2d( 10, m_y ), c_vector_2d( avatar_size, avatar_size ),
			                                            ImColor( 1.f, 1.f, 1.f, 1.f ), data.m_avatar, 0.f, ImDrawFlags_::ImDrawFlags_None ) );

		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >( g_render.m_fonts[ e_font_names::font_name_tahoma_bd_12 ], c_vector_2d( draw_avatar ? 27 : 10, m_y ),
		                                         data.m_text.c_str( ), data.m_color.get_u32( ),
		                                         ImColor( 0.f, 0.f, 0.f, data.m_color.base< e_color_type::color_type_a >( ) ),
		                                         e_text_flags::text_flag_dropshadow ) );

		m_y += 15;

		g_ctx.m_last_spectators_y = m_y;
	}
}

void n_misc::impl_t::draw_spectator_list()
{
    if (!g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game() || !GET_VARIABLE(g_variables.m_spectators_list, bool))
        return;

    constexpr float window_width = 200.0f;
    constexpr float window_rounding = 5.0f;
    constexpr float window_padding = 0.0f;
    constexpr float title_top_margin = 5.0f;
    constexpr float title_to_list_spacing = 15.0f;
    constexpr float item_left_margin = 10.0f;
    constexpr float item_height = 17.0f;
    constexpr float item_spacing = 10.0f;
    constexpr float bottom_padding = 10.0f;
    constexpr ImU32 bg_color = IM_COL32(13,13,13,255);
    constexpr ImU32 text_color = IM_COL32(255,255,255,255);

    // Собираем спектаторов
    std::vector<std::string> spectators;
    g_entity_cache.enumerate(e_enumeration_type::type_players, [&](c_base_entity* entity) {
        if (!entity || entity->is_alive() || entity->is_dormant())
            return;

        const auto entity_index = entity->get_index();
        const auto spectated_player = reinterpret_cast<c_base_entity*>(
            g_interfaces.m_client_entity_list->get_client_entity_from_handle(entity->get_observer_target_handle()));

        if (!spectated_player || !spectated_player->is_alive() || spectated_player != g_ctx.m_local)
            return;

        player_info_t info{};
        g_interfaces.m_engine_client->get_player_info(entity_index, &info);

        if (info.m_is_hltv)
            return;

        std::string name = std::string(info.m_name).substr(0, 24);
        spectators.push_back(name);
    });

    if (spectators.empty())
        return;

    // Высота окна: паддинг сверху + высота заголовка + отступ до первого ника + (item_height + item_spacing)*(n-1) + item_height + 10 (паддинг до низа)
    float window_height = title_top_margin + ImGui::GetFontSize() + title_to_list_spacing
        + spectators.size() * item_height
        + (spectators.size() > 1 ? (spectators.size() - 1) * item_spacing : 0)
        + bottom_padding;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, window_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(window_padding, window_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);

    ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);
    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::Begin("Spectators", nullptr, flags);
    {
        // Заголовок
        ImGui::PushFont(fonts_for_gui::regular_11);
        const char* title = "Spectators";
        ImVec2 title_size = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosY(title_top_margin);
        ImGui::SetCursorPosX((window_width - title_size.x) * 0.5f);
        ImGui::TextColored(ImColor(text_color), title);
        ImGui::PopFont();

        // Первый ник — строго на 20 пикселей ниже заголовка
        float y = title_top_margin + ImGui::GetFontSize() + title_to_list_spacing;
        for (size_t i = 0; i < spectators.size(); i++) {
            ImGui::SetCursorPosY(y + i * (item_height + item_spacing));
            ImGui::SetCursorPosX(item_left_margin);
            ImGui::PushFont(fonts_for_gui::regular_11);
            ImGui::TextColored(ImColor(text_color), spectators[i].c_str());
            ImGui::PopFont();
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(3);
}
