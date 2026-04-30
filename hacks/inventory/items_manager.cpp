#include "items_manager.h"
#include "algorithm"
#include "../dependencies/fnv1a/fnv1a.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
namespace
{
	constexpr uint32_t HASH_CSGO_TYPE_PISTOL              = fnv1a::hash_const( "#CSGO_Type_Pistol" );
	constexpr uint32_t HASH_CSGO_TYPE_RIFLE               = fnv1a::hash_const( "#CSGO_Type_Rifle" );
	constexpr uint32_t HASH_CSGO_TYPE_SNIPER_RIFLE        = fnv1a::hash_const( "#CSGO_Type_SniperRifle" );
	constexpr uint32_t HASH_CSGO_TYPE_MACHINEGUN          = fnv1a::hash_const( "#CSGO_Type_Machinegun" );
	constexpr uint32_t HASH_CSGO_TYPE_SMG                 = fnv1a::hash_const( "#CSGO_Type_SMG" );
	constexpr uint32_t HASH_CSGO_TYPE_SHOTGUN             = fnv1a::hash_const( "#CSGO_Type_Shotgun" );
	constexpr uint32_t HASH_CSGO_TYPE_KNIFE               = fnv1a::hash_const( "#CSGO_Type_Knife" );
	constexpr uint32_t HASH_CSGO_TYPE_GRENADE             = fnv1a::hash_const( "#CSGO_Type_Grenade" );
	constexpr uint32_t HASH_CSGO_TYPE_EQUIPMENT           = fnv1a::hash_const( "#CSGO_Type_Equipment" );
	constexpr uint32_t HASH_CSGO_TYPE_C4                  = fnv1a::hash_const( "#CSGO_Type_C4" );
	constexpr uint32_t HASH_TYPE_HANDS                    = fnv1a::hash_const( "#Type_Hands" );
	constexpr uint32_t HASH_CSGO_TYPE_COLLECTIBLE         = fnv1a::hash_const( "#CSGO_Type_Collectible" );
	constexpr uint32_t HASH_TYPE_CUSTOM_PLAYER            = fnv1a::hash_const( "#Type_CustomPlayer" );
	constexpr uint32_t HASH_CSGO_TYPE_MUSIC_KIT           = fnv1a::hash_const( "#CSGO_Type_MusicKit" );
	constexpr uint32_t HASH_CSGO_TYPE_WEAPON_CASE         = fnv1a::hash_const( "#CSGO_Type_WeaponCase" );
	constexpr uint32_t HASH_CSGO_TOOL_WEAPON_CASE_KEY_TAG = fnv1a::hash_const( "#CSGO_Tool_WeaponCase_KeyTag" );
} // namespace
void n_items_manager::impl_t::Initialize( )
{
	const auto item_system = C_ItemSystem::GetItemSystem( );

	for ( uint32_t i = NULL; i < item_system->m_nCountItemID; i++ ) {
		C_EconItemDefinition* item = *reinterpret_cast< C_EconItemDefinition** >( item_system->m_nItemID + 0x4 * ( 0x3 * i ) + 0x4 );
		switch ( fnv1a::hash( item->m_szType ) ) {
		case HASH_CSGO_TYPE_PISTOL:
		case HASH_CSGO_TYPE_RIFLE:
		case HASH_CSGO_TYPE_SNIPER_RIFLE:
		case HASH_CSGO_TYPE_MACHINEGUN:
		case HASH_CSGO_TYPE_SMG:
		case HASH_CSGO_TYPE_SHOTGUN:
		case HASH_CSGO_TYPE_KNIFE:
		case HASH_CSGO_TYPE_GRENADE:
		case HASH_CSGO_TYPE_EQUIPMENT:
		case HASH_CSGO_TYPE_C4:
		case HASH_TYPE_HANDS:
		case HASH_CSGO_TYPE_COLLECTIBLE:
		case HASH_TYPE_CUSTOM_PLAYER:
		case HASH_CSGO_TYPE_MUSIC_KIT:
		case HASH_CSGO_TYPE_WEAPON_CASE:
		case HASH_CSGO_TOOL_WEAPON_CASE_KEY_TAG:
			items[ item->m_nID ] = item;
			break;
		}
	}

	for ( size_t i = NULL; i < item_system->m_nCountPaintKit; i++ ) {
		auto kit            = *reinterpret_cast< C_EconPaintKitDefinition** >( item_system->m_nPaintKit + 0x18 * i + 0x14 );
		const auto kit_name = kit->m_szName;

		for ( size_t j = NULL; j < item_system->m_nCountAlternativeIcon; j++ ) {
			auto name = std::string( *reinterpret_cast< const char** >( 0x78 * j + item_system->m_nAlternativeIcon + 0x18 ) );

			if ( name[ name.length( ) - 0x7 ] != '_' )
				continue;

			name = name.substr( 0x17, name.length( ) - 0x1E );

			const auto pos = name.find( kit_name );
			if ( pos == std::string::npos )
				continue;

			const auto weapon_name = name.substr( NULL, pos - 0x1 );

			if ( name.length( ) - pos != strlen( kit_name ) )
				continue;

			auto weapon = item_system->GetItemSchemaInterface( )->GetItemDefentionByName( weapon_name.c_str( ) );
			if ( !weapon )
				continue;

			PaintKitsType[ weapon->m_nID ].emplace_back( kit );
			PaintKits[ kit->m_nID ] = kit;
		}
	}

	for ( const auto& it : PaintKitsType ) {
		const auto type = fnv1a::hash( items[ it.first ]->m_szType );

		if ( type == fnv1a::hash( "#CSGO_Type_Knife" ) || type == fnv1a::hash( "#Type_Hands" ) )
			continue;

		std::sort( PaintKitsType[ it.first ].begin( ), PaintKitsType[ it.first ].end( ),
		           []( auto a, auto b ) -> bool { return a->m_nRarity > b->m_nRarity; } );
	}

	for ( size_t i = 2; i < item_system->m_nCountMusicKit; i++ ) {
		auto kit                = *reinterpret_cast< C_EconMusicDefinition** >( item_system->m_nMusicKit + 0x18 * i + 0x14 );
		MusicKits[ kit->m_nID ] = kit;
	}

	for ( size_t i = 1; i < item_system->m_nCountStickerKit; i++ ) {
		auto sticker_kit                  = *reinterpret_cast< C_EconStickerDefinition** >( item_system->m_nStickerKit + 0x18 * i + 0x14 );
		StickerKits[ sticker_kit->m_nID ] = sticker_kit;
	}
}

std::string ToUtf8( std::wstring_view wstr )
{
	if ( wstr.empty( ) )
		return std::string( );

	auto size_needed = WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int32_t >( wstr.size( ) ), NULL, 0, NULL, NULL );

	std::string out( size_needed, 0 );
	WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int >( wstr.size( ) ), &out[ 0 ], size_needed, NULL, NULL );

	return out;
}

std::string LocalizeTex( const char* in )
{
	const auto wide_name = g_interfaces.m_localize->find_safe( in );

	if ( !wcslen( wide_name ) )
		return "";

	return ToUtf8( wide_name );
}
enum ItemDefinitionIndex {
	WEAPON_NONE,
	WEAPON_DEAGLE,
	WEAPON_ELITE,
	WEAPON_FIVESEVEN,
	WEAPON_GLOCK,
	WEAPON_AK47 = 7,
	WEAPON_AUG,
	WEAPON_AWP,
	WEAPON_FAMAS,
	WEAPON_G3SG1,
	WEAPON_GALILAR = 13,
	WEAPON_M249,
	WEAPON_M4A1 = 16,
	WEAPON_MAC10,
	WEAPON_P90 = 19,
	WEAPON_ZONE_REPULSOR,
	WEAPON_MP5SD = 23,
	WEAPON_UMP45,
	WEAPON_XM1014,
	WEAPON_BIZON,
	WEAPON_MAG7,
	WEAPON_NEGEV,
	WEAPON_SAWEDOFF,
	WEAPON_TEC9,
	WEAPON_TASER,
	WEAPON_HKP2000,
	WEAPON_MP7,
	WEAPON_MP9,
	WEAPON_NOVA,
	WEAPON_P250,
	WEAPON_SHIELD,
	WEAPON_SCAR20,
	WEAPON_SG553,
	WEAPON_SSG08,
	WEAPON_KNIFEGG,
	WEAPON_KNIFE,
	WEAPON_FLASHBANG,
	WEAPON_HEGRENADE,
	WEAPON_SMOKEGRENADE,
	WEAPON_MOLOTOV,
	WEAPON_DECOY,
	WEAPON_INCGRENADE,
	WEAPON_C4,
	WEAPON_HEALTHSHOT = 57,
	WEAPON_KNIFE_T    = 59,
	WEAPON_M4A1_SILENCER,
	WEAPON_USP_SILENCER,
	WEAPON_CZ75A = 63,
	WEAPON_REVOLVER,
	WEAPON_TAGRENADE = 68,
	WEAPON_FISTS,
	WEAPON_BREACHCHARGE,
	WEAPON_TABLET = 72,
	WEAPON_MELEE  = 74,
	WEAPON_AXE,
	WEAPON_HAMMER,
	WEAPON_SPANNER     = 78,
	WEAPON_KNIFE_GHOST = 80,
	WEAPON_FIREBOMB,
	WEAPON_DIVERSION,
	WEAPON_FRAG_GRENADE,
	WEAPON_SNOWBALL,
	WEAPON_BUMPMINE,
	WEAPON_KNIFE_BAYONET = 500,
	WEAPON_KNIFE_CSS     = 503,
	WEAPON_KNIFE_FLIP    = 505,
	WEAPON_KNIFE_GUT,
	WEAPON_KNIFE_KARAMBIT,
	WEAPON_KNIFE_M9_BAYONET,
	WEAPON_KNIFE_TACTICAL,
	WEAPON_KNIFE_FALCHION       = 512,
	WEAPON_KNIFE_SURVIVAL_BOWIE = 514,
	WEAPON_KNIFE_BUTTERFLY,
	WEAPON_KNIFE_PUSH,
	WEAPON_KNIFE_CORD,
	WEAPON_KNIFE_CANIS,
	WEAPON_KNIFE_URSUS,
	WEAPON_KNIFE_GYPSY_JACKKNIFE,
	WEAPON_KNIFE_OUTDOOR,
	WEAPON_KNIFE_STILETTO,
	WEAPON_KNIFE_WIDOWMAKER,
	WEAPON_KNIFE_SKELETON    = 525,
	GLOVE_STUDDED_BLOODHOUND = 5027,
	GLOVE_T_SIDE             = 5028,
	GLOVE_CT_SIDE            = 5029,
	GLOVE_SPORTY             = 5030,
	GLOVE_SLICK              = 5031,
	GLOVE_LEATHER_WRAP       = 5032,
	GLOVE_MOTORCYCLE         = 5033,
	GLOVE_SPECIALIST         = 5034,
	GLOVE_HYDRA              = 5035
};
void n_items_manager::impl_t::ImagePrecache( )
{
	const auto item_system = C_ItemSystem::GetItemSystem( );

	for ( uint32_t i = NULL; i < item_system->m_nCountItemID; i++ ) {
		auto item = *reinterpret_cast< C_EconItemDefinition** >( item_system->m_nItemID + 0x4 * ( 0x3 * i ) + 0x4 );

		if ( std::string( item->m_szName ).find( "#CSGO_CustomPlayer" ) != std::string::npos ) {
			if ( item->m_szInventoryImage != nullptr )
				m_vAgents.push_back( new AgentsItem_t{
					item->m_nID, item->m_szName, std::string( item->m_szInventoryImage ).find( "ctm" ) != std::string::npos ? 1 : 0,
					item->m_szInventoryImage != nullptr
						? GetSkin( std::string( "resource/flash/" + std::string( item->m_szInventoryImage ) + ".png" ) )
						: nullptr } );
		}

		switch ( item->m_nID ) {
		case WEAPON_DEAGLE:
		case WEAPON_ELITE:
		case WEAPON_FIVESEVEN:
		case WEAPON_GLOCK:
		case WEAPON_AK47:
		case WEAPON_AUG:
		case WEAPON_AWP:
		case WEAPON_FAMAS:
		case WEAPON_G3SG1:
		case WEAPON_GALILAR:
		case WEAPON_M249:
		case WEAPON_M4A1:
		case WEAPON_MAC10:
		case WEAPON_P90:
		case WEAPON_MP5SD:
		case WEAPON_UMP45:
		case WEAPON_XM1014:
		case WEAPON_BIZON:
		case WEAPON_MAG7:
		case WEAPON_NEGEV:
		case WEAPON_SAWEDOFF:
		case WEAPON_TEC9:
		case WEAPON_TASER:
		case WEAPON_HKP2000:
		case WEAPON_MP7:
		case WEAPON_MP9:
		case WEAPON_NOVA:
		case WEAPON_P250:
		case WEAPON_SCAR20:
		case WEAPON_SG553:
		case WEAPON_SSG08:
		case WEAPON_M4A1_SILENCER:
		case WEAPON_USP_SILENCER:
		case WEAPON_CZ75A:
		case WEAPON_REVOLVER:
		case WEAPON_KNIFE_BAYONET:
		case WEAPON_KNIFE_CSS:
		case WEAPON_KNIFE_FLIP:
		case WEAPON_KNIFE_GUT:
		case WEAPON_KNIFE_KARAMBIT:
		case WEAPON_KNIFE_M9_BAYONET:
		case WEAPON_KNIFE_TACTICAL:
		case WEAPON_KNIFE_FALCHION:
		case WEAPON_KNIFE_SURVIVAL_BOWIE:
		case WEAPON_KNIFE_BUTTERFLY:
		case WEAPON_KNIFE_PUSH:
		case WEAPON_KNIFE_CORD:
		case WEAPON_KNIFE_CANIS:
		case WEAPON_KNIFE_URSUS:
		case WEAPON_KNIFE_GYPSY_JACKKNIFE:
		case WEAPON_KNIFE_OUTDOOR:
		case WEAPON_KNIFE_STILETTO:
		case WEAPON_KNIFE_WIDOWMAKER:
		case WEAPON_KNIFE_SKELETON:
			m_vDefaults.push_back(
				new DefaultItem_t{ item->m_nID, item->m_szName,GetSkin( GetSkinPath( item->m_nID, 0 ) ) } );
			break;
		}
	}

	for ( size_t i = NULL; i < item_system->m_nCountPaintKit; i++ ) {
		auto kit            = *reinterpret_cast< C_EconPaintKitDefinition** >( item_system->m_nPaintKit + 0x18 * i + 0x14 );
		const auto kit_name = kit->m_szName;

		for ( size_t j = NULL; j < item_system->m_nCountAlternativeIcon; j++ ) {
			auto name = std::string( *reinterpret_cast< const char** >( 0x78 * j + item_system->m_nAlternativeIcon + 0x18 ) );

			if ( name[ name.length( ) - 0x7 ] != '_' )
				continue;

			name = name.substr( 0x17, name.length( ) - 0x1E );

			const auto pos = name.find( kit_name );
			if ( pos == std::string::npos )
				continue;

			const auto weapon_name = name.substr( NULL, pos - 0x1 );

			if ( name.length( ) - pos != strlen( kit_name ) )
				continue;

			auto weapon = item_system->GetItemSchemaInterface( )->GetItemDefentionByName( weapon_name.c_str( ) );
			if ( !weapon )
				continue;

			m_vWeapons.push_back( new WeaponItem_t{ weapon, kit, kit->m_nDescriptionTag,
			                                                     GetSkin( GetSkinPath( weapon->m_nID, kit->m_nID ) ) } );

			if ( weapon->m_nID == 5030 || weapon->m_nID == 5034 || weapon->m_nID == 5031 || weapon->m_nID == 5027 || weapon->m_nID == 5033 ||
			     weapon->m_nID == 5035 || weapon->m_nID == 4725 ) {
				m_vGloves.push_back( new GloveItem_t{ weapon->m_nID, kit->m_nDescriptionTag, kit->m_nID,
				                                                   GetSkin( GetSkinPath( weapon->m_nID, kit->m_nID ) ) } );
			}
		}
	}

	Initialized = true;
}

IDirect3DTexture9* n_items_manager::impl_t::GetSkin( std::string path )
{
	IDirect3DTexture9* skin_image = nullptr;

	void* entry;
	entry = g_interfaces.m_filesystem->Open( path.c_str( ), ( "r" ), ( "GAME" ) );

	if ( entry ) {
		int file_len = g_interfaces.m_filesystem->Size( entry );
		char* image  = new char[ file_len ];

		g_interfaces.m_filesystem->Read( image, file_len, entry );
		g_interfaces.m_filesystem->Close( entry );

		D3DXCreateTextureFromFileInMemory( g_interfaces.m_direct_device, image, file_len, &skin_image );
		delete[] image;
	}

	return skin_image;
}

std::string n_items_manager::impl_t::GetSkinPath( int16_t m_nItemID, int32_t m_iPaintKit )
{
	if ( !items[ m_nItemID ] )
		return "";

	if ( m_nItemID ) {
		if ( m_iPaintKit ) {
			if ( m_nItemID == 58 ) {
				return std::string( "resource/flash/" + std::string( MusicKits[ m_iPaintKit ]->m_szInventoryImage ) + ".png" );
			} else {
				return std::string( "resource/flash/econ/default_generated/" + std::string( items[ m_nItemID ]->m_szItemName ) + "_" +
				                    std::string( m_iPaintKit == 542 ? "cu_bizon_curse" : PaintKits[ m_iPaintKit ]->m_szName ) +
				                    "_light_large.png" );
			}
		} else if ( items[ m_nItemID ]->m_szType == "#Type_Hands" && !m_iPaintKit || m_nItemID == 58 && !m_iPaintKit ) {
			return "";
		} else {
			return std::string( "resource/flash/" + std::string( items[ m_nItemID ]->m_szInventoryImage ) + ".png" );
		}
	}
}
