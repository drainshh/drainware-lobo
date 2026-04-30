#include "utilities.h"

#include "../game/sdk/includes/includes.h"
#include "../globals/includes/includes.h"

/* used ~ std::thread (*/
#include <thread>
#include <Psapi.h>

int n_utilities::impl_t::create_thread( unsigned long __stdcall function( void* ), void* parameter )
{
	std::thread( function, parameter ).detach( );

	return 1;
}


int n_utilities::impl_t::get_average_fps( ImGuiIO io )
{
	static int average_fps = static_cast< int >( io.Framerate + 0.5f );

	if ( static float last_checked = g_interfaces.m_global_vars_base->m_real_time;
	     fabs( last_checked - g_interfaces.m_global_vars_base->m_real_time ) > 2.0f ) {
		average_fps  = static_cast< int >( io.Framerate + 0.5f );
		last_checked = g_interfaces.m_global_vars_base->m_real_time;
	}
	return average_fps;
}

const char8_t* n_utilities::impl_t::get_weapon_icon( short item_definition_index )
{
	if ( item_definition_index == weapon_none )
		return u8"";

	switch ( item_definition_index ) {
	case weapon_deagle:
		return u8"\uE001";
	case weapon_elite:
		return u8"\uE002";
	case weapon_fiveseven:
		return u8"\uE003";
	case weapon_glock:
		return u8"\uE004";
	case weapon_ak47:
		return u8"\uE007";
	case weapon_aug:
		return u8"\uE008";
	case weapon_awp:
		return u8"\uE009";
	case weapon_famas:
		return u8"\uE00A";
	case weapon_g3sg1:
		return u8"\uE00B";
	case weapon_galilar:
		return u8"\uE00D";
	case weapon_m249:
		return u8"\uE00E";
	case weapon_m4a1:
		return u8"\uE010";
	case weapon_mac10:
		return u8"\uE011";
	case weapon_p90:
		return u8"\uE013";
	case weapon_mp5sd:
		return u8"\uE017";
	case weapon_ump45:
		return u8"\uE018";
	case weapon_xm1014:
		return u8"\uE019";
	case weapon_bizon:
		return u8"\uE01A";
	case weapon_mag7:
		return u8"\uE01B";
	case weapon_negev:
		return u8"\uE01C";
	case weapon_sawedoff:
		return u8"\uE01D";
	case weapon_tec9:
		return u8"\uE01E";
	case weapon_taser:
		return u8"\uE01F";
	case weapon_hkp2000:
		return u8"\uE020";
	case weapon_mp7:
		return u8"\uE021";
	case weapon_mp9:
		return u8"\uE022";
	case weapon_nova:
		return u8"\uE023";
	case weapon_p250:
		return u8"\uE024";
	case weapon_scar20:
		return u8"\uE026";
	case weapon_sg556:
		return u8"\uE027";
	case weapon_ssg08:
		return u8"\uE028";
	case weapon_knife:
		return u8"\uE02A";
	case weapon_flashbang:
		return u8"\uE02B";
	case weapon_hegrenade:
		return u8"\uE02C";
	case weapon_smokegrenade:
		return u8"\uE02D";
	case weapon_molotov:
		[[fallthrough]];
	case weapon_firebomb:
		return u8"\uE02E";
	case weapon_decoy:
		[[fallthrough]];
	case weapon_diversion:
		return u8"\uE02F";
	case weapon_incgrenade:
		return u8"\uE030";
	case weapon_c4:
		return u8"\uE031";
	case weapon_healthshot:
		return u8"\uE039";
	case weapon_knife_gg:
		[[fallthrough]];
	case weapon_knife_t:
		return u8"\uE03B";
	case weapon_m4a1_silencer:
		return u8"\uE03C";
	case weapon_usp_silencer:
		return u8"\uE03D";
	case weapon_cz75a:
		return u8"\uE03F";
	case weapon_revolver:
		return u8"\uE040";
	case weapon_tagrenade:
		return u8"\uE044";
	case weapon_fists:
		return u8"\uE045";
	case weapon_tablet:
		return u8"\uE048";
	case weapon_melee:
		return u8"\uE04A";
	case weapon_axe:
		return u8"\uE04B";
	case weapon_hammer:
		return u8"\uE04C";
	case weapon_spanner:
		return u8"\uE04E";
	case weapon_knife_bayonet:
		return u8"\uE1F4";
	case weapon_knife_css:
		return u8"\uE1F7";
	case weapon_knife_flip:
		return u8"\uE1F9";
	case weapon_knife_gut:
		return u8"\uE1FA";
	case weapon_knife_karambit:
		return u8"\uE1FB";
	case weapon_knife_m9_bayonet:
		return u8"\uE1FC";
	case weapon_knife_tactical:
		return u8"\uE1FD";
	case weapon_knife_falchion:
		return u8"\uE200";
	case weapon_knife_survival_bowie:
		return u8"\uE202";
	case weapon_knife_butterfly:
		return u8"\uE203";
	case weapon_knife_push:
		return u8"\uE204";
	case weapon_knife_cord:
		return u8"\uE205";
	case weapon_knife_canis:
		return u8"\uE206";
	case weapon_knife_ursus:
		return u8"\uE207";
	case weapon_knife_gypsy_jackknife:
		return u8"\uE208";
	case weapon_knife_outdoor:
		return u8"\uE209";
	case weapon_knife_stiletto:
		return u8"\uE20A";
	case weapon_knife_widowmaker:
		return u8"\uE20B";
	case weapon_knife_skeleton:
		return u8"\uE20D";
	default:
		g_console.print(
			std::vformat( "! get_weapon_icon failed to find icon index: {} !", std::make_format_args( item_definition_index ) ).c_str( ) );

		return u8"";
	}
}

bool n_utilities::impl_t::is_weapon_valid( )
{
	auto weapon_handle = g_ctx.m_local->get_active_weapon_handle( );
	if ( !weapon_handle )
		return false;

	const auto active_weapon =
		reinterpret_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( weapon_handle ) );

	if ( !active_weapon )
		return false;

	const auto definition_index = active_weapon->get_item_definition_index( );

	return definition_index > 0 && !( definition_index >= 41 && definition_index <= 59 ) && !( definition_index >= 68 );
}
static std::pair< void*, std::size_t > getModuleInformation( const char* name ) noexcept
{
	if ( HMODULE handle = GetModuleHandleA( name ) ) {
		if ( MODULEINFO moduleInfo; GetModuleInformation( GetCurrentProcess( ), handle, &moduleInfo, sizeof( moduleInfo ) ) )
			return std::make_pair( moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage );
	}
	return { };
}

[[nodiscard]] static auto generateBadCharTable( std::string_view pattern ) noexcept
{
	assert( !pattern.empty( ) );

	std::array< std::size_t, ( std::numeric_limits< std::uint8_t >::max )( ) + 1 > table;

	auto lastWildcard = pattern.rfind( '?' );
	if ( lastWildcard == std::string_view::npos )
		lastWildcard = 0;

	const auto defaultShift = ( std::max )( std::size_t( 1 ), pattern.length( ) - 1 - lastWildcard );
	table.fill( defaultShift );

	for ( auto i = lastWildcard; i < pattern.length( ) - 1; ++i )
		table[ static_cast< std::uint8_t >( pattern[ i ] ) ] = pattern.length( ) - 1 - i;

	return table;
}

static std::uintptr_t findPattern( const char* moduleName, std::string_view pattern, bool reportNotFound = true ) noexcept
{
	static auto id = 0;
	++id;

	const auto [ moduleBase, moduleSize ] = getModuleInformation( moduleName );

	if ( moduleBase && moduleSize ) {
		const auto lastIdx      = pattern.length( ) - 1;
		const auto badCharTable = generateBadCharTable( pattern );

		auto start     = static_cast< const char* >( moduleBase );
		const auto end = start + moduleSize - pattern.length( );

		while ( start <= end ) {
			int i = lastIdx;
			while ( i >= 0 && ( pattern[ i ] == '?' || start[ i ] == pattern[ i ] ) )
				--i;

			if ( i < 0 )
				return reinterpret_cast< std::uintptr_t >( start );

			start += badCharTable[ static_cast< std::uint8_t >( start[ lastIdx ] ) ];
		}
	}

	if ( reportNotFound )
		MessageBoxA( NULL, ( "failed to find pattern #" + std::to_string( id ) + '!' ).c_str( ), "fuck(((", MB_OK | MB_ICONWARNING );
	return 0;
}
template< typename T >
static constexpr auto relativeToAbsolute( uintptr_t address ) noexcept
{
	return ( T )( address + 4 + *reinterpret_cast< std::int32_t* >( address ) );
}
void* PatternScan( const char* szModuleName, const char* szPattern )
{
	HMODULE module = GetModuleHandleA( szModuleName );
	if ( !module )
		return nullptr;

	static auto pattern_to_byte = []( const char* pattern ) {
		auto bytes = std::vector< int >{ };
		auto start = const_cast< char* >( pattern );
		auto end   = const_cast< char* >( pattern ) + strlen( pattern );

		for ( auto current = start; current < end; ++current ) {
			if ( *current == '?' ) {
				++current;
				if ( *current == '?' )
					++current;
				bytes.push_back( -1 );
			} else {
				bytes.push_back( strtoul( current, &current, 16 ) );
			}
		}
		return bytes;
	};

	auto dosHeader = ( PIMAGE_DOS_HEADER )module;
	auto ntHeaders = ( PIMAGE_NT_HEADERS )( ( std::uint8_t* )module + dosHeader->e_lfanew );

	auto sizeOfImage  = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = pattern_to_byte( szPattern );
	auto scanBytes    = reinterpret_cast< std::uint8_t* >( module );

	auto s = patternBytes.size( );
	auto d = patternBytes.data( );

	for ( auto i = 0ul; i < sizeOfImage - s; ++i ) {
		bool found = true;
		for ( auto j = 0ul; j < s; ++j ) {
			if ( scanBytes[ i + j ] != d[ j ] && d[ j ] != -1 ) {
				found = false;
				break;
			}
		}
		if ( found ) {
			return &scanBytes[ i ];
		}
	}
	return nullptr;
}
Memory::Memory( ) noexcept
{
		loadSky               = relativeToAbsolute< decltype( loadSky ) >( findPattern( "engine.dll", "\xE8????\x84\xC0\x74\x2D\xA1" ) + 1 );
	restoreEntityToPredictedFrame =
		reinterpret_cast< decltype( restoreEntityToPredictedFrame ) >( findPattern( "client", "\x55\x8B\xEC\x8B\x4D?\x56\xE8????\x8B\x75" ) );
		
		fuckin::m_AddressList[ CheatAddressList::ClearInventoryImageRGBA ] =
			( void* )( ( DWORD )(g_modules.m_modules[ CLIENT_DLL ].find_pattern( ( "55 8B EC 81 EC ? ? ? ? 57 8B F9 C7 47" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::InventoryItemByItemID ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 8B 55 08 83 EC 10 8B C2" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::AddEconItem ] =
			( void* )( ( DWORD )( PatternScan( ( "client.dll" ), ( "55 8B EC 83 E4 F8 A1 ? ? ? ? 83 EC 14 53 56 57 8B F9 8B 08" ) ) ) );
	
		 addEconItem = relativeToAbsolute< decltype( addEconItem ) >( findPattern( "client.dll", "\xE8????\x84\xC0\x74\xE7" ) + 1 );
		
		fuckin::m_AddressList[ CheatAddressList::CustomTagEconItem ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "E8 ? ? ? ? 8B 46 78 C1 E8 0A A8 01 74 13 8B 46 34" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::CustomDescEconItem ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "E8 ? ? ? ? 33 DB 39 5E 3C 7E 5E" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::CreateBaseTypeCache ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 51 53 56 8B D9 8D 45 08" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::SOCData ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 E4 F0 83 EC 18 56 8B F1 57 8B 86" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::StaticData ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 51 53 8B D9 8B 0D ? ? ? ? 56 57 8B B9" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::CreateEconItem ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 EC 1C 8D 45 E4 C7 45" ) ) ) + 0x14 );
		fuckin::m_AddressList[ CheatAddressList::ItemSystem ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "A1 ? ? ? ? 85 C0 75 53" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::SetAttributeValue ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 E4 F8 83 EC 3C 53 8B 5D 08 56 57 6A" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::PreCacheCustomMaterials ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 EC 0C 53 8B 5D 08 56 57 8B 7D 0C" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::DeleteItem ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 EC 24 56 57 83" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::UpdateGeneratedMaterial ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 53 56 57 8B 7D 10 8B F1" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::EquipWearable ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 EC 10 53 8B 5D 08 57 8B F9" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::InitializeAttibutes ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 E4 F8 83 EC 0C 53 56 8B F1 8B 86" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::GetLootListItemsCount_Return ] = ( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(
			( "85 C0 0F 84 ? ? ? ? 8B C8 E8 ? ? ? ? 52 50 E8 ? ? ? ? 83 C4 08 89 44 24 14 85 C0 0F 84 ? ? ? ? 8B 0D" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::FailedToOpenCrate ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(
	                                                    ( "68 ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0 84 C0 74 1A 8B 35 ? ? ? ? 8B D3 33 C9 8B 3E E8 ? ? "
	                                                        "? ? 50 8B CE FF 97 ? ? ? ? 5F 5E B0 01 5B 8B E5 5D C2 04" ) ) ) +
	                   0x1 );
		fuckin::m_AddressList[ CheatAddressList::CreateSeriesByID ] = ( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(
			( "55 8B EC 8B 45 08 56 57 8B 30 8B 41 10 83 F8 FF 74 1E 8B 79 04 8D 0C 40 8B 54 CF 10 3B D6 7E 05" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::IsConnected_Return ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "84 C0 75 05 B0 01 5F" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::ItemCustomizationNotification ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "68 ? ? ? ? 8B 80 ? ? ? ? FF D0 84 C0 74 28" ) ) ) + 0x1 );
		fuckin::m_AddressList[ CheatAddressList::GetLootListByName ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 E4 F8 8B 55 08 81 EC ? ? ? ? 56 57" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::GetLootListInterfaceByIndex ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 8B 55 08 56 8B F1 85 D2 78 47" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::ToolCanApplyTo ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "55 8B EC 83 EC 18 53 56 8B F1 57 8B FA" ) ) ) );
		fuckin::m_AddressList[ CheatAddressList::FireBullets_Return ] =
			( void* )( ( DWORD )( g_modules.m_modules[ CLIENT_DLL ].find_pattern(( "3B 3D ?? ?? ?? ?? 75 4C" ) ) ) + 0x6 );
		fuckin::m_AddressList[ CheatAddressList::RunFrame ] =
			( void* )( ( DWORD )( g_modules.m_modules[ ENGINE_DLL ].find_pattern( ( "55 8B EC 83 EC ? 53 56 8A D9" ) ) ) );
}