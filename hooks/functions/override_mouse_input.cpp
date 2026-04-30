#include "../../game/sdk/classes/c_engine_client.h"
#include "../../game/sdk/classes/c_global_vars_base.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/movement/movement.h"
#include "../hooks.h"
#include "../../hacks/prediction/prediction.h"
#include "../../hacks/movement/edgebug.h"
extern bool HITGODA;
extern bool float_ground;
void __fastcall n_detoured_functions::override_mouse_input( void* thisptr, int edx, float* x, float* y )
{
	static auto original = g_hooks.m_override_mouse_input.get_original< decltype( &n_detoured_functions::override_mouse_input ) >( );

	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return original( thisptr, edx, x, y );

	if ( HITGODA && !float_ground )
	{
		if ( g_prediction.backup_data.m_velocity.m_z < 0.f ) {
			if ( *x != 0.f ) {
				*x *= GET_VARIABLE( g_variables.m_pixel_surf_assist_lockfactor, float );
			}
		}
	}
	if ( !GET_VARIABLE( g_variables.SiletEdgeBug,bool ) )
		g_edgebug.edgebug_lock( *x, *y );
	original( thisptr, edx, x, y );
}
