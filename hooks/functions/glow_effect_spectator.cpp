#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/menu/menu.h"
#include "../hooks.h"

bool __cdecl n_detoured_functions::glow_effect_spectator( c_base_entity* player, c_base_entity* local, e_glow_style& style, c_vector& glow_color,
                                                          float& alpha_start, float& alpha, float& time_start, float& time_target, bool& animate )
{
	g_ctx.m_is_glow_being_drawn = true;
	const auto m_vis_color      = GET_VARIABLE( g_variables.m_glow_vis_color, c_color );
	const auto m_invis_color    = GET_VARIABLE( g_variables.m_glow_invis_color, c_color );

	const bool can_see_player = local->can_see_player( player );
	const bool legacy_glow = GET_VARIABLE( g_variables.m_glow_enable, bool );
	const bool visible_glow = legacy_glow || GET_VARIABLE( g_variables.m_visible_glow, bool ) || GET_VARIABLE( g_variables.m_visible_stencil_glow, bool );
	const bool invisible_glow =
		legacy_glow || GET_VARIABLE( g_variables.m_invisible_glow, bool ) || GET_VARIABLE( g_variables.m_invisible_stencil_glow, bool );

	glow_color =
		( can_see_player
	          ? c_vector( m_vis_color.base< color_type_r >( ), m_vis_color.base< color_type_g >( ), m_vis_color.base< color_type_b >( ) )
	          : c_vector( m_invis_color.base< color_type_r >( ), m_invis_color.base< color_type_g >( ), m_invis_color.base< color_type_b >( ) ) );

	alpha                       = can_see_player ? m_vis_color.base< color_type_a >( ) : m_invis_color.base< color_type_a >( );
	g_ctx.m_is_glow_being_drawn = false;
	return ( can_see_player ? visible_glow : invisible_glow ) && player != local && local->is_enemy( player );
}
