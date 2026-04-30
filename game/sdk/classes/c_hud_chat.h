#pragma once

#include <cstdarg>
#include <cstdio>

class c_hud_chat
{
public:
	void chat_printf( const char* fmt, ... )
	{
		char message[ 1024 ]{ };
		va_list args;
		va_start( args, fmt );
		vsprintf_s( message, sizeof( message ), fmt, args );
		va_end( args );

		using fn_t = void( __cdecl* )( void*, int, int, const char*, ... );
		( *reinterpret_cast< fn_t** >( this ) )[ 27 ]( this, 0, 0, message );
	}
};
