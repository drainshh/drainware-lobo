#pragma once
#include "../../../utilities/memory/virtual.h"

class c_surface
{
public:
	void play_sound( const char* sound_path )
	{
		using original_fn = void( __thiscall* )( c_surface*, const char* );
		return ( *( original_fn** )this )[ 82 ]( this, sound_path );
	}
	void unlock_cursor() {
		g_virtual.call< void >( this, 66 );
	}
};