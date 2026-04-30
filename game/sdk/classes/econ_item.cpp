#define _X86_ // Define the target architecture
#include "econ_item.h"
#include "../../../utilities/utilities.h"
#include "i_player_inventory.h"

C_EconItem* C_EconItem::CreateEconItem( )
{
	auto CreateEconitemFn = reinterpret_cast< C_EconItem*( __stdcall* )( ) >(
		*reinterpret_cast< uintptr_t* >( fuckin::m_AddressList[ CheatAddressList::CreateEconItem ] ) );
	return CreateEconitemFn( );
}

void C_EconItem::SetAttributeValue( uint32_t iIndex, void* pValue )
{
	if ( const auto attribute = C_ItemSystem::GetItemSystem( )->GetItemSchemaInterface( )->GetAttributeDefinitionInterface( iIndex ) )
		reinterpret_cast< void( __thiscall* )( C_EconItem*, uint32_t, void* ) >( fuckin::m_AddressList[ CheatAddressList::SetAttributeValue ] )(
			this, attribute, pValue );
}

void C_EconItem::SetCustomName( const char* name )
{
	static auto Address = fuckin::m_AddressList[ CheatAddressList::CustomTagEconItem ];

	auto relativeOffset = *reinterpret_cast< int32_t* >( reinterpret_cast< uintptr_t >( Address ) + 1 ); // Читаем относительное смещение

	auto absoluteAddress = reinterpret_cast< uintptr_t >( Address ) + 5 + relativeOffset; // Вычисляем абсолютный адрес

	using FnSetCustomName       = C_EconItem*( __thiscall* )( void*, const char* );
	static auto fnSetCustomName = reinterpret_cast< FnSetCustomName >( absoluteAddress );

	fnSetCustomName( this, name );
}

void C_EconItem::SetCustomDesc( const char* name )
{
	static auto Address = fuckin::m_AddressList[ CheatAddressList::CustomDescEconItem ];

	auto relativeOffset = *reinterpret_cast< int32_t* >( reinterpret_cast< uintptr_t >( Address ) + 1 ); // Читаем относительное смещение

	auto absoluteAddress = reinterpret_cast< uintptr_t >( Address ) + 5 + relativeOffset; // Вычисляем абсолютный адрес

	using FnSetCustomName       = C_EconItem*( __thiscall* )( void*, const char* );
	static auto fnSetCustomName = reinterpret_cast< FnSetCustomName >( absoluteAddress );

	fnSetCustomName( this, name );
}

std::vector< econ_attirbute > C_EconItem::GetAttributes( )
{
	std::vector< econ_attirbute > aAttributes;

	auto v16 = *( WORD** )( std::uintptr_t( this ) + 0x18 );
	if ( v16 ) {
		auto v17 = v16 + 1;
		auto v48 = ( uint32_t )&v16[ 4 * ( ( uint32_t )*v16 >> 10 ) + 1 ];
		if ( ( unsigned int )( v16 + 1 ) < v48 ) {
			do {
				auto v18 = ( DWORD* )C_ItemSystem::GetItemSystem( );
				auto v19 = *v17;
				if ( v19 < v18[ 75 ] ) {
					auto v20 = *( DWORD* )( v18[ 72 ] + 4 * v19 );
					if ( v20 ) {
						auto attrib  = econ_attirbute( );
						attrib.m_nID = v19;

						auto pAttribute = &attrib;

						*( DWORD* )( ( uintptr_t )pAttribute + 20 ) = 15;
						*( DWORD* )( ( uintptr_t )pAttribute + 16 ) = 0;
						*( uint8_t* )( uintptr_t )pAttribute        = 0;

						auto v45 = *( DWORD* )( v20 + 0xC );

						( *( void( __thiscall** )( DWORD, int32_t, DWORD ) )( *( DWORD* )v45 + 12 ) )( v45, ( int32_t )( v17 + 2 ),
						                                                                               ( uintptr_t )pAttribute );

						aAttributes.push_back( attrib );
					}
				}

				v17 += 4;
			} while ( ( uint32_t )v17 < v48 );
		}
	}

	return aAttributes;
}
