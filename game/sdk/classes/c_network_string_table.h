#pragma once

#include "../../../utilities/memory/virtual.h"

class c_network_string_table
{
public:
	int get_num_strings( )
	{
		return g_virtual.call< int >( this, 3 );
	}

	int add_string( const bool is_server, const char* value, const int length = -1, const void* user_data = nullptr )
	{
		return g_virtual.call< int >( this, 8, is_server, value, length, user_data );
	}

	const char* get_string( const int string_number )
	{
		return g_virtual.call< const char* >( this, 9, string_number );
	}

	int find_string_index( const char* value )
	{
		return g_virtual.call< int >( this, 12, value );
	}
};

class c_network_string_table_container
{
public:
	c_network_string_table* find_table( const char* table_name )
	{
		return g_virtual.call< c_network_string_table* >( this, 3, table_name );
	}
};
