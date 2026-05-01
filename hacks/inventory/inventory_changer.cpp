#include "inventory_changer.h"
#include "items_manager.h"
#include "algorithm"
#include "../dependencies/fnv1a/fnv1a.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../movement/movement_assist_simulation.h"
#include "../misc/scaleform/scaleform.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace
{
	void inventory_debug_log( const std::string& message )
	{
		if ( message.find( "failed" ) != std::string::npos )
			g_drainware_inventory_health = message;

		if ( !GET_VARIABLE( g_variables.m_debug_logging, bool ) )
			return;

		std::filesystem::create_directories( "C:\\drainware" );
		std::ofstream file( "C:\\drainware\\debuglog.txt", std::ios::app );
		if ( !file.good( ) )
			return;

		SYSTEMTIME time{ };
		GetLocalTime( &time );
		file << '[' << std::setfill( '0' ) << std::setw( 2 ) << time.wHour << ':' << std::setw( 2 ) << time.wMinute << ':' << std::setw( 2 )
		     << time.wSecond << "][skins] " << message << '\n';
	}
}

std::add_pointer_t< c_client_networkable* __cdecl( int32_t, int32_t ) > get_wearable_create_fn( )
{
	if ( !g_interfaces.m_base_client )
		return nullptr;

	for ( c_base_client* pClass = g_interfaces.m_base_client->get_all_classes( ); pClass != nullptr; pClass = pClass->m_next ) {
		if ( !pClass || !pClass->m_recv_table || fnv1a::hash( pClass->m_network_name ) != fnv1a::hash( "CEconWearable" ) )
			continue;

		return pClass->m_create_fn;
	}

	return nullptr;
}

void n_invetory_changer::impl_t::SequenceRemapping( c_recv_proxy_data* pData, c_base_entity* pEntity )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;

	if ( !pEntity || !pEntity->get_weapon_handle( ) )
		return;

	c_base_entity* pViewModelWeapon =
		static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( pEntity->get_weapon_handle( ) ) );
	if ( !pViewModelWeapon )
		return;

	c_base_entity* hOwner =
		static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( pViewModelWeapon->get_owner_entity_handle( ) ) );
	if ( hOwner != g_ctx.m_local )
		return;

	if ( !g_items_manager.items[ pViewModelWeapon->m_nItemID( ) ] )
		return;

	if ( fnv1a::hash( g_items_manager.items[ pViewModelWeapon->m_nItemID( ) ]->m_szType ) != fnv1a::hash( "#CSGO_Type_Knife" ) )
		return;

	auto& iSequence = pData->m_value.m_int;
	iSequence       = GetNewAnimation( fnv1a::hash( g_items_manager.items[ pViewModelWeapon->m_nItemID( ) ]->m_szDisplayModelName ), iSequence );
}
static int32_t random_sequence( int32_t min, int32_t max )
{
	return rand( ) % ( max - min + 1 ) + min;
}
int32_t n_invetory_changer::impl_t::GetNewAnimation( const uint32_t model, const int32_t sequence )
{
	enum e_sequence_num {
		default_draw           = 0,
		default_idle1          = 1,
		default_idle2          = 2,
		default_light_miss1    = 3,
		default_light_miss2    = 4,
		default_heavy_miss1    = 9,
		default_heavy_hit1     = 10,
		default_heavy_backstab = 11,
		default_lookat01       = 12,

		butterfly_draw     = 0,
		butterfly_draw2    = 1,
		butterfly_lookat01 = 13,
		butterfly_lookat03 = 15,

		falchion_idle1              = 1,
		falchion_heavy_miss1        = 8,
		falchion_heavy_miss1_noflip = 9,
		falchion_lookat01           = 12,
		falchion_lookat02           = 13,

		daggers_idle1       = 1,
		daggers_light_miss1 = 2,
		daggers_light_miss5 = 6,
		daggers_heavy_miss2 = 11,
		daggers_heavy_miss1 = 12,

		bowie_idle1 = 1,
	};

	switch ( model ) {
	case fnv1a::hash_const( "models/weapons/v_knife_butterfly.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_draw:
			return random_sequence( e_sequence_num::butterfly_draw, e_sequence_num::butterfly_draw2 );
		case e_sequence_num::default_lookat01:
			return random_sequence( e_sequence_num::butterfly_lookat01, e_sequence_num::butterfly_lookat03 );
		default:
			return sequence + 1;
		}
	}
	case fnv1a::hash_const( "models/weapons/v_knife_falchion_advanced.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_idle2:
			return e_sequence_num::falchion_idle1;
		case e_sequence_num::default_heavy_miss1:
			return random_sequence( e_sequence_num::falchion_heavy_miss1, e_sequence_num::falchion_heavy_miss1_noflip );
		case e_sequence_num::default_lookat01:
			return random_sequence( e_sequence_num::falchion_lookat01, e_sequence_num::falchion_lookat02 );
		case e_sequence_num::default_draw:
		case e_sequence_num::default_idle1:
			return sequence;
		default:
			return sequence - 1;
		}
	}
	case fnv1a::hash_const( "models/weapons/v_knife_push.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_idle2:
			return e_sequence_num::daggers_idle1;
		case e_sequence_num::default_light_miss1:
		case e_sequence_num::default_light_miss2:
			return random_sequence( e_sequence_num::daggers_light_miss1, e_sequence_num::daggers_light_miss5 );
		case e_sequence_num::default_heavy_miss1:
			return random_sequence( e_sequence_num::daggers_heavy_miss2, e_sequence_num::daggers_heavy_miss1 );
		case e_sequence_num::default_heavy_hit1:
		case e_sequence_num::default_heavy_backstab:
		case e_sequence_num::default_lookat01:
			return sequence + 3;
		case e_sequence_num::default_draw:
		case e_sequence_num::default_idle1:
			return sequence;
		default:
			return sequence + 2;
		}
	}
	case fnv1a::hash_const( "models/weapons/v_knife_survival_bowie.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_draw:
		case e_sequence_num::default_idle1:
			return sequence;
		case e_sequence_num::default_idle2:
			return e_sequence_num::bowie_idle1;
		default:
			return sequence - 1;
		}
	}
	case fnv1a::hash_const( "models/weapons/v_knife_ursus.mdl" ):
	case fnv1a::hash_const( "models/weapons/v_knife_cord.mdl" ):
	case fnv1a::hash_const( "models/weapons/v_knife_canis.mdl" ):
	case fnv1a::hash_const( "models/weapons/v_knife_outdoor.mdl" ):
	case fnv1a::hash_const( "models/weapons/v_knife_skeleton.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_draw:
			return random_sequence( e_sequence_num::butterfly_draw, e_sequence_num::butterfly_draw2 );
		case e_sequence_num::default_lookat01:
			return random_sequence( e_sequence_num::butterfly_lookat01, 14 );
		default:
			return sequence + 1;
		}
	}
	case fnv1a::hash_const( "models/weapons/v_knife_stiletto.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_lookat01:
			return random_sequence( 12, 13 );
		default:
			return sequence;
		}
	}
	case fnv1a::hash_const( "models/weapons/v_knife_widowmaker.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_lookat01:
			return random_sequence( 14, 15 );
		default:
			return sequence;
		}
	}
	case fnv1a::hash_const( "models/weapons/v_knife_css.mdl" ): {
		switch ( sequence ) {
		case e_sequence_num::default_lookat01:
			return 15;
		default:
			return sequence;
		}
	}
	default:
		return sequence;
	}
}
void n_invetory_changer::impl_t::ApplyGlove( IPlayerInventory* LocalInventory, player_info_t info )
{
	if ( !LocalInventory || !g_ctx.m_local )
		return;

	C_EconItemView* pEconItemView = LocalInventory->GetItemInLoadout( static_cast< int32_t >( g_ctx.m_local->get_team( ) ), 41 );
	if ( !pEconItemView )
		return;

	C_EconItem* pSocData = pEconItemView->GetSocData( );
	if ( !pSocData )
		return;

	C_EconItemDefinition* pStaticData = pEconItemView->GetStaticData( );
	if ( !pStaticData )
		return;

	auto hMyWearables = g_ctx.m_local->get_wearables_handle( );
	if ( !hMyWearables )
		return;

	constexpr std::uint64_t iMask = ( std::numeric_limits< std::uint32_t >::max )( );
	const std::uint32_t iLowID    = iMask & pSocData->m_nItemID;
	const std::uint32_t iHighID   = pSocData->m_nItemID >> 32;

	c_base_entity* pWearable = static_cast< c_base_entity* >(g_interfaces.m_client_entity_list->get_client_entity_from_handle( hMyWearables[ 0 ] ) );
	if ( !pWearable || static_cast< std::uint32_t >( pWearable->get_item_id_high( ) ) != iHighID ||
	     static_cast< std::uint32_t >( pWearable->get_item_id_low( ) ) != iLowID ) {
		int Entry = g_interfaces.m_client_entity_list->get_highest_entity_index( ) + 1;
		for ( int nEntityID = 0; nEntityID < Entry - 1; nEntityID++ ) {
			auto pEntity = g_interfaces.m_client_entity_list->get_client_entity( nEntityID );
			if ( !pEntity || pEntity->get_client_class( )->m_class_id != e_class_ids::c_rope_keyframe )
				continue;

			Entry = nEntityID;
			break;
		}

		const int nSerialID         = rand( ) % 0x1000;
		const auto CreateWearableFn = get_wearable_create_fn( );
		if ( !CreateWearableFn ) {
			inventory_debug_log( "ApplyGlove failed reason=missing CEconWearable create fn" );
			return;
		}
		CreateWearableFn( Entry, nSerialID );

		pWearable = reinterpret_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity( Entry ) );
		if ( pWearable ) {
			hMyWearables[ 0 ] = Entry | nSerialID << 16;

			pWearable->get_item_id_low( )   = iLowID;
			pWearable->get_item_id_high( )  = iHighID;
			pWearable->m_nItemID( )      = pSocData->m_nWeaponID;
			pWearable->get_account_id( )    = static_cast< int32_t >( info.m_ull_xuid );
			pWearable->m_bInitialized( )    = true;
			pWearable->set_model_index( g_interfaces.m_model_info->get_model_index( pStaticData->m_szWorldModelName ) );
			pWearable->EquipWearable( g_ctx.m_local );
			g_ctx.m_local->m_nBody( ) = 1;
			pWearable->InitializeAttibutes( );
		}
	}
}
void n_invetory_changer::impl_t::ApplyWeapons( IPlayerInventory* LocalInventory, player_info_t Info )
{
	if ( !LocalInventory || !g_ctx.m_local || !g_interfaces.m_client_entity_list )
		return;

	auto hMyWeapons = g_ctx.m_local->get_weapons_handle( );
	if ( !hMyWeapons )
		return;

	for ( auto iIndex = 0; hMyWeapons[ iIndex ] != 0xFFFFFFFF; iIndex++ ) {
		c_base_entity* pWeapon =
			static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( hMyWeapons[ iIndex ] ) );
		if ( !pWeapon )
			continue;

		const auto weapon_id = pWeapon->m_nItemID( );
		const auto weapon_definition = g_items_manager.items[ weapon_id ];
		if ( !weapon_definition || !weapon_definition->m_szType )
			continue;

		if ( fnv1a::hash( weapon_definition->m_szType ) == fnv1a::hash( "#CSGO_Type_Knife" ) )
			continue;

		if ( Info.m_xuid_high != pWeapon->get_owner_xuid_high( ) || Info.m_xuid_low != pWeapon->get_owner_xuid_low( ) )
			continue;

		C_EconItemView* pEconItemView = LocalInventory->GetItemInLoadout( static_cast< int32_t >( g_ctx.m_local->get_team( ) ),
		                                                                  weapon_definition->m_nLoadoutSlot );
		if ( !pEconItemView )
			continue;

		C_EconItem* pSocData = pEconItemView->GetSocData( );
		if ( !pSocData )
			continue;

		if ( pWeapon->m_nItemID( ) != pSocData->m_nWeaponID )
			continue;

		constexpr std::uint64_t iMask = ( std::numeric_limits< std::uint32_t >::max )( );
		const std::uint32_t iLowID    = iMask & pSocData->m_nItemID;
		const std::uint32_t iHighID   = pSocData->m_nItemID >> 32;

		pWeapon->get_account_id( )  = static_cast< int32_t >( Info.m_ull_xuid );
		pWeapon->get_item_id_low( )  = iLowID;
		pWeapon->get_item_id_high( ) = iHighID;
	}
}
void n_invetory_changer::impl_t::ApplyKnifes( IPlayerInventory* LocalInventory, player_info_t info )
{
	if ( !LocalInventory || !g_ctx.m_local || !g_interfaces.m_client_entity_list || !g_interfaces.m_model_info )
		return;

	C_EconItemView* pEconItemView = LocalInventory->GetItemInLoadout( static_cast< int >( g_ctx.m_local->get_team( ) ), 0 );
	if ( !pEconItemView )
		return;

	C_EconItem* pSocData = pEconItemView->GetSocData( );
	if ( !pSocData )
		return;

	auto m_hMyWeapons = g_ctx.m_local->get_weapons_handle( );
	if ( !m_hMyWeapons )
		return;

	for ( auto iIndex = 0; m_hMyWeapons[ iIndex ] != 0xFFFFFFFF; iIndex++ ) {
		c_base_entity* pWeapon =
			static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( m_hMyWeapons[ iIndex ] ) );
		if ( !pWeapon )
			continue;

		const auto weapon_id = pWeapon->m_nItemID( );
		const auto weapon_definition = g_items_manager.items[ weapon_id ];
		if ( !weapon_definition || !weapon_definition->m_szType ||
		     fnv1a::hash( weapon_definition->m_szType ) != fnv1a::hash( "#CSGO_Type_Knife" ) )
			continue;

		if ( info.m_xuid_low != pWeapon->get_owner_xuid_low( ) || info.m_xuid_high != pWeapon->get_owner_xuid_high( ) )
			continue;

		constexpr std::uint64_t iMask = ( std::numeric_limits< std::uint32_t >::max )( );
		const std::uint32_t iLowID    = iMask & pSocData->m_nItemID;
		const std::uint32_t iHighID   = pSocData->m_nItemID >> 32;

		pWeapon->get_item_id_low( )  = iLowID;
		pWeapon->get_item_id_high( ) = iHighID;
		pWeapon->get_account_id( )  = static_cast< int32_t >( info.m_ull_xuid );
		pWeapon->m_nItemID( )     = pSocData->m_nWeaponID;
		const auto knife_definition = g_items_manager.items[ pSocData->m_nWeaponID ];
		if ( knife_definition && knife_definition->m_szDisplayModelName )
			pWeapon->set_model_index( g_interfaces.m_model_info->get_model_index( knife_definition->m_szDisplayModelName ) );
	}

	/* Get viewmodel */
	auto pViewModel =  reinterpret_cast< c_base_entity* >(g_interfaces.m_client_entity_list->get_client_entity_from_handle(g_ctx.m_local->get_view_model_handle( )));
	if ( !pViewModel )
		return;

	/* Get viewmodel weapon */
	auto pViewModelWeapon = reinterpret_cast< c_base_entity* >(g_interfaces.m_client_entity_list->get_client_entity_from_handle(pViewModel->get_weapon_handle( )));
	if ( !pViewModelWeapon )
		return;

	/* Set world model index */
	c_base_entity* pWorldModel = reinterpret_cast< c_base_entity* >(
		g_interfaces.m_client_entity_list->get_client_entity_from_handle( pViewModelWeapon->m_hWeaponWorldModel( ) ) );
	const auto viewmodel_definition = g_items_manager.items[ pViewModelWeapon->m_nItemID( ) ];
	if ( pWorldModel && viewmodel_definition && viewmodel_definition->m_szWorldModelName )
		pWorldModel->get_model_index( ) =
			g_interfaces.m_model_info->get_model_index( viewmodel_definition->m_szWorldModelName );

	/* Set viewmodel weapon index */
	if ( viewmodel_definition && viewmodel_definition->m_szDisplayModelName )
		pViewModel->get_model_index( ) = g_interfaces.m_model_info->get_model_index( viewmodel_definition->m_szDisplayModelName );
}

void n_invetory_changer::impl_t::ApplyAgents( IPlayerInventory* LocalInventory, e_client_frame_stage Stage )
{
	C_EconItemView* m_LoadOut = LocalInventory->GetItemInLoadout( static_cast< int32_t >( g_ctx.m_local->get_team( ) ), 38 );
	if ( !m_LoadOut )
		return;

	C_EconItem* m_Item = m_LoadOut->GetSocData( );
	if ( !m_Item )
		return;

	C_EconItemDefinition* pDefIndex = g_items_manager.items[ m_Item->m_nWeaponID ];
	if ( !pDefIndex || fnv1a::hash( pDefIndex->m_szType ) != fnv1a::hash( "#Type_CustomPlayer" ) )
		return;

	const char* m_szModel = pDefIndex->m_szDisplayModelName;
	if ( !m_szModel )
		return;

	int nModelIdx = g_interfaces.m_model_info->get_model_index( m_szModel );
	if ( !nModelIdx )
		return;

	g_ctx.m_local->set_model_index( nModelIdx );
	if ( c_base_entity* pRagdoll =
	         static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( g_ctx.m_local->m_hRagdoll( ) ) ) )
		pRagdoll->set_model_index( nModelIdx );

	m_nModelIdx = nModelIdx;
}
c_uipanel* menu_root;
c_uipanel* GetRoot( )
{
	auto panel    = g_interfaces.m_panorama->access_ui_engine( )->get_last_dispatched_event_target_panel( );
	c_uipanel* ret = nullptr;

	while ( panel && g_interfaces.m_panorama->access_ui_engine( )->is_valid_panel_ptr( panel ) ) {
		if ( fnv1a::hash( panel->get_id( ) ) == fnv1a::hash( "CSGOHud" ) ) {
			ret = panel;
			break;
		} else if ( fnv1a::hash( panel->get_id( ) ) == fnv1a::hash( "CSGOMainMenu" ) ) {
			ret = panel;
			break;
		}

		panel = panel->get_parent( );
	}
	return ret;
}

void n_invetory_changer::impl_t::AddItemToInventory( IPlayerInventory* pLocalInventory, InventoryItem_t item, bool bAcKnowledge, std::string szParameter )
{
	if ( !pLocalInventory ) {
		inventory_debug_log( "AddItemToInventory failed reason=null local inventory" );
		return;
	}

	if ( item.m_nItemID <= 0 || !g_items_manager.items[ item.m_nItemID ] ) {
		inventory_debug_log( "AddItemToInventory failed reason=invalid item id=" + std::to_string( item.m_nItemID ) );
		return;
	}

	auto pItem = C_EconItem::CreateEconItem( );
	if ( !pItem ) {
		inventory_debug_log( "AddItemToInventory failed reason=CreateEconItem returned null" );
		return;
	}

	pItem->m_nItemID     = pLocalInventory->GetLastValueID( ) + iUniqueID++;
	pItem->m_nOriginalID = 0;
	pItem->m_nAccountID  = static_cast< uint32_t >( pLocalInventory->GetOwnerID( ).m_nID );
	pItem->m_nInventory  = bAcKnowledge ? 0 : 1;
	pItem->m_nQuality    = 4;
	pItem->m_nWeaponID   = item.m_nItemID;

	const char* szType = g_items_manager.items[ item.m_nItemID ]->m_szType;
	if ( !szType ) {
		inventory_debug_log( "AddItemToInventory failed reason=null item type id=" + std::to_string( item.m_nItemID ) );
		return;
	}
	if ( szType == ( "#CSGO_Type_MusicKit" ) ) {
		pItem->m_nRarity = 3;
		pItem->SetMusicID( item.m_iPaintKit );

		if ( item.m_StatTrack.enabled ) {
			pItem->SetStatTrack( item.m_StatTrack.counter );
			pItem->SetStatTrackType( 1 );
			pItem->m_nQuality = 9;
		}
	} else {
		pItem->SetPaintKit( static_cast< float_t >( item.m_iPaintKit ) );
		pItem->SetWear( item.m_flWear );
		pItem->SetSeed( item.m_flSeed );
		pItem->SetCustomName( item.m_Customtag.aTagName );
		pItem->SetCustomDesc( "drainware" );

		for ( std::size_t j = 0; j < item.m_aStickers.size( ); ++j ) {
			const auto& sticker = item.m_aStickers[ j ];
			if ( sticker.m_nID == 0 )
				continue;

			pItem->SetStickerID( j, sticker.m_nID );
			pItem->SetStickerWear( j, sticker.m_flWear );
		}

		if ( item.m_StatTrack.enabled && szType != ( "#Type_Hands" ) ) {
			pItem->SetStatTrack( item.m_StatTrack.counter );
			pItem->SetStatTrackType( 0 );
			pItem->m_nQuality = 9;
		}

		if ( fnv1a::hash( szType ) == fnv1a::hash( "#CSGO_Type_Knife" ) || fnv1a::hash( szType ) == fnv1a::hash( "#Type_Hands" ) )
			pItem->m_nQuality = 3;

		if ( item.m_CustomRarity.enabled )
			pItem->m_nRarity = item.m_CustomRarity.counter;
		else {
			if ( item.m_iPaintKit ) {
				if ( !g_items_manager.PaintKits[ item.m_iPaintKit ] ) {
					inventory_debug_log( "AddItemToInventory failed reason=invalid paint kit=" + std::to_string( item.m_iPaintKit ) );
					return;
				}
				item.m_iRarity =
					std::clamp( g_items_manager.items[ item.m_nItemID ]->GetRarityValue( ) + g_items_manager.PaintKits[ item.m_iPaintKit ]->m_nRarity - 1,
				                0, ( g_items_manager.PaintKits[ item.m_iPaintKit ]->m_nRarity == 7 ) ? 7 : 6 );
			} else
				item.m_iRarity = g_items_manager.items[ item.m_nItemID ]->GetRarityValue( );

			pItem->m_nRarity = item.m_iRarity;
		}

		pItem->m_nQuality = item.m_iQuality;
	}

	pLocalInventory->GetItemBaseTypeCache( )->AddObject( pItem );
	pLocalInventory->SoCreated( pLocalInventory->GetOwnerID( ), ( C_SharedObject* )pItem, 4 );

	if ( const auto pView = pLocalInventory->GetInventoryItemByItemID( pItem->m_nItemID ) )
		pView->GetClearInventoryImageRGBA( );

	m_aInventoryItemList.emplace( pItem->m_nItemID, item );

	bool isKnife = ( fnv1a::hash( szType ) == fnv1a::hash( "#CSGO_Type_Knife" ) );

	if ( isKnife ) {
		for ( const auto& [ itemID, storedItem ] : m_aInventoryItemList ) {
			if ( itemID == pItem->m_nItemID )
				continue;

			const char* oldType = g_items_manager.items[ storedItem.m_nItemID ]->m_szType;

			if ( fnv1a::hash( oldType ) == fnv1a::hash( "#CSGO_Type_Knife" ) ) {
				if ( item.auto_equipment_ct && storedItem.auto_equipment_ct )
					g_interfaces.m_inventory_manager->EquipItemInLoadout( 3, g_items_manager.items[ storedItem.m_nItemID ]->m_nLoadoutSlot, 0, false );

				if ( item.auto_equipment_t && storedItem.auto_equipment_t )
					g_interfaces.m_inventory_manager->EquipItemInLoadout( 2, g_items_manager.items[ storedItem.m_nItemID ]->m_nLoadoutSlot, 0, false );
			}
		}
	}

	if ( item.auto_equipment_t && item.auto_equipment_ct ) {
		g_interfaces.m_inventory_manager->EquipItemInLoadout( 3, g_items_manager.items[ item.m_nItemID ]->m_nLoadoutSlot, pItem->m_nItemID, false );
		g_interfaces.m_inventory_manager->EquipItemInLoadout( 2, g_items_manager.items[ item.m_nItemID ]->m_nLoadoutSlot, pItem->m_nItemID, false );
	} else {
		if ( item.auto_equipment_ct && !item.auto_equipment_t )
			g_interfaces.m_inventory_manager->EquipItemInLoadout( 3, g_items_manager.items[ item.m_nItemID ]->m_nLoadoutSlot, pItem->m_nItemID,
			                                                       false );
		else if ( item.auto_equipment_t && !item.auto_equipment_ct )
			g_interfaces.m_inventory_manager->EquipItemInLoadout( 2, g_items_manager.items[ item.m_nItemID ]->m_nLoadoutSlot, pItem->m_nItemID,
			                                                       false );
	}
	std::string message = std::string( "$.DispatchEvent('PanoramaComponent_Inventory_ItemCustomizationNotification', '0','" + szParameter +
	                                   ( "','" ) + std::to_string( pItem->m_nItemID ) + ( "');" ) )
	                          .c_str( );
	if ( !g_scaleform.m_uiengine ) {
		g_scaleform.m_uiengine = g_interfaces.m_panorama->access_ui_engine( );
		
	} else {
		if ( menu_root ) {
			g_scaleform.m_uiengine->run_script( menu_root, message.c_str( ), "panorama/layout/base_mainmenu.xml", 8, 10, false, false );
		} else {
			menu_root = GetRoot( );
			if ( menu_root ) {
				g_scaleform.m_uiengine->run_script( menu_root, message.c_str( ), "panorama/layout/base_mainmenu.xml", 8, 10, false, false );
			}
		}
	}
}
void n_invetory_changer::impl_t::OnPostDataUpdateStart( IPlayerInventory* LocalInventory )
{
	if ( !LocalInventory || !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !g_interfaces.m_client_state ||
	     g_interfaces.m_client_state->m_signon_state < 6 || !g_interfaces.m_engine_client )
		return;

	player_info_t Info;
	if ( !g_interfaces.m_engine_client->get_player_info( g_ctx.m_local->get_index( ), &Info ) )
		return;

	ApplyWeapons( LocalInventory, Info );
	ApplyKnifes( LocalInventory, Info );
	ApplyGlove( LocalInventory, Info );
}
void n_invetory_changer::impl_t::OnSoUpdated( C_SharedObject* object, int32_t event )
{
	if ( iLastEquipped <= 0 && object->GetTypeID( ) != 0x2B )
		return;

	*reinterpret_cast< int16_t* >( std::uintptr_t( object ) + 0x10 ) = 0;
	--iLastEquipped;
}
void n_invetory_changer::impl_t::OnEquipItemInLoadOut( int32_t iTeam, int32_t iSlot, uint64_t iItemID )
{
	const auto pLocalInventory = g_interfaces.m_inventory_manager->GetLocalPlayerInventory( );
	if ( !pLocalInventory )
		return;

	const auto& inventory_item_it = m_aInventoryItemList.find( iItemID );
	if ( inventory_item_it == m_aInventoryItemList.end( ) )
		return;

	if ( fnv1a::hash( g_items_manager.items[ inventory_item_it->second.m_nItemID ]->m_szType ) == fnv1a::hash( "#CSGO_Type_Collectible" ) ) {
		if ( const auto pView = pLocalInventory->GetInventoryItemByItemID( iItemID ) ) {
			if ( const auto pEconitem = pView->GetSocData( ) )
				pLocalInventory->SoUpdated( pLocalInventory->GetOwnerID( ), ( C_SharedObject* )pEconitem, 0x4 );
		}
	} else {
		g_interfaces.m_inventory_manager->EquipItemInLoadout(
			iTeam, iSlot, ( uint64_t( 0xF ) << 0x3C ) | static_cast< int16_t >( inventory_item_it->second.m_nItemID ), false );

		C_EconItemView* pView = pLocalInventory->GetItemInLoadout( iTeam, iSlot );
		if ( pView ) {
			if ( C_EconItem* m_Item = pView->GetSocData( ) )
				pLocalInventory->SoUpdated( pLocalInventory->GetOwnerID( ), ( C_SharedObject* )m_Item, 0x4 );
		}

		++iLastEquipped;
	}
}
inline std::uintptr_t* find_hud_element( const char* name )
{
	static auto hud = *reinterpret_cast< void** >( g_modules.m_modules[ CLIENT_DLL ].find_pattern( "B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89" ) + 0x1 );

	using find_hud_element_fn      = std::uintptr_t*( __thiscall* )( void*, const char* );
	static auto o_find_hud_element = reinterpret_cast< find_hud_element_fn >(
		g_modules.m_modules[ CLIENT_DLL ].find_pattern( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) );
	assert( o_find_hud_element != nullptr );

	return o_find_hud_element( hud, name );
}

void n_invetory_changer::impl_t::full_hud_update( )
{
	if ( !g_ctx.force_full_update )
		return;

	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;

	// update hud
	using clear_hud_weapon_icon_fn      = int( __thiscall* )( void*, int );
	static auto o_clear_hud_weapon_icon = reinterpret_cast< clear_hud_weapon_icon_fn >(
		g_modules.m_modules[ CLIENT_DLL ].find_pattern( "55 8B EC 51 53 56 8B 75 08 8B D9 57 6B" ) ); // @xref: "WeaponIcon--itemcount"
	assert( o_clear_hud_weapon_icon != nullptr );
	if ( const auto hud_weapons = find_hud_element( "CCSGO_HudWeaponSelection" ) - 0x28; hud_weapons != nullptr ) {
		// go through all weapons
		for ( std::size_t i = 0; i < *( hud_weapons + 0x20 ); i++ )
			i = o_clear_hud_weapon_icon( hud_weapons, i );
	}

	g_interfaces.m_client_state->m_delta_tick = -1;
	g_ctx.force_full_update                   = false;
}
void n_invetory_changer::impl_t::OnFrameStageNotify( e_client_frame_stage Stage )
{
	drainware_stability_breadcrumb( "inventory_frame_stage" );
	if ( !g_interfaces.m_inventory_manager || !g_ctx.m_local ) {
		g_drainware_inventory_health = "waiting for local inventory";
		return;
	}

	IPlayerInventory* pLocalInventory = g_interfaces.m_inventory_manager->GetLocalPlayerInventory( );
	if ( !pLocalInventory ) {
		g_drainware_inventory_health = "missing local inventory";
		return;
	}
	g_drainware_inventory_health = "OK";

	switch ( Stage ) {
	case e_client_frame_stage::render_start: {
		
		ApplyAgents( pLocalInventory, Stage );
	} break;

	case e_client_frame_stage::net_update_postdataupdate_start: {
		OnPostDataUpdateStart( pLocalInventory );
	} break;
	case e_client_frame_stage::net_update_postdataupdate_end: {
		ApplyAgents( pLocalInventory, Stage );
	} break;

	default:
		break;
	}

	if ( g_ctx.m_bNeedFullUpdate ) {
		g_ctx.force_full_update = true;
		full_hud_update( );
		g_ctx.m_bNeedFullUpdate                   = false;
	}
}
void n_invetory_changer::impl_t::OnEvent( game_event_t* m_GameEvent )
{
	IPlayerInventory* pLocalInventory = g_interfaces.m_inventory_manager->GetLocalPlayerInventory( );
	if ( !pLocalInventory )
		return;

	const uint32_t& uHash = fnv1a::hash( m_GameEvent->get_name( ) );
	switch ( uHash ) {
	case fnv1a::hash_const( "player_death" ): {
		ChangeHudIcons( pLocalInventory, m_GameEvent );
		LiveStatTrack( pLocalInventory, m_GameEvent );
	} break;

	case fnv1a::hash_const( "round_mvp" ): {
		/* Check for LocalPlayer */
		if ( g_interfaces.m_engine_client->get_local_player( ) !=
		     g_interfaces.m_engine_client->get_player_for_user_id( m_GameEvent->get_int( ( "userid" ) ) ) )
			return;

		C_EconItemView* pEconItemView = pLocalInventory->GetItemInLoadout( NULL, 54 );
		if ( !pEconItemView )
			return;

		C_EconItem* pSocData = pEconItemView->GetSocData( );
		if ( !pSocData )
			return;

		auto aInventoryItem = m_aInventoryItemList.find( pSocData->m_nItemID );
		if ( aInventoryItem == m_aInventoryItemList.end( ) )
			return;

		auto& pInventoryItem = aInventoryItem->second;
		if ( !pInventoryItem.m_StatTrack.enabled )
			return;

		m_GameEvent->set_int( ( "musickitmvps" ), ++pInventoryItem.m_StatTrack.counter );
		pSocData->SetStatTrack( pInventoryItem.m_StatTrack.counter );
		pLocalInventory->SoUpdated( pLocalInventory->GetOwnerID( ), ( C_SharedObject* )pSocData, 4 );
	} break;
	}
}


void n_invetory_changer::impl_t::ChangeHudIcons( IPlayerInventory* LocalInventory, game_event_t* m_GameEvent )
{
	if ( g_interfaces.m_engine_client->get_player_for_user_id( m_GameEvent->get_int( ( "attacker" ) ) ) !=
	     g_interfaces.m_engine_client->get_local_player( ) )
		return;

	std::string_view szWeapon = std::string_view{ m_GameEvent->get_string( ( "weapon" ) ) };
	if ( szWeapon != ( "knife" ) && szWeapon != ( "knife_t" ) )
		return;

	C_EconItemView* pEconItemView = LocalInventory->GetItemInLoadout( static_cast< int32_t >( g_ctx.m_local->get_team( ) ), NULL );
	if ( !pEconItemView )
		return;

	C_EconItem* pSocData = pEconItemView->GetSocData( );
	if ( !pSocData )
		return;

	C_EconItemDefinition* pDefinition = g_items_manager.items[ pSocData->m_nWeaponID ];
	if ( !pDefinition )
		return;

	const char* szWeaponName = pDefinition->m_szItemName;
	if ( std::string_view{ ( std::string )( szWeaponName ) }._Starts_with( ( "weapon_" ) ) )
		m_GameEvent->set_w_string( ( "weapon" ), ( const wchar_t* )( szWeaponName + 7 ) );
}
void n_invetory_changer::impl_t::LiveStatTrack( IPlayerInventory* LocalInventory, game_event_t* m_GameEvent )
{
	if ( g_interfaces.m_engine_client->get_player_for_user_id( m_GameEvent->get_int( ( "attacker" ) ) ) !=
	     g_interfaces.m_engine_client->get_local_player( ) )
		return;

	c_base_entity* pWeapon =
		static_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity_from_handle( g_ctx.m_local->get_active_weapon_handle( ) ) );
	if ( !pWeapon )
		return;

	std::uint64_t iItemID = ( std::uint64_t( pWeapon->get_item_id_high( ) ) << 32 ) | pWeapon->get_item_id_low( );
	if ( !iItemID )
		return;

	C_EconItemView* pEconItemView = LocalInventory->GetInventoryItemByItemID( iItemID );
	if ( !pEconItemView )
		return;

	C_EconItem* pSocData = pEconItemView->GetSocData( );
	if ( !pSocData )
		return;

	auto aInventoryItems = m_aInventoryItemList.find( iItemID );
	if ( aInventoryItems == m_aInventoryItemList.end( ) )
		return;

	auto& pInventoryItem = aInventoryItems->second;
	if ( !pInventoryItem.m_StatTrack.enabled )
		return;

	++pInventoryItem.m_StatTrack.counter;
	pSocData->SetStatTrack( pInventoryItem.m_StatTrack.counter );
	LocalInventory->SoUpdated( LocalInventory->GetOwnerID( ), ( C_SharedObject* )pSocData, 4 );
}

bool SaveInventory( const std::unordered_map< uint64_t, InventoryItem_t >& inventory, const char* filename )
{
	HANDLE hFile = CreateFileA( filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;

	DWORD written = 0;

	// Сначала записываем количество элементов
	uint32_t count = static_cast< uint32_t >( inventory.size( ) );
	if ( !WriteFile( hFile, &count, sizeof( count ), &written, nullptr ) || written != sizeof( count ) ) {
		CloseHandle( hFile );
		return false;
	}

	// Для каждого элемента записываем ключ и данные
	for ( const auto& pair : inventory ) {
		uint64_t key                = pair.first;
		const InventoryItem_t& item = pair.second;

		// Записываем ключ
		if ( !WriteFile( hFile, &key, sizeof( key ), &written, nullptr ) || written != sizeof( key ) ) {
			CloseHandle( hFile );
			return false;
		}

		// Записываем простые поля
		if ( !WriteFile( hFile, &item.m_nItemID, sizeof( item.m_nItemID ), &written, nullptr ) || written != sizeof( item.m_nItemID ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_iPaintKit, sizeof( item.m_iPaintKit ), &written, nullptr ) || written != sizeof( item.m_iPaintKit ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_flPearlescent, sizeof( item.m_flPearlescent ), &written, nullptr ) ||
		     written != sizeof( item.m_flPearlescent ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_flWear, sizeof( item.m_flWear ), &written, nullptr ) || written != sizeof( item.m_flWear ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_flSeed, sizeof( item.m_flSeed ), &written, nullptr ) || written != sizeof( item.m_flSeed ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Записываем массив color (4x3 float)
		if ( !WriteFile( hFile, &item.color, sizeof( item.color ), &written, nullptr ) || written != sizeof( item.color ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_iQuality, sizeof( item.m_iQuality ), &written, nullptr ) || written != sizeof( item.m_iQuality ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Записываем булевы переменные
		if ( !WriteFile( hFile, &item.auto_equipment_ct, sizeof( item.auto_equipment_ct ), &written, nullptr ) ||
		     written != sizeof( item.auto_equipment_ct ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.auto_equipment_t, sizeof( item.auto_equipment_t ), &written, nullptr ) ||
		     written != sizeof( item.auto_equipment_t ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Структура m_CustomRarity
		if ( !WriteFile( hFile, &item.m_CustomRarity.enabled, sizeof( item.m_CustomRarity.enabled ), &written, nullptr ) ||
		     written != sizeof( item.m_CustomRarity.enabled ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_CustomRarity.counter, sizeof( item.m_CustomRarity.counter ), &written, nullptr ) ||
		     written != sizeof( item.m_CustomRarity.counter ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Структура m_StatTrack
		if ( !WriteFile( hFile, &item.m_StatTrack.enabled, sizeof( item.m_StatTrack.enabled ), &written, nullptr ) ||
		     written != sizeof( item.m_StatTrack.enabled ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_StatTrack.counter, sizeof( item.m_StatTrack.counter ), &written, nullptr ) ||
		     written != sizeof( item.m_StatTrack.counter ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Структура m_Customtag
		if ( !WriteFile( hFile, &item.m_Customtag.enabled, sizeof( item.m_Customtag.enabled ), &written, nullptr ) ||
		     written != sizeof( item.m_Customtag.enabled ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_Customtag.aTagName, sizeof( item.m_Customtag.aTagName ), &written, nullptr ) ||
		     written != sizeof( item.m_Customtag.aTagName ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Массив стикеров
		for ( size_t i = 0; i < item.m_aStickers.size( ); ++i ) {
			if ( !WriteFile( hFile, &item.m_aStickers[ i ].m_nID, sizeof( item.m_aStickers[ i ].m_nID ), &written, nullptr ) ||
			     written != sizeof( item.m_aStickers[ i ].m_nID ) ) {
				CloseHandle( hFile );
				return false;
			}
			if ( !WriteFile( hFile, &item.m_aStickers[ i ].m_flWear, sizeof( item.m_aStickers[ i ].m_flWear ), &written, nullptr ) ||
			     written != sizeof( item.m_aStickers[ i ].m_flWear ) ) {
				CloseHandle( hFile );
				return false;
			}
		}
		// Дополнительные поля
		if ( !WriteFile( hFile, &item.ct_team, sizeof( item.ct_team ), &written, nullptr ) || written != sizeof( item.ct_team ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.t_team, sizeof( item.t_team ), &written, nullptr ) || written != sizeof( item.t_team ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.none_team, sizeof( item.none_team ), &written, nullptr ) || written != sizeof( item.none_team ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !WriteFile( hFile, &item.m_iRarity, sizeof( item.m_iRarity ), &written, nullptr ) || written != sizeof( item.m_iRarity ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Сериализация строки SkinName: сначала длина, затем данные
		uint32_t strLength = static_cast< uint32_t >( item.SkinName.size( ) );
		if ( !WriteFile( hFile, &strLength, sizeof( strLength ), &written, nullptr ) || written != sizeof( strLength ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( strLength > 0 ) {
			if ( !WriteFile( hFile, item.SkinName.data( ), strLength, &written, nullptr ) || written != strLength ) {
				CloseHandle( hFile );
				return false;
			}
		}
	}
	CloseHandle( hFile );
	return true;
}

// Функция для загрузки данных из файла
bool LoadInventory( std::unordered_map< uint64_t, InventoryItem_t >& inventory, const char* filename )
{
	HANDLE hFile = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;

	DWORD readBytes = 0;
	uint32_t count  = 0;
	if ( !ReadFile( hFile, &count, sizeof( count ), &readBytes, nullptr ) || readBytes != sizeof( count ) ) {
		CloseHandle( hFile );
		return false;
	}



	for ( uint32_t i = 0; i < count; ++i ) {
		uint64_t key         = 0;
		InventoryItem_t item = { };
		// Чтение ключа
		if ( !ReadFile( hFile, &key, sizeof( key ), &readBytes, nullptr ) || readBytes != sizeof( key ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Чтение полей
		if ( !ReadFile( hFile, &item.m_nItemID, sizeof( item.m_nItemID ), &readBytes, nullptr ) || readBytes != sizeof( item.m_nItemID ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_iPaintKit, sizeof( item.m_iPaintKit ), &readBytes, nullptr ) || readBytes != sizeof( item.m_iPaintKit ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_flPearlescent, sizeof( item.m_flPearlescent ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.m_flPearlescent ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_flWear, sizeof( item.m_flWear ), &readBytes, nullptr ) || readBytes != sizeof( item.m_flWear ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_flSeed, sizeof( item.m_flSeed ), &readBytes, nullptr ) || readBytes != sizeof( item.m_flSeed ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.color, sizeof( item.color ), &readBytes, nullptr ) || readBytes != sizeof( item.color ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_iQuality, sizeof( item.m_iQuality ), &readBytes, nullptr ) || readBytes != sizeof( item.m_iQuality ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.auto_equipment_ct, sizeof( item.auto_equipment_ct ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.auto_equipment_ct ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.auto_equipment_t, sizeof( item.auto_equipment_t ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.auto_equipment_t ) ) {
			CloseHandle( hFile );
			return false;
		}
		// m_CustomRarity
		if ( !ReadFile( hFile, &item.m_CustomRarity.enabled, sizeof( item.m_CustomRarity.enabled ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.m_CustomRarity.enabled ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_CustomRarity.counter, sizeof( item.m_CustomRarity.counter ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.m_CustomRarity.counter ) ) {
			CloseHandle( hFile );
			return false;
		}
		// m_StatTrack
		if ( !ReadFile( hFile, &item.m_StatTrack.enabled, sizeof( item.m_StatTrack.enabled ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.m_StatTrack.enabled ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_StatTrack.counter, sizeof( item.m_StatTrack.counter ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.m_StatTrack.counter ) ) {
			CloseHandle( hFile );
			return false;
		}
		// m_Customtag
		if ( !ReadFile( hFile, &item.m_Customtag.enabled, sizeof( item.m_Customtag.enabled ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.m_Customtag.enabled ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_Customtag.aTagName, sizeof( item.m_Customtag.aTagName ), &readBytes, nullptr ) ||
		     readBytes != sizeof( item.m_Customtag.aTagName ) ) {
			CloseHandle( hFile );
			return false;
		}
		// Стикеры
		for ( size_t j = 0; j < item.m_aStickers.size( ); ++j ) {
			if ( !ReadFile( hFile, &item.m_aStickers[ j ].m_nID, sizeof( item.m_aStickers[ j ].m_nID ), &readBytes, nullptr ) ||
			     readBytes != sizeof( item.m_aStickers[ j ].m_nID ) ) {
				CloseHandle( hFile );
				return false;
			}
			if ( !ReadFile( hFile, &item.m_aStickers[ j ].m_flWear, sizeof( item.m_aStickers[ j ].m_flWear ), &readBytes, nullptr ) ||
			     readBytes != sizeof( item.m_aStickers[ j ].m_flWear ) ) {
				CloseHandle( hFile );
				return false;
			}
		}
		// Дополнительные поля
		if ( !ReadFile( hFile, &item.ct_team, sizeof( item.ct_team ), &readBytes, nullptr ) || readBytes != sizeof( item.ct_team ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.t_team, sizeof( item.t_team ), &readBytes, nullptr ) || readBytes != sizeof( item.t_team ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.none_team, sizeof( item.none_team ), &readBytes, nullptr ) || readBytes != sizeof( item.none_team ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( !ReadFile( hFile, &item.m_iRarity, sizeof( item.m_iRarity ), &readBytes, nullptr ) || readBytes != sizeof( item.m_iRarity ) ) {
			CloseHandle( hFile );
			return false;
		}
		// SkinName: читаем длину и затем строку
		uint32_t strLength = 0;
		if ( !ReadFile( hFile, &strLength, sizeof( strLength ), &readBytes, nullptr ) || readBytes != sizeof( strLength ) ) {
			CloseHandle( hFile );
			return false;
		}
		if ( strLength > 0 ) {
			char* buffer = new char[ strLength + 1 ];
			if ( !ReadFile( hFile, buffer, strLength, &readBytes, nullptr ) || readBytes != strLength ) {
				delete[] buffer;
				CloseHandle( hFile );
				return false;
			}
			buffer[ strLength ] = '\0';
			item.SkinName       = buffer;
			delete[] buffer;
		}
		// Добавляем элемент в контейнер
		inventory[ key ] = item;
	}

	CloseHandle( hFile );
	return true;
}
