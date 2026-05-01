#include "../../game/sdk/classes/c_move_data.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/movement/movement_assist_simulation.h"
#include "../hooks.h"

void __fastcall n_detoured_functions::process_movement( void* thisptr, void* edx, c_base_entity* player, c_move_data* move_data )
{
	static auto original = g_hooks.m_process_movement.get_original< decltype( &n_detoured_functions::process_movement ) >( );

	// fix prediction errros when jumping
	// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/game/shared/gamemovement.cpp#L5036
	move_data->m_game_code_moved_player = false;

	if ( g_in_movement_assist_simulation )
		movement_assist_debug_log( "movement", std::string( "process_movement simulation reason=" ) +
		                                           g_movement_assist_simulation_reason +
		                                           " side_effect_state_suppressed=1",
		                           0.25f, "process_movement_sim" );

	original( thisptr, edx, player, move_data );

	if ( g_in_movement_assist_simulation )
		move_data->m_game_code_moved_player = false;
}
