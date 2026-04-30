#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../hooks.h"

namespace
{
	bool is_group( const char* group, const char* needle )
	{
		return group && strstr( group, needle ) != nullptr;
	}

	float channel( const c_color& color, const int index )
	{
		return std::clamp( static_cast< float >( color[ index ] ) / 255.f, 0.f, 1.f );
	}

}

void __fastcall n_detoured_functions::get_color_modulation( void* ecx, void* edx, float* r, float* g, float* b )
{
	static auto original = g_hooks.m_get_color_modulation.get_original< decltype( &n_detoured_functions::get_color_modulation ) >( );
	original( ecx, edx, r, g, b );

	if ( !GET_VARIABLE( g_variables.m_world_modulation, bool ) || !r || !g || !b )
		return;

	const auto material = reinterpret_cast< c_material* >( ecx );
	if ( !material || material->is_error_material( ) || !g_interfaces.m_engine_client->is_in_game( ) )
		return;

	const char* group = material->get_texture_group_name( );
	const bool is_world = is_group( group, TEXTURE_GROUP_WORLD );
	const bool is_prop = is_group( group, "StaticProp textures" );
	const bool is_sky = is_group( group, TEXTURE_GROUP_SKYBOX );

	if ( !is_world && !is_prop && !is_sky )
		return;

	const bool apply_world = is_world && GET_VARIABLE( g_variables.m_world_modulate_world, bool );
	const bool apply_prop = is_prop && GET_VARIABLE( g_variables.m_world_modulate_props, bool );
	const bool apply_sky = is_sky && GET_VARIABLE( g_variables.m_world_modulate_sky, bool );
	if ( !apply_world && !apply_prop && !apply_sky )
		return;

	const c_color color = apply_prop ? GET_VARIABLE( g_variables.m_world_modulation_prop_color, c_color )
	                      : apply_sky ? GET_VARIABLE( g_variables.m_world_modulation_sky_color, c_color )
	                                  : GET_VARIABLE( g_variables.m_world_modulation_world_color, c_color );
	const float intensity = std::clamp( GET_VARIABLE( g_variables.m_world_modulation_intensity, float ), 0.f, 2.f );
	const float brightness = std::clamp( GET_VARIABLE( g_variables.m_world_modulation_brightness, float ), 0.1f, 2.f );
	const float exposure = std::clamp( GET_VARIABLE( g_variables.m_world_modulation_exposure, float ), 0.25f, 2.f );
	const float group_scale = apply_prop ? 0.58f : apply_sky ? 0.92f : 0.34f;
	const float mix = std::clamp( intensity * group_scale, 0.f, 1.45f );

	*r = std::clamp( ( *r * ( 1.f - mix ) + channel( color, 0 ) * mix ) * brightness * exposure, 0.f, 1.f );
	*g = std::clamp( ( *g * ( 1.f - mix ) + channel( color, 1 ) * mix ) * brightness * exposure, 0.f, 1.f );
	*b = std::clamp( ( *b * ( 1.f - mix ) + channel( color, 2 ) * mix ) * brightness * exposure, 0.f, 1.f );

	if ( GET_VARIABLE( g_variables.m_world_material_changer, int ) == 2 ) {
		const float flat = ( *r + *g + *b ) / 3.f;
		*r = std::clamp( flat * 0.92f, 0.f, 1.f );
		*g = std::clamp( flat * 0.92f, 0.f, 1.f );
		*b = std::clamp( flat * 0.92f, 0.f, 1.f );
	}
}
