#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/movement/movement_assist_simulation.h"
#include "../hooks.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>
#include <unordered_map>

namespace
{
	constexpr int k_chan_static = 6;

	bool contains_movement_land_sound( const char* sound_entry, const char* sample )
	{
		std::string text;
		if ( sound_entry )
			text += sound_entry;
		text += ' ';
		if ( sample )
			text += sample;

		std::transform( text.begin( ), text.end( ), text.begin( ),
		                []( const unsigned char c ) { return static_cast< char >( std::tolower( c ) ); } );

		return text.find( "player\\land" ) != std::string::npos || text.find( "player/land" ) != std::string::npos;
	}

	std::string normalized_sound_text( const char* sound_entry, const char* sample )
	{
		std::string text;
		if ( sound_entry )
			text += sound_entry;
		text += ' ';
		if ( sample )
			text += sample;

		std::transform( text.begin( ), text.end( ), text.begin( ),
		                []( const unsigned char c ) { return static_cast< char >( std::tolower( c ) ); } );
		return text;
	}

	bool is_engine_movement_sound( const char* sound_entry, const char* sample )
	{
		const std::string text = normalized_sound_text( sound_entry, sample );
		return text.find( "player\\land" ) != std::string::npos || text.find( "player/land" ) != std::string::npos ||
		       text.find( "player\\footsteps" ) != std::string::npos || text.find( "player/footsteps" ) != std::string::npos ||
		       text.find( "suit_" ) != std::string::npos || text.find( "player\\pl_" ) != std::string::npos ||
		       text.find( "player/pl_" ) != std::string::npos;
	}

	bool is_feature_sound( const char* sound_entry, const char* sample )
	{
		const std::string text = normalized_sound_text( sound_entry, sample );
		return text.find( "weapons/revolver/revolver_prepare" ) != std::string::npos ||
		       text.find( "weapons\\revolver\\revolver_prepare" ) != std::string::npos || text.find( "ui\\beep" ) != std::string::npos ||
		       text.find( "ui/beep" ) != std::string::npos || text.find( "drainware" ) != std::string::npos ||
		       text.find( "routecalc" ) != std::string::npos || text.find( "pixelsurf" ) != std::string::npos;
	}

	bool should_suppress_static_land_duplicate( const char* sample )
	{
		static std::unordered_map< std::string, float > last_land_sound_time;
		static float burst_window_start = 0.f;
		static int burst_count = 0;

		const auto now = std::chrono::steady_clock::now( );
		const float now_seconds = std::chrono::duration< float >( now.time_since_epoch( ) ).count( );
		const std::string key = sample && sample[ 0 ] ? sample : "player_land";

		if ( now_seconds - burst_window_start > 0.75f ) {
			burst_window_start = now_seconds;
			burst_count = 0;
		}

		++burst_count;
		const auto it = last_land_sound_time.find( key );
		const bool duplicate = it != last_land_sound_time.end( ) && now_seconds - it->second < 0.12f;
		last_land_sound_time[ key ] = now_seconds;

		if ( burst_count > 5 ) {
			g_drainware_sound_spam_detected = true;
			g_drainware_custom_movement_sounds_muted = true;
			return true;
		}

		return duplicate;
	}
}

void __fastcall n_detoured_functions::emit_sound( void* ecx, void* edx, void* filter, int idx, int channel, const char* sound_entry,
                                                  unsigned int sound_entry_hash, const char* sample, float volume, int seed, float attenuation,
                                                  int flags, int pitch, const c_vector* origin, const c_vector* direction, void* vec_origins,
                                                  bool update_pos, float soundtime, int speakerentity, int unk )
{
	static auto original = g_hooks.m_emit_sound.get_original< decltype( &n_detoured_functions::emit_sound ) >( );

	if ( g_in_movement_assist_simulation ) {
		const bool feature_sound = drainware_current_sound_is_feature( ) || is_feature_sound( sound_entry, sample );
		const bool movement_noise = is_engine_movement_sound( sound_entry, sample );
		const char* source = g_drainware_feature_sound_source && g_drainware_feature_sound_source[ 0 ] ?
		                         g_drainware_feature_sound_source :
		                         g_movement_assist_simulation_reason;
		if ( movement_noise && !feature_sound ) {
			drainware_note_suppressed_sound( "in_assist_simulation", sample );
			if ( GET_VARIABLE( g_variables.m_debug_log_sound, bool ) ) {
				movement_assist_debug_log( "sound", std::string( "suppress reason=simulation_noise source=" ) + source +
				                                      " sample=" + ( sample ? sample : "<null>" ),
				                           0.12f, sample ? sample : "assist_sound" );
			}
			return;
		}

		if ( feature_sound && GET_VARIABLE( g_variables.m_debug_log_sound, bool ) ) {
			movement_assist_debug_log( "sound", std::string( "allow reason=feature_sound source=" ) + source +
			                                      " sample=" + ( sample ? sample : "<null>" ),
			                           0.12f, sample ? sample : "feature_sound" );
		}
	}

	if ( channel == k_chan_static && contains_movement_land_sound( sound_entry, sample ) && should_suppress_static_land_duplicate( sample ) ) {
		drainware_note_suppressed_sound( "static_land_duplicate", sample );
		movement_assist_debug_log( "sound", std::string( "suppressed reason=static_land_duplicate sample=" ) + ( sample ? sample : "<null>" ),
		                           0.25f, "static_land" );
		return;
	}

	return original( ecx, edx, filter, idx, channel, sound_entry, sound_entry_hash, sample, volume, seed, attenuation, flags, pitch, origin,
	                 direction, vec_origins, update_pos, soundtime, speakerentity, unk );
}
